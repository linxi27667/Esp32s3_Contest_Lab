#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* UART_ID_x 是 ESP32-S3 芯片内部的 UART 资源编号。 */
typedef enum {
    UART_ID_0 = 0, /**< UART_NUM_0，通常由 Type-C 调试/下载链路使用。 */
    UART_ID_1,     /**< UART_NUM_1，默认用于 GPIO 外部通信。 */
    UART_ID_2,     /**< UART_NUM_2，备用普通 UART 资源。 */
    UART_ID_COUNT,
} uart_id_t;

/* 当前工程的外部上位机串口使用 UART1。 */
#define UART_EXTERNAL UART_ID_1

/**
 * @brief 初始化所有启用的业务 UART。
 *
 * UART0 默认由 ESP-IDF console 使用，因此默认不由本 HAL 重复初始化。
 * 每一路是否启用、串口号、TX/RX 引脚和波特率在 config.h 中配置。
 */
esp_err_t Uart_Init(void);

/**
 * @brief 发送指定长度的二进制数据。
 */
esp_err_t Uart_Send(uart_id_t id, const uint8_t *data, size_t length);

/**
 * @brief 发送一个 C 字符串，不发送结尾的 '\0'。
 */
esp_err_t Uart_SendStr(uart_id_t id, const char *str);

/**
 * @brief 使用 printf 格式向指定 UART 发送字符串。
 */
esp_err_t Uart_Sendf(uart_id_t id, const char *fmt, ...);

/**
 * @brief 从指定 UART 读取数据。
 *
 * @return 大于 0 表示读取到的字节数，0 表示超时，-1 表示参数或驱动错误。
 */
int Uart_Read(
    uart_id_t id,
    uint8_t *buffer,
    size_t length,
    TickType_t timeout_ticks);
