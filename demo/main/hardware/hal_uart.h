#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* 像选 USART1/USART2 一样选业务串口。
 * 不占用 UART0（默认日志口）。ESP32-S3 业务可用 UART1、UART2。 */
typedef enum {
    UART_EXTERNAL = 0, /**< config.h 中配置的外部通信串口，默认映射 UART_NUM_1。 */
    UART_ID_COUNT,
} uart_id_t;

/**
 * @brief 初始化全部业务串口，安装驱动并配置波特率和 TX/RX 引脚。
 *
 * 串口号、引脚、波特率和缓冲区大小均在 config.h 中配置，UART0 日志口不受影响。
 *
 * @retval ESP_OK 全部串口初始化成功。
 * @retval ESP_ERR_INVALID_STATE 串口库已经初始化。
 * @retval 其他值 驱动安装、参数配置或引脚绑定失败。
 */
esp_err_t Uart_Init(void);

/**
 * @brief 发送指定长度的二进制数据。
 *
 * @param[in] id 串口编号，可填 UART_EXTERNAL。
 * @param[in] data 待发送数据的首地址，不能为 NULL；数据中可以包含 0x00。
 * @param[in] length 发送字节数，必须大于 0，且不能超过 data 实际可读长度。
 *
 * @retval ESP_OK 全部数据已写入发送缓冲区。
 * @retval ESP_ERR_INVALID_ARG data 为 NULL 或 length 为 0。
 * @retval ESP_ERR_INVALID_STATE 串口库未初始化、id 无效或该串口未就绪。
 * @retval ESP_FAIL UART 写入失败或实际写入长度不足。
 */
esp_err_t Uart_Send(uart_id_t id, const uint8_t *data, size_t length);

/**
 * @brief 发送一个以 '\0' 结尾的 C 字符串，不发送末尾的 '\0'。
 *
 * @param[in] id 串口编号，可填 UART_EXTERNAL。
 * @param[in] str 待发送字符串，必须是非 NULL、非空且以 '\0' 结尾的有效字符串。
 *
 * @retval ESP_OK 字符串发送成功。
 * @retval ESP_ERR_INVALID_ARG str 为 NULL 或空字符串。
 * @retval ESP_ERR_INVALID_STATE 串口库未初始化、id 无效或该串口未就绪。
 * @retval ESP_FAIL UART 写入失败或实际写入长度不足。
 */
esp_err_t Uart_SendStr(uart_id_t id, const char *str);

/**
 * @brief 按 printf 格式化字符串并通过指定串口发送。
 *
 * @param[in] id 串口编号，可填 UART_EXTERNAL。
 * @param[in] fmt printf 风格格式字符串，不能为 NULL，例如 "value=%d\r\n"。
 * @param[in] ... 与 fmt 格式占位符匹配的可变参数。
 *
 * @retval ESP_OK 格式化内容发送成功。
 * @retval ESP_ERR_INVALID_ARG fmt 为 NULL，或格式化后内容为空。
 * @retval ESP_ERR_INVALID_STATE 串口库未初始化、id 无效或该串口未就绪。
 * @retval ESP_FAIL 格式化或 UART 写入失败。
 * @note 最多发送 UART_PRINTF_BUFFER_SIZE - 1 字节，超出部分会被截断。
 */
esp_err_t Uart_Sendf(uart_id_t id, const char *fmt, ...);

/**
 * @brief 从指定串口读取最多 length 字节。
 *
 * @param[in] id 串口编号，可填 UART_EXTERNAL。
 * @param[out] buffer 接收数据的缓冲区地址，不能为 NULL。
 * @param[in] length buffer 可写容量和本次最多读取字节数，必须大于 0。
 * @param[in] timeout_ticks 等待时间，单位为 FreeRTOS tick。可填 0、
 *                          pdMS_TO_TICKS(ms) 或 portMAX_DELAY。
 *
 * @return 大于 0 表示实际读取字节数；0 表示超时且没有数据；-1 表示参数、状态或驱动错误。
 * @note 返回的数据不会自动添加字符串结束符 '\0'。若按字符串使用，应预留一个字节并手动补 '\0'。
 */
int Uart_Read(uart_id_t id, uint8_t *buffer, size_t length, TickType_t timeout_ticks);
