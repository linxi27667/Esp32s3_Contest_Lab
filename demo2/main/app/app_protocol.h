#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef void (*protocol_command_handler_t)(const char *command);

/**
 * @brief 初始化 UART 文本协议解析器并注册命令处理函数。
 *
 * @param[in] handler 收到一条完整命令后的处理函数，不能为 NULL。
 *                    command 是以 '\0' 结尾的字符串，仅在回调期间有效。
 *
 * @retval ESP_OK 初始化成功。
 * @retval ESP_ERR_INVALID_ARG handler 为 NULL。
 * @note 解析器只供一个接收任务调用，不应在中断中使用。
 */
esp_err_t Protocol_Init(protocol_command_handler_t handler);

/**
 * @brief 向解析器输入 UART 收到的数据，并按换行符拆分完整命令。
 *
 * 每条命令使用 '\n' 结束，也兼容上位机常用的 "\r\n"。一次调用可以输入
 * 半条命令、一条命令或多条命令。
 *
 * @param[in] data UART 接收数据的首地址，不能为 NULL。
 * @param[in] length data 中的有效字节数，必须大于 0。
 *
 * @retval ESP_OK 数据处理成功。
 * @retval ESP_ERR_INVALID_ARG data 为 NULL 或 length 为 0。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Protocol_Init()。
 * @retval ESP_ERR_INVALID_SIZE 命令超过 UART_COMMAND_MAX_LENGTH，已丢弃到下一换行符。
 * @note 命令处理函数在 Protocol_Feed() 内同步调用，应避免长时间阻塞。
 */
esp_err_t Protocol_Feed(const uint8_t *data, size_t length);

/**
 * @brief 清除尚未接收完整的命令和超长命令丢弃状态。
 *
 * @note 不会清除 Protocol_Init() 注册的命令处理函数。
 */
void Protocol_Reset(void);
