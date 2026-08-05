#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"

typedef struct {
    uart_port_t port;
    int tx_pin;
    int rx_pin;
    int baud_rate;
    bool ready;
} uart_slot_t;

static uart_slot_t g_uarts[UART_ID_COUNT] = {
    {
        .port = UART_EXTERNAL_PORT_NUM,
        .tx_pin = UART_EXTERNAL_TX_PIN,
        .rx_pin = UART_EXTERNAL_RX_PIN,
        .baud_rate = UART_EXTERNAL_BAUD_RATE,
        .ready = false,
    },
};

static bool g_driver_ready = false;

/* 初始化单路业务串口；失败时删除已安装驱动。 */
static esp_err_t Uart_Init_One(uart_id_t id)
{
    uart_slot_t *slot = &g_uarts[id];

    const uart_config_t uart_config = {
        .baud_rate = slot->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(
        slot->port,
        UART_RX_BUFFER_SIZE,
        UART_TX_BUFFER_SIZE,
        0,
        NULL,
        0);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_param_config(slot->port, &uart_config);
    if (err != ESP_OK) {
        uart_driver_delete(slot->port);
        return err;
    }

    err = uart_set_pin(
        slot->port,
        slot->tx_pin,
        slot->rx_pin,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        uart_driver_delete(slot->port);
        return err;
    }

    slot->ready = true;
    return ESP_OK;
}

/* 失败时回收已初始化的串口。 */
static void Uart_Deinit_All(void)
{
    for (int i = 0; i < UART_ID_COUNT; ++i) {
        if (g_uarts[i].ready) {
            uart_driver_delete(g_uarts[i].port);
            g_uarts[i].ready = false;
        }
    }
    g_driver_ready = false;
}

/* 初始化全部业务串口：装驱动、配参数、绑引脚。 */
esp_err_t Uart_Init(void)
{
    if (g_driver_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < UART_ID_COUNT; ++i) {
        esp_err_t err = Uart_Init_One((uart_id_t)i);
        if (err != ESP_OK) {
            Uart_Deinit_All();
            return err;
        }
    }

    g_driver_ready = true;
    return ESP_OK;
}

/* 发送指定长度的二进制数据。 */
esp_err_t Uart_Send(uart_id_t id, const uint8_t *data, size_t length)
{
    if (!g_driver_ready || id < 0 || id >= UART_ID_COUNT || !g_uarts[id].ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int written = uart_write_bytes(g_uarts[id].port, data, length);
    if (written < 0) {
        return ESP_FAIL;
    }
    return ((size_t)written == length) ? ESP_OK : ESP_FAIL;
}

/* 发送一个C字符串（不含结尾'\0'）。 */
esp_err_t Uart_SendStr(uart_id_t id, const char *str)
{
    if (str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return Uart_Send(id, (const uint8_t *)str, strlen(str));
}

/* printf 风格发送，方便上位机调试打印。 */
esp_err_t Uart_Sendf(uart_id_t id, const char *fmt, ...)
{
    if (!g_driver_ready || id < 0 || id >= UART_ID_COUNT || !g_uarts[id].ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (fmt == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char buffer[UART_PRINTF_BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length < 0) {
        return ESP_FAIL;
    }
    if ((size_t)length >= sizeof(buffer)) {
        length = (int)sizeof(buffer) - 1;
    }

    return Uart_Send(id, (const uint8_t *)buffer, (size_t)length);
}

/* 读取最多 length 字节；返回实际读到的字节数，超时返回 0，错误返回 -1。 */
int Uart_Read(uart_id_t id, uint8_t *buffer, size_t length, TickType_t timeout_ticks)
{
    if (!g_driver_ready || id < 0 || id >= UART_ID_COUNT || !g_uarts[id].ready) {
        return -1;
    }
    if (buffer == NULL || length == 0) {
        return -1;
    }

    return uart_read_bytes(g_uarts[id].port, buffer, length, timeout_ticks);
}
