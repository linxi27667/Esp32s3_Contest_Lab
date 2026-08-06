#include "config.h"
#include "task_entries.h"

static const char *g_tag = "ADC_TASK";

void Adc_Task(void *argument)
{
    (void)argument;

    while (true) {
        adc_sample_t sample = {0};
        if (Adc_Read_Sample(ADC_ID_0, &sample) == ESP_OK) {
            ESP_LOGI(
                g_tag,
                "ADC%u raw=%d pin_mv=%d",
                (unsigned)ADC_ID_0,
                sample.raw,
                sample.pin_mv);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}
