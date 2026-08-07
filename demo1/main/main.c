#include "task_manager.h"
#include "config.h"
#include "debug_console.h"

static const char *g_tag = "MAIN";
/* ESP-IDF 应用入口只负责硬件初始化和启动应用任务。 */
void app_main(void)
{
    ESP_ERROR_CHECK(Nvs_Init());
    ESP_ERROR_CHECK(Led_Init());
    ESP_ERROR_CHECK(RgbLed_Init(RGB_LED_DEFAULT_COUNT));

    int32_t value = 0;
    if(Nvs_GetI32("mode", &value) == ESP_OK &&
    value >= 0 && value <= 3){
        f_mode = (uint8_t)value;
    }else {
    f_mode = 0;
    }

    if (Nvs_GetI32("bright", &value) == ESP_OK &&
    (value == 32 || value == 128)) {
    RgbLed_SetBrightness((uint8_t)value);
    } else {
    RgbLed_SetBrightness(32);
    }

    ESP_ERROR_CHECK(Buzzer_Init());
    ESP_ERROR_CHECK(Key_Interrupt_Init());
    ESP_ERROR_CHECK(Touch_Init());
    ESP_ERROR_CHECK(Timer_Init());
    ESP_ERROR_CHECK(Uart_Hal_Init());
    ESP_ERROR_CHECK(Adc_Init());
    ESP_ERROR_CHECK(Debug_Console_Init());

    ESP_ERROR_CHECK(AppTasks_Start());
    ESP_LOGI(g_tag, "Embedded contest workspace ready.");

    /* 业务任务已创建完成，显式删除 ESP-IDF 的启动任务。 */
    vTaskDelete(NULL);
}
