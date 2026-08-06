#include "config.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* 中断和任务之间通过队列传递按键事件。 */
static QueueHandle_t g_key_event_queue = NULL;
static bool g_key_initialized = false;
/* 轮询模式保存上一次确认过的稳定状态。 */
static bool g_key_scan_pressed = false;

/* 按键GPIO中断回调：仅入队事件，不在ISR中消抖。 */
static void IRAM_ATTR Key_Isr_Handler(void *argument)
{
    (void)argument;

    const key_event_t event = {
        .pin = KEY_PIN,
    };
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (g_key_event_queue != NULL) {
        xQueueSendFromISR(
            g_key_event_queue,
            &event,
            &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/* 按指定中断类型配置按键输入GPIO。 */
static esp_err_t Key_Gpio_Init(gpio_int_type_t interrupt_type)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << KEY_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = interrupt_type,
    };

    return gpio_config(&config);
}

/* 只初始化按键GPIO，供轮询扫描模式使用。 */
esp_err_t Key_Init(void)
{
    if (g_key_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = Key_Gpio_Init(GPIO_INTR_DISABLE);
    if (err != ESP_OK) {
        return err;
    }

    g_key_initialized = true;
    return ESP_OK;
}

/* 配置按键GPIO并安装外部中断。 */
esp_err_t Key_Interrupt_Init(void)
{
    if (g_key_event_queue != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 中断模式先完成GPIO初始化，再创建事件队列。 */
    esp_err_t err = Key_Init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    /* 中断模式才打开下降沿触发。 */
    err = gpio_set_intr_type(KEY_PIN, GPIO_INTR_NEGEDGE);
    if (err != ESP_OK) {
        return err;
    }

    g_key_event_queue = xQueueCreate(
        KEY_QUEUE_LENGTH,
        sizeof(key_event_t));
    if (g_key_event_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        vQueueDelete(g_key_event_queue);
        g_key_event_queue = NULL;
        return err;
    }

    err = gpio_isr_handler_add(KEY_PIN, Key_Isr_Handler, NULL);
    if (err != ESP_OK) {
        vQueueDelete(g_key_event_queue);
        g_key_event_queue = NULL;
        return err;
    }

    return ESP_OK;
}

/* 在任务中等待按键中断事件。 */
esp_err_t Key_GetEvent(key_event_t *event, TickType_t timeout_ticks)
{
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_key_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return xQueueReceive(
               g_key_event_queue,
               event,
               timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

/* 读取当前按键是否处于按下状态。 */
esp_err_t Key_IsPressed(bool *pressed)
{
    if (pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_key_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    *pressed = gpio_get_level(KEY_PIN) == KEY_ACTIVE_LEVEL;
    return ESP_OK;
}

/* 轮询扫描按键并用延时消抖；只能在任务中调用，不能在ISR中调用。 */
esp_err_t Key_Scan(bool *pressed)
{
    if (pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_key_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    bool current_pressed = false;
    esp_err_t err = Key_IsPressed(&current_pressed);
    if (err != ESP_OK) {
        return err;
    }

    /* 只有检测到状态变化时才延时，避免每次扫描都阻塞20ms。 */
    if (current_pressed != g_key_scan_pressed) {
        vTaskDelay(pdMS_TO_TICKS(20));

        err = Key_IsPressed(&current_pressed);
        if (err != ESP_OK) {
            return err;
        }

        /* 延时后状态仍然一致，才认为按键状态真的改变。 */
        if (current_pressed != g_key_scan_pressed) {
            g_key_scan_pressed = current_pressed;
        }
    }

    *pressed = g_key_scan_pressed;
    return ESP_OK;
}
