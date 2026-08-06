#pragma once

#include "esp_err.h"

/**
 * @brief 初始化 ESP-IDF 调试控制台并注册调试命令。
 *
 * 调试控制台使用 sdkconfig 配置的默认 console 通道，当前为 UART_NUM_0。
 * ESP-IDF 会创建 REPL 任务，应用层不需要再次创建调试接收任务。
 *
 * @retval ESP_OK 调试控制台初始化成功。
 * @retval ESP_ERR_INVALID_STATE console 已经初始化。
 * @retval ESP_ERR_NO_MEM 控制台或命令注册内存不足。
 * @retval 其他值 UART 驱动或 REPL 创建失败。
 */
esp_err_t Debug_Console_Init(void);
