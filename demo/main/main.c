#include "config.h"
#include "debug_console.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *g_tag = "MAIN";

static TaskHandle_t g_key_task_handle = NULL;
static TaskHandle_t g_touch_task_handle = NULL;
static TaskHandle_t g_timer_task_handle = NULL;
static TaskHandle_t g_external_uart_task_handle = NULL;
static TaskHandle_t g_adc_task_handle = NULL;

/* 统一封装任务创建，并把创建失败转换为ESP错误码。 */
static esp_err_t Task_Create(
    TaskFunction_t task,
    const char *name,
    uint32_t stack_size,
    UBaseType_t priority,
    TaskHandle_t *handle)
{
    BaseType_t result = xTaskCreate(
        task,
        name,
        stack_size,
        NULL,
        priority,
        handle);

    if (result != pdPASS) {
        ESP_LOGE(g_tag, "Failed to create task: %s", name);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/* 启动一次指定频率的蜂鸣器声音，并由单次定时器负责自动停止。 */
static esp_err_t Buzzer_Beep_Once(uint32_t frequency_hz, uint32_t duration_ms)
{
    esp_err_t err = Buzzer_Passive_Play(frequency_hz);
    if (err != ESP_OK) {
        return err;
    }

    err = Timer_Start_OneShot_Ms(
        TIMER_ID_1,
        TIMER_EVENT_BUZZER_STOP,
        duration_ms);
    if (err != ESP_OK) {
        /* 定时器启动失败时，不能让蜂鸣器一直响。 */
        (void)Buzzer_Passive_Stop();
    }

    return err;
}

/* 按键应用任务：等待中断事件，消抖后处理按下业务。 */
static void Key_Task(void *argument)
{
    (void)argument;

    key_event_t event;

    while (true) {
        /* 任务阻塞等待中断事件，不占用CPU。 */
        if (Key_GetEvent(&event, portMAX_DELAY) != ESP_OK) {
            continue;
        }

        /* 按键消抖：等待触点稳定后再读取一次。 */
        vTaskDelay(pdMS_TO_TICKS(20));

        bool pressed = false;
        if (Key_IsPressed(&pressed) != ESP_OK || !pressed) {
            continue;
        }

        /* 在这里写按键按下后的业务。 */
        ESP_LOGI(g_tag, "Key pressed on GPIO%d", event.pin);

        if (Buzzer_Beep_Once(1000, 100) != ESP_OK) {
            ESP_LOGW(g_tag, "Failed to start key beep");
        }
    }
}

/* 触摸应用任务：触摸硬件连续扫描，任务周期读取并检测状态边沿。 */
static void Touch_Task(void *argument)
{
    (void)argument;

    bool previous_pressed = false;

    while (true) {
        bool pressed = false;
        if (Touch_IsPressed(&pressed) == ESP_OK && pressed != previous_pressed) {
            previous_pressed = pressed;
            ESP_LOGI(g_tag, "Touch %s", pressed ? "pressed" : "released");

            if (pressed && Buzzer_Beep_Once(2000, 100) != ESP_OK) {
                ESP_LOGW(g_tag, "Failed to start touch beep");
            }
        }

        /* 20 ms 足够完成触摸响应，同时避免任务空转占用 CPU。 */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* 定时器应用任务：阻塞等待闹钟事件并处理业务。 */
static void Timer_Task(void *argument)
{
    (void)argument;

    timer_event_t event;

    while (true) {
        /* 任务阻塞等待定时器闹钟事件。 */
        if (Timer_GetEvent(&event, portMAX_DELAY) != ESP_OK) {
            continue;
        }

        /* 每一路定时器使用独立分支，单次事件不需要额外 flag 判断。 */
        switch (event.type) {
        case TIMER_EVENT_PERIODIC_TICK:
            /* 当前示例中 TIMER_ID_0 每秒产生一次周期事件。 */
            ESP_LOGI(
                g_tag,
                "Timer0 periodic: period=%lums count=%llu us",
                (unsigned long)event.period_ms,
                (unsigned long long)event.count_us);
            break;

        case TIMER_EVENT_BUZZER_STOP:
            /* 按键或 Touch 触发的 100 ms 单次蜂鸣器事件。 */
            if (Buzzer_Passive_Stop() != ESP_OK) {
                ESP_LOGW(g_tag, "Failed to stop passive buzzer");
            }
            break;

        case TIMER_EVENT_LED_BLINK:
            /* 预留给 ADC 报警闪烁或其他周期业务。 */
            ESP_LOGI(g_tag, "Timer2 event");
            break;

        case TIMER_EVENT_RGB_REFRESH:
            /* 预留给 RGB 动画或其他单次业务。 */
            ESP_LOGI(g_tag, "Timer3 event");
            break;

        default:
            ESP_LOGW(g_tag, "Unknown timer event: %u", (unsigned)event.id);
            break;
        }
    }
}

/* 外部 UART 任务：读取外部数据，协议确定后在此接入解析函数。 */
static void External_Uart_Task(void *argument)
{
    (void)argument;

    uint8_t buffer[64];

    while (true) {
        int length = Uart_Read(
            UART_EXTERNAL,
            buffer,
            sizeof(buffer),
            portMAX_DELAY);
        if (length > 0) {
            ESP_LOGD(g_tag, "Received %d bytes from external UART", length);
            ESP_LOG_BUFFER_HEX_LEVEL(
                g_tag,
                buffer,
                (size_t)length,
                ESP_LOG_DEBUG);

            /* TODO: 根据外部设备协议调用 External_Protocol_Feed()。 */
        }
    }
}

/* ADC应用任务：周期采样；用 ADC_ID_x 选择通道。 */
static void Adc_Task(void *argument)
{
    (void)argument;

    while (true) {
        adc_sample_t sample = {0};

        /* 一次采样同时得到 raw 和校准后的 ADC 引脚电压。 */
        if (Adc_Read_Sample(ADC_ID_0, &sample) == ESP_OK) {
            ESP_LOGI(
                g_tag,
                "ADC%u raw=%d pin_mv=%d",
                (unsigned)ADC_ID_0,
                sample.raw,
                sample.pin_mv);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}

/* 创建全部应用任务；统一任务注册入口。 */
static esp_err_t Task_CreateAll(void)
{
    /* 输入和数据源任务只在这里创建；LED、RGB 灯和蜂鸣器由业务事件直接驱动。 */
    esp_err_t err = Task_Create(
        Key_Task,
        "Key_Task",
        KEY_TASK_STACK_SIZE,
        KEY_TASK_PRIORITY,
        &g_key_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Touch_Task,
        "Touch_Task",
        TOUCH_TASK_STACK_SIZE,
        TOUCH_TASK_PRIORITY,
        &g_touch_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Timer_Task,
        "Timer_Task",
        TIMER_TASK_STACK_SIZE,
        TIMER_TASK_PRIORITY,
        &g_timer_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        External_Uart_Task,
        "External_Uart_Task",
        EXTERNAL_UART_TASK_STACK_SIZE,
        EXTERNAL_UART_TASK_PRIORITY,
        &g_external_uart_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    return Task_Create(
        Adc_Task,
        "Adc_Task",
        ADC_TASK_STACK_SIZE,
        ADC_TASK_PRIORITY,
        &g_adc_task_handle);
}

/* ESP-IDF应用入口：初始化外设并创建全部任务。 */
void app_main(void)
{
    /* ESP-IDF应用入口 */
    ESP_ERROR_CHECK(Nvs_Init());
    ESP_ERROR_CHECK(Led_Init());
    ESP_ERROR_CHECK(RgbLed_Init(RGB_LED_DEFAULT_COUNT));
    ESP_ERROR_CHECK(Buzzer_Init());
    ESP_ERROR_CHECK(Key_Interrupt_Init());
    ESP_ERROR_CHECK(Touch_Init());
    ESP_ERROR_CHECK(Timer_Init());
    /* 示例：只开 TIMER_ID_0；需要更多路时再 Start TIMER_ID_1..3。 */
    ESP_ERROR_CHECK(Timer_Start_Periodic_Ms(
        TIMER_ID_0,
        TIMER_EVENT_PERIODIC_TICK,
        TIMER_DEFAULT_PERIOD_MS));
    ESP_ERROR_CHECK(Uart_Init());
    ESP_ERROR_CHECK(Adc_Init());
    ESP_ERROR_CHECK(Debug_Console_Init());
    ESP_ERROR_CHECK(Task_CreateAll());
    ESP_LOGI(g_tag, "Embedded contest workspace ready.");
}
