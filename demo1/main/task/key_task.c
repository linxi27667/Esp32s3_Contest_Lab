#include "config.h"
#include "key_task.h"
#include "main_task.h"

static const char *g_tag = "KEY_TASK";

static void Key0_HandleEvent(const key_event_t *event)
{
    switch (event->type) {
    case KEY_EVENT_PRESSED:
        ESP_LOGI(g_tag, "KEY_ID_0 pressed on GPIO%d", event->pin);
        break;

    case KEY_EVENT_RELEASED: {
        /* 一次完整点击以释放边沿为准，避免按下和释放各计数一次。 */
        const uint8_t mode = MainTask_NextMode();
        if (Uart_Sendf(UART_ID_1, "MODE=%u\r\n", (unsigned)mode) != ESP_OK) {
            ESP_LOGW(g_tag, "Failed to send mode");
        }
        if (Buzzer_Beep_Once(1000, 100) != ESP_OK) {
            ESP_LOGW(g_tag, "Failed to start key beep");
        }
        ESP_LOGI(g_tag, "KEY_ID_0 released on GPIO%d", event->pin);
        break;
    }

    default:
        ESP_LOGW(g_tag, "Unknown KEY_ID_0 event: %u", (unsigned)event->type);
        break;
    }
}

static void Key_HandleEvent(const key_event_t *event)
{
    switch (event->id) {
    case KEY_ID_0:
        Key0_HandleEvent(event);
        break;
    case KEY_ID_1:
        /* TODO: 添加 KEY_ID_1 的事件处理函数。 */
        break;
    case KEY_ID_2:
        /* TODO: 添加 KEY_ID_2 的事件处理函数。 */
        break;
    case KEY_ID_3:
        /* TODO: 添加 KEY_ID_3 的事件处理函数。 */
        break;
    default:
        ESP_LOGW(g_tag, "Unknown key id: %u", (unsigned)event->id);
        break;
    }
}

void Key_Task(void *argument)
{
    (void)argument;
    key_event_t event;

    while (true) {
        if (Key_GetEvent(&event, portMAX_DELAY) != ESP_OK) {
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(20));

        bool pressed = false;
        if (Key_IsPressed(event.id, &pressed) != ESP_OK) {
            continue;
        }

        const bool expected_pressed = event.type == KEY_EVENT_PRESSED;
        if (pressed == expected_pressed) {
            Key_HandleEvent(&event);
        }
    }
}
