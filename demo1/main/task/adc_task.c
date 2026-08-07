#include "adc_task.h"
#include "config.h"

static const char *g_tag = "ADC_TASK";

/* 与 adc_task.h 中的 extern 声明对应，整个工程只能在此处定义一次。 */
adc_sample_t g_adc0_sample = {0};
uint8_t mv_to_vlaue = 0;
uint8_t alarm_count = 0;
void Adc_Task(void *argument)
{
    (void)argument;

    while (true) {
        if (Adc_Read_Sample(ADC_ID_0, &g_adc0_sample) == ESP_OK) {
            ESP_LOGI(
                g_tag,
                "ADC%u raw=%d pin_mv=%d",
                (unsigned)ADC_ID_0,
                g_adc0_sample.raw,
                g_adc0_sample.pin_mv);
        }
        mv_to_vlaue = g_adc0_sample.pin_mv * 100 / 3300;
        if(mv_to_vlaue <= 0) mv_to_vlaue = 0;
        else if (mv_to_vlaue >= 100) mv_to_vlaue = 100;
        alarm_count++;
        if (alarm_count >= 5) {
            alarm_count = 0;

            Uart_Sendf(
                UART_ID_1,
                "AA=%d MV=%d STATE=%s\r\n",
             mv_to_vlaue,
                g_adc0_sample.pin_mv,
                main_state == MAIN_STATE_ALARM ? "ALARM" : "NORMAL");
            }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
