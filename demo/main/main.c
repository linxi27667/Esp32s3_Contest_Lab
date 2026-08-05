#include "config.h"
#include "debug_console.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *g_tag = "MAIN";

static TaskHandle_t g_led_task_handle = NULL;
static TaskHandle_t g_buzzer_task_handle = NULL;
static TaskHandle_t g_key_task_handle = NULL;
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

/* LED应用任务模板：在循环中编写业务，每轮必须阻塞。 */
static void Led_Task(void *argument)
{
    (void)argument;

    while (true) {
        /* LED任务业务写在这里；每轮必须等待、延时或阻塞。 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 蜂鸣器应用任务模板：调用有源/无源蜂鸣器接口。 */
static void Buzzer_Task(void *argument)
{
    (void)argument;

    while (true) {
        /* 蜂鸣器业务写在这里，调用Buzzer_Active_Set()或Buzzer_Passive_Play()。 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
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

        /* 在这里写定时到达后的业务；用 event.id 区分 TIMER_ID_0..TIMER_ID_3。 */
        ESP_LOGI(
            g_tag,
            "Timer%u alarm: period=%lums count=%llu us",
            (unsigned)event.id,
            (unsigned long)event.period_ms,
            (unsigned long long)event.count_us);
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
    /* 所有应用任务只在这里创建。 */
    esp_err_t err = Task_Create(
        Led_Task,
        "Led_Task",
        LED_TASK_STACK_SIZE,
        LED_TASK_PRIORITY,
        &g_led_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Buzzer_Task,
        "Buzzer_Task",
        BUZZER_TASK_STACK_SIZE,
        BUZZER_TASK_PRIORITY,
        &g_buzzer_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Key_Task,
        "Key_Task",
        KEY_TASK_STACK_SIZE,
        KEY_TASK_PRIORITY,
        &g_key_task_handle);
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
    ESP_ERROR_CHECK(Led_Init());
    ESP_ERROR_CHECK(Buzzer_Init());
    ESP_ERROR_CHECK(Key_Interrupt_Init());
    ESP_ERROR_CHECK(Timer_Init());
    /* 示例：只开 TIMER_ID_0；需要更多路时再 Start TIMER_ID_1..3。 */
    ESP_ERROR_CHECK(Timer_Start_Periodic_Ms(TIMER_ID_0, TIMER_DEFAULT_PERIOD_MS));
    ESP_ERROR_CHECK(Uart_Init());
    ESP_ERROR_CHECK(Adc_Init());
    ESP_ERROR_CHECK(Debug_Console_Init());
    ESP_ERROR_CHECK(Task_CreateAll());
    ESP_LOGI(g_tag, "Embedded contest workspace ready.");
}
