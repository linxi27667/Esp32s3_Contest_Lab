#include "config.h"
#include "key_task.h"

/* KEY_ID_0 的状态机和业务辅助函数都放在按键任务文件中。 */
static void Key0_HandleEvent(const key_event_t *event)
{
    switch (event->type) {
    case KEY_EVENT_PRESSED:
        break;

    case KEY_EVENT_RELEASED:
        f_mode++;
        if(f_mode > 3) f_mode = 0;
        Nvs_SetI32("mode", f_mode);
        Buzzer_Beep_Once(1000, 100);
        Uart_Sendf(UART_ID_1, "MODE=%u\r\n", (unsigned)f_mode);
        break;

    default:
        break;
    }
}

/* 第一层按 KEY_ID 分发，第二层由各按键处理自己的事件类型。 */
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

        /* 中断只负责上报边沿，任务延时 20 ms 后确认稳定电平。 */
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
