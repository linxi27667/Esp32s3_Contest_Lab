#include "adc_task.h"
#include "config.h"

static const char *g_tag = "ADC_TASK";

/* 与 adc_task.h 中的 extern 声明对应，整个工程只能在此处定义一次。 */
adc_sample_t g_adc0_sample = {0};

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

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}
