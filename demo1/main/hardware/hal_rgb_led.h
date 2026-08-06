#pragma once

#include <stdint.h>

#include "esp_err.h"

/** @brief 一个 RGB 像素的原始颜色，三个分量范围均为 0~255。 */
typedef struct {
    uint8_t red;   /**< 红色分量。 */
    uint8_t green; /**< 绿色分量。 */
    uint8_t blue;  /**< 蓝色分量。 */
} rgb_color_t;

/**
 * @brief 初始化 WS2812 灯带的 RMT 驱动，并将全部像素熄灭。
 *
 * 数据引脚、RMT 分辨率和默认亮度由 config.h 配置，灯珠数量由参数传入。
 *
 * @param[in] led_count 灯珠数量，必须大于 0。
 *
 * @retval ESP_OK 初始化成功，灯带已清空。
 * @retval ESP_ERR_INVALID_STATE 灯带已经初始化。
 * @retval 其他值 RMT 通道创建或灯带清空失败。
 */
esp_err_t RgbLed_Init(uint32_t led_count);

/**
 * @brief 释放 RMT 灯带驱动和颜色缓存。
 *
 * 释放后可以再次调用 RgbLed_Init()，并传入不同的灯珠数量。
 *
 * @retval ESP_OK 释放成功。
 * @retval ESP_ERR_INVALID_STATE 灯带尚未初始化。
 * @retval 其他值 删除底层灯带设备失败。
 */
esp_err_t RgbLed_Deinit(void);

/**
 * @brief 修改一个像素的缓存颜色，并写入 led_strip 发送缓冲区。
 *
 * 颜色分量会自动乘以当前全局亮度。调用后还需调用 RgbLed_Refresh()，
 * 才会把新颜色发送到灯带。
 *
 * @param[in] index 像素下标，范围为 0 ~ 初始化时灯珠数量-1。
 * @param[in] red 红色分量，范围 0~255。
 * @param[in] green 绿色分量，范围 0~255。
 * @param[in] blue 蓝色分量，范围 0~255。
 *
 * @retval ESP_OK 缓存和发送缓冲区更新成功。
 * @retval ESP_ERR_INVALID_ARG index 越界。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 led_strip 缓冲区更新失败。
 */
esp_err_t RgbLed_SetPixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 使用 HSV 颜色模型设置一个像素的缓存颜色。
 *
 * hue 为色相角度，0/120/240 分别接近红/绿/蓝；saturation 和 value 范围为 0~255。
 * 调用后还需调用 RgbLed_Refresh() 才会发送到灯带。
 *
 * @param[in] index 像素下标，范围为 0 ~ 初始化时灯珠数量-1。
 * @param[in] hue 色相角度，范围为 0~359，超过 359 时按 360 取模。
 * @param[in] saturation 饱和度，0 表示灰度，255 表示最鲜艳。
 * @param[in] value 颜色明度，0 表示熄灭，255 表示最大值。
 *
 * @retval ESP_OK 颜色缓存更新成功。
 * @retval ESP_ERR_INVALID_ARG index 越界。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 */
esp_err_t RgbLed_SetPixelHsv(uint32_t index, uint16_t hue, uint8_t saturation, uint8_t value);

/**
 * @brief 使用同一个 HSV 颜色设置全部像素的缓存颜色。
 *
 * 函数会将 HSV 颜色转换为 RGB 后写入全部灯珠，但不会立即发送到物理灯带。
 * 调用后还需调用 RgbLed_Refresh()，才会把新颜色发送到灯带。
 *
 * @param[in] hue 色相角度，范围为 0~359，超过 359 时按 360 取模。
 * @param[in] saturation 饱和度，0 表示灰度，255 表示最鲜艳。
 * @param[in] value 颜色明度，0 表示熄灭，255 表示最大值。
 * @retval ESP_OK 全部像素的颜色缓存更新成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 任意像素更新失败。
 */
esp_err_t RgbLed_FillHsv(uint16_t hue, uint8_t saturation, uint8_t value);

/**
 * @brief 把全部像素的缓存颜色设置为同一个值。
 *
 * 调用后还需调用 RgbLed_Refresh()，才会把颜色发送到灯带。
 *
 * @param[in] red 红色分量，范围 0~255。
 * @param[in] green 绿色分量，范围 0~255。
 * @param[in] blue 蓝色分量，范围 0~255。
 *
 * @retval ESP_OK 全部像素设置成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 任意像素更新失败。
 */
esp_err_t RgbLed_Fill(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 生成一帧沿灯带均匀分布的彩虹并立即刷新。
 *
 * @param[in] hue_offset 整条彩虹的色相偏移，范围 0~359，超过 359 时按 360 取模。
 *
 * @retval ESP_OK 彩虹帧生成并发送成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 像素设置或 RMT 发送失败。
 */
esp_err_t RgbLed_ShowRainbow(uint16_t hue_offset);

/**
 * @brief 直接运行流水彩虹动画。
 *
 * 本函数会阻塞当前任务，每帧之间调用 vTaskDelay() 让出 CPU。
 * cycle_count 为 0 时无限循环，适合在独立的 FreeRTOS 任务中调用。
 *
 * @param[in] frame_delay_ms 帧间隔，必须大于 0；越小动画越快。
 * @param[in] cycle_count 彩虹完整循环次数，0 表示无限循环。
 *
 * @retval ESP_OK 完成指定次数的动画。
 * @retval ESP_ERR_INVALID_ARG frame_delay_ms 为 0。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 彩虹帧发送失败。
 */
esp_err_t RgbLed_RainbowFlow(uint32_t frame_delay_ms, uint32_t cycle_count);

/**
 * @brief 将当前发送缓冲区同步到物理灯带。
 *
 * @retval ESP_OK 发送成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 RMT 发送失败。
 */
esp_err_t RgbLed_Refresh(void);

/**
 * @brief 清空颜色缓存并立即熄灭全部物理像素。
 *
 * 与 RgbLed_SetPixel() 和 RgbLed_Fill() 不同，本函数内部已经发送刷新数据。
 *
 * @retval ESP_OK 灯带已熄灭。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 led_strip 清空或发送失败。
 */
esp_err_t RgbLed_Clear(void);

/**
 * @brief 修改全局亮度，并按新亮度重新计算、刷新全部像素。
 *
 * 亮度只影响物理输出，不会破坏 g_rgb_pixels 中保存的原始颜色。
 *
 * @param[in] brightness 亮度系数，0 表示熄灭，255 表示不缩放。
 *
 * @retval ESP_OK 亮度修改并刷新成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 RgbLed_Init()。
 * @retval 其他值 像素更新或 RMT 发送失败。
 */
esp_err_t RgbLed_SetBrightness(uint8_t brightness);

/**
 * @brief 读取当前全局亮度系数。
 *
 * @return 当前亮度，范围 0~255；初始化前返回 RGB_LED_DEFAULT_BRIGHTNESS。
 */
uint8_t RgbLed_GetBrightness(void);
