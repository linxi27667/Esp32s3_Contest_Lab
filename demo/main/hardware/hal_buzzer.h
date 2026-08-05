#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief 初始化有源蜂鸣器 GPIO 和无源蜂鸣器 LEDC PWM。
 *
 * 引脚、有效电平由 config.h 配置。初始化完成后两个蜂鸣器均保持关闭。
 *
 * @retval ESP_OK 初始化成功。
 * @retval 其他值 GPIO 或 LEDC 初始化失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Buzzer_Init(void);

/**
 * @brief 打开或关闭有源蜂鸣器。
 *
 * @param[in] on true 表示打开并发声，false 表示关闭。
 *
 * @retval ESP_OK 设置成功。
 * @retval 其他值 GPIO 输出失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Buzzer_Active_Set(bool on);

/**
 * @brief 使用 LEDC PWM 驱动无源蜂鸣器输出指定频率。
 *
 * @param[in] frequency_hz 目标频率，单位 Hz，必须大于 0。
 *                         常用值如 1000、2000、4000，实际范围受 LEDC 时钟限制。
 *
 * @retval ESP_OK 开始输出指定频率。
 * @retval ESP_ERR_INVALID_ARG frequency_hz 为 0 或 LEDC 不支持该频率。
 * @retval 其他值 LEDC 占空比配置失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Buzzer_Passive_Play(uint32_t frequency_hz);

/**
 * @brief 停止无源蜂鸣器 PWM 输出。
 *
 * @retval ESP_OK 停止成功。
 * @retval 其他值 LEDC 占空比配置失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Buzzer_Passive_Stop(void);
