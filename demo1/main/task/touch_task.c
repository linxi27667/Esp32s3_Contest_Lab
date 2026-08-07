#include "config.h"
#include "touch_task.h"

static const char *g_tag = "TOUCH_TASK";

void Touch_Task(void *argument)
{
    (void)argument;
    bool previous_pressed = false;

    while (true) {
        bool pressed = false;
        if (Touch_IsPressed(&pressed) == ESP_OK && pressed != previous_pressed) {
            previous_pressed = pressed;
            ESP_LOGI(g_tag, "Touch %s", pressed ? "pressed" : "released");

                if (pressed) {
        /* 按下时切换亮度 */
                     uint8_t brightness = RgbLed_GetBrightness();
                     brightness = (brightness == 128)?32:128;
                     RgbLed_SetBrightness(brightness);
                     Nvs_SetI32("bright", RgbLed_GetBrightness());
                     Buzzer_Beep_Once(2000, 100);
                     Uart_Sendf(UART_ID_1, "BRIGHT=%d\r\n", RgbLed_GetBrightness());
    }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
