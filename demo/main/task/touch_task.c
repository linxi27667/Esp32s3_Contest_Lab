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

            if (pressed && Buzzer_Beep_Once(2000, 100) != ESP_OK) {
                ESP_LOGW(g_tag, "Failed to start touch beep");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
