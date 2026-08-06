#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/** @brief 一次触摸通道采样得到的原始值、滤波值和动态基准值。 */
typedef struct {
    uint32_t raw;       /**< 未经过滤波的硬件原始值。 */
    uint32_t smooth;    /**< 经过 IIR 滤波后的平滑值。 */
    uint32_t benchmark; /**< 驱动动态维护的未触摸基准值。 */
} touch_sample_t;

/**
 * @brief 初始化一个电容触摸通道并启动连续扫描。
 *
 * 通道编号、初始扫描次数、超时时间和默认阈值百分比由 config.h 配置。
 * 初始化过程会进行多次单次扫描以建立稳定基准，再按基准值配置硬件阈值。
 *
 * @retval ESP_OK 初始化成功，触摸通道已开始连续扫描。
 * @retval ESP_ERR_INVALID_STATE 触摸驱动已经初始化。
 * @retval 其他值 控制器、通道、滤波器或扫描配置失败。
 */
esp_err_t Touch_Init(void);

/**
 * @brief 读取当前触摸通道的原始值、平滑值和基准值。
 *
 * @param[out] sample_out 接收 touch_sample_t 结果的地址，不能为 NULL。
 *
 * @retval ESP_OK 三种采样数据均读取成功。
 * @retval ESP_ERR_INVALID_ARG sample_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Touch_Init()。
 * @retval 其他值 触摸驱动读取失败。
 */
esp_err_t Touch_Read(touch_sample_t *sample_out);

/**
 * @brief 根据当前平滑值和基准值判断触摸片是否被按下。
 *
 * 判定公式为 smooth >= benchmark + benchmark * threshold_percent / 100。
 * 阈值越小越灵敏，也越容易受环境噪声影响。
 *
 * @param[out] pressed_out 接收触摸状态；true 表示已触摸，不能为 NULL。
 *
 * @retval ESP_OK 判断成功。
 * @retval ESP_ERR_INVALID_ARG pressed_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Touch_Init()。
 * @retval 其他值 触摸采样读取失败。
 */
esp_err_t Touch_IsPressed(bool *pressed_out);

/**
 * @brief 运行时修改触摸阈值百分比，并重新配置硬件阈值。
 *
 * 本函数会临时停止连续扫描，使用当前基准值更新通道配置，然后恢复连续扫描。
 *
 * @param[in] percent 新阈值百分比，有效范围为 1~100。
 *
 * @retval ESP_OK 阈值更新并恢复扫描成功。
 * @retval ESP_ERR_INVALID_ARG percent 不在 1~100 范围内。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Touch_Init()。
 * @retval 其他值 采样、停止、重配置或恢复扫描失败。
 */
esp_err_t Touch_SetThresholdPercent(uint8_t percent);

/**
 * @brief 读取当前触摸阈值百分比。
 *
 * @return 当前阈值百分比；初始化前返回 TOUCH_THRESHOLD_PERCENT。
 */
uint8_t Touch_GetThresholdPercent(void);
