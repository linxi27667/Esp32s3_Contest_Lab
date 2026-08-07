#include "app_protocol.h"

#include <stdbool.h>

#include "config.h"

static protocol_command_handler_t g_handler = NULL;
static char g_line[UART_COMMAND_MAX_LENGTH + 1];
static size_t g_line_length = 0;
static bool g_discarding = false;
static bool g_skip_next_lf = false;

static void Protocol_Dispatch_Line(void)
{
    if (g_line_length == 0) {
        return;
    }

    g_line[g_line_length] = '\0';
    g_handler(g_line);
    g_line_length = 0;
}

esp_err_t Protocol_Init(protocol_command_handler_t handler)
{
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    g_handler = handler;
    Protocol_Reset();
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
            if (byte == '\r' || byte == '\n') {
                g_discarding = false;
            }
            continue;
        }

        if (byte == '\r') {
            Protocol_Dispatch_Line();
            g_skip_next_lf = true;
            continue;
        }

        if (byte == '\n') {
            if (!g_skip_next_lf) {
                Protocol_Dispatch_Line();
            }
            g_skip_next_lf = false;
            continue;
        }

        g_skip_next_lf = false;

        if (g_line_length >= UART_COMMAND_MAX_LENGTH) {
            g_line_length = 0;
            g_discarding = true;
            result = ESP_ERR_INVALID_SIZE;
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
    g_skip_next_lf = false;
}
