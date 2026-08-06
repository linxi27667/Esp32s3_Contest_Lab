#include "app_protocol.h"

#include <stdbool.h>

#include "config.h"
#include "esp_log.h"

static const char *g_tag = "PROTOCOL";

static protocol_command_handler_t g_handler = NULL;
static char g_line[UART_COMMAND_MAX_LENGTH + 1];
static size_t g_line_length = 0;
static bool g_discarding = false;

esp_err_t Protocol_Init(protocol_command_handler_t handler)
{
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    g_handler = handler;
    Protocol_Reset();
    ESP_LOGI(g_tag, "UART text protocol ready, max command length=%u",
             (unsigned)UART_COMMAND_MAX_LENGTH);
    return ESP_OK;
}

esp_err_t Protocol_Feed(const uint8_t *data, size_t length)
{
    if (g_handler == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = ESP_OK;

    for (size_t i = 0; i < length; ++i) {
        char byte = (char)data[i];

        if (g_discarding) {
            if (byte == '\n') {
                g_discarding = false;
            }
            continue;
        }

        if (byte == '\r') {
            continue;
        }

        if (byte == '\n') {
            if (g_line_length > 0) {
                g_line[g_line_length] = '\0';
                g_handler(g_line);
                g_line_length = 0;
            }
            continue;
        }

        if (g_line_length >= UART_COMMAND_MAX_LENGTH) {
            g_line_length = 0;
            g_discarding = true;
            result = ESP_ERR_INVALID_SIZE;
            ESP_LOGW(g_tag, "Command too long, discard until newline");
            continue;
        }

        g_line[g_line_length++] = byte;
    }

    return result;
}

void Protocol_Reset(void)
{
    g_line_length = 0;
    g_discarding = false;
}
