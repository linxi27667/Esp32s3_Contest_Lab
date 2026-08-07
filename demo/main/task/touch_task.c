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
                uint8_t brightness = RgbLed_GetBrightness();
                brightness = brightness == 128 ? 32 : 128;

                (void)RgbLed_SetBrightness(brightness);
                (void)RgbLed_Refresh();
                /* NVS 保存模板：亮度修改成功后立即写入 Flash。 */
                (void)Nvs_SetI32("bright", brightness);
                (void)Buzzer_Beep_Once(2000, 100);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
