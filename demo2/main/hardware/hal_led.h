#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    LED_ID_1 = 0, /**< 第1路 LED，对应 config.h 中的 LED1_PIN。 */
    LED_ID_2,     /**< 第2路 LED，对应 config.h 中的 LED2_PIN。 */
    LED_ID_3,     /**< 第3路 LED，对应 config.h 中的 LED3_PIN。 */
    LED_ID_4,     /**< 第4路 LED，对应 config.h 中的 LED4_PIN。 */
    LED_ID_COUNT,
} led_id_t;

/**
 * @brief 初始化全部 LED GPIO，并将所有 LED 设置为熄灭状态。
 *
 * LED 引脚和有效电平由 config.h 中的 LEDx_PIN、LED_ON_LEVEL 配置。
 *
 * @retval ESP_OK 初始化成功。
 * @retval 其他值 GPIO 配置或输出电平设置失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Led_Init(void);

/**
 * @brief 设置指定 LED 的亮灭状态。
 *
 * @param[in] led LED 编号，可填 LED_ID_1、LED_ID_2、LED_ID_3、LED_ID_4。
 * @param[in] on  true 表示点亮，false 表示熄灭。
 *
 * @retval ESP_OK 设置成功。
 * @retval ESP_ERR_INVALID_ARG led 超出有效范围。
 * @retval 其他值 GPIO 输出失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Led_Set(led_id_t led, bool on);

/**
 * @brief 同时设置全部 LED 的亮灭状态。
 *
 * @param[in] on true 表示全部点亮，false 表示全部熄灭。
 *
 * @retval ESP_OK 设置成功。
 * @retval 其他值 任一路 GPIO 输出失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Led_All_Set(bool on);
