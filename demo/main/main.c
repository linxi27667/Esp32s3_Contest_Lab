#include "app_tasks.h"
#include "config.h"
#include "debug_console.h"

static const char *g_tag = "MAIN";

/* ESP-IDF 应用入口只负责硬件初始化和启动应用任务。 */
void app_main(void)
{
    ESP_ERROR_CHECK(Nvs_Init());
    ESP_ERROR_CHECK(Led_Init());
    ESP_ERROR_CHECK(RgbLed_Init(RGB_LED_DEFAULT_COUNT));
    ESP_ERROR_CHECK(Buzzer_Init());
    ESP_ERROR_CHECK(Key_Interrupt_Init());
    ESP_ERROR_CHECK(Touch_Init());
    ESP_ERROR_CHECK(Timer_Init());
    ESP_ERROR_CHECK(Uart_Init());
    ESP_ERROR_CHECK(Adc_Init());
    ESP_ERROR_CHECK(Debug_Console_Init());

    ESP_ERROR_CHECK(AppTasks_Start());
    ESP_LOGI(g_tag, "Embedded contest workspace ready.");
}
