#include "config.h"
#include "esp_log_buffer.h"
#include "task_entries.h"

static const char *g_tag = "UART_TASK";

void Uart_Task(void *argument)
{
    (void)argument;
    uint8_t buffer[64];

    while (true) {
        int length = Uart_Read(
            UART_ID_1,
            buffer,
            sizeof(buffer),
            portMAX_DELAY);
        if (length > 0) {
            ESP_LOGD(g_tag, "UART1 received %d bytes", length);
            ESP_LOG_BUFFER_HEX_LEVEL(
                g_tag, buffer, (size_t)length, ESP_LOG_DEBUG);

            /* TODO: 在这里调用 Protocol_Feed(buffer, length)。 */
        }
    }
}
