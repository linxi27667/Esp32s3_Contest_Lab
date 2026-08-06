#include "config.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

/* 灯带句柄、动态颜色缓存、实际灯珠数量和全局亮度。 */
static led_strip_handle_t g_rgb_strip = NULL;
static rgb_color_t *g_rgb_pixels = NULL;
static uint32_t g_rgb_led_count = 0;
static uint8_t g_rgb_brightness = RGB_LED_DEFAULT_BRIGHTNESS;

/* 使用 0~255 全局亮度对单个颜色分量进行定点缩放并四舍五入。 */
static uint8_t RgbLed_Scale(uint8_t value)
{
    return (uint8_t)(((uint16_t)value * g_rgb_brightness + 127U) / 255U);
}

/* 把一个像素的原始颜色按当前亮度写入 led_strip 发送缓冲区。 */
static esp_err_t RgbLed_WritePixel(uint32_t index)
{
    const rgb_color_t *color = &g_rgb_pixels[index];
    return led_strip_set_pixel(
        g_rgb_strip,
        index,
        RgbLed_Scale(color->red),
        RgbLed_Scale(color->green),
        RgbLed_Scale(color->blue));
}

/* 创建指定数量的 WS2812 RMT 设备，并动态分配原始颜色缓存。 */
esp_err_t RgbLed_Init(uint32_t led_count)
{
    if (g_rgb_strip != NULL || g_rgb_pixels != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (led_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    g_rgb_pixels = calloc(led_count, sizeof(*g_rgb_pixels));
    if (g_rgb_pixels == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_PIN,
        .max_leds = led_count,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };
    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RGB_LED_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 0,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(
        &strip_config,
        &rmt_config,
        &g_rgb_strip);
    if (err != ESP_OK) {
        free(g_rgb_pixels);
        g_rgb_pixels = NULL;
        return err;
    }

    g_rgb_led_count = led_count;
    return RgbLed_Clear();
}

/* 删除 RMT 灯带设备并释放动态颜色缓存。 */
esp_err_t RgbLed_Deinit(void)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = led_strip_del(g_rgb_strip);
    if (err != ESP_OK) {
        return err;
    }

    free(g_rgb_pixels);
    g_rgb_pixels = NULL;
    g_rgb_strip = NULL;
    g_rgb_led_count = 0;
    return ESP_OK;
}

/* 更新单个像素的原始缓存和发送缓冲区，但不立即触发物理发送。 */
esp_err_t RgbLed_SetPixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (index >= g_rgb_led_count) {
        return ESP_ERR_INVALID_ARG;
    }

    g_rgb_pixels[index] = (rgb_color_t) {
        .red = red,
        .green = green,
        .blue = blue,
    };
    return RgbLed_WritePixel(index);
}

/* 将 HSV 转换成 RGB，再复用普通像素接口保存原始颜色。 */
esp_err_t RgbLed_SetPixelHsv(
    uint32_t index,
    uint16_t hue,
    uint8_t saturation,
    uint8_t value)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (index >= g_rgb_led_count) {
        return ESP_ERR_INVALID_ARG;
    }

    hue %= 360U;
    uint32_t rgb_max = value;
    uint32_t rgb_min = rgb_max * (255U - saturation) / 255U;
    uint32_t sector = hue / 60U;
    uint32_t diff = hue % 60U;
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60U;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;

    switch (sector) {
    case 0:
        red = rgb_max; green = rgb_min + rgb_adj; blue = rgb_min;
        break;
    case 1:
        red = rgb_max - rgb_adj; green = rgb_max; blue = rgb_min;
        break;
    case 2:
        red = rgb_min; green = rgb_max; blue = rgb_min + rgb_adj;
        break;
    case 3:
        red = rgb_min; green = rgb_max - rgb_adj; blue = rgb_max;
        break;
    case 4:
        red = rgb_min + rgb_adj; green = rgb_min; blue = rgb_max;
        break;
    default:
        red = rgb_max; green = rgb_min; blue = rgb_max - rgb_adj;
        break;
    }

    return RgbLed_SetPixel(
        index,
        (uint8_t)red,
        (uint8_t)green,
        (uint8_t)blue);
}

/* 批量使用同一个 HSV 颜色更新全部像素，但不立即触发物理发送。 */
esp_err_t RgbLed_FillHsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    for (uint32_t i = 0; i < g_rgb_led_count; ++i) {
        esp_err_t err = RgbLed_SetPixelHsv(i, hue, saturation, value);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

/* 批量把全部像素设置为同一原始颜色，但不立即触发物理发送。 */
esp_err_t RgbLed_Fill(uint8_t red, uint8_t green, uint8_t blue)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    for (uint32_t i = 0; i < g_rgb_led_count; ++i) {
        esp_err_t err = RgbLed_SetPixel(i, red, green, blue);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

/* 生成一帧均匀覆盖 360 度色相环的彩虹，并立即刷新灯带。 */
esp_err_t RgbLed_ShowRainbow(uint16_t hue_offset)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL || g_rgb_led_count == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    for (uint32_t i = 0; i < g_rgb_led_count; ++i) {
        uint16_t hue = (uint16_t)(
            (hue_offset + (i * 360U / g_rgb_led_count)) % 360U);
        esp_err_t err = RgbLed_SetPixelHsv(i, hue, 255, 255);
        if (err != ESP_OK) {
            return err;
        }
    }
    return RgbLed_Refresh();
}

/* 按固定色相步长不断生成彩虹帧；0 次表示一直运行。 */
esp_err_t RgbLed_RainbowFlow(uint32_t frame_delay_ms, uint32_t cycle_count)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (frame_delay_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t completed_cycles = 0;
    uint16_t hue_offset = 0;
    while (cycle_count == 0 || completed_cycles < cycle_count) {
        esp_err_t err = RgbLed_ShowRainbow(hue_offset);
        if (err != ESP_OK) {
            return err;
        }

        hue_offset = (uint16_t)(
            (hue_offset + RGB_LED_RAINBOW_STEP_DEGREES) % 360U);
        if (hue_offset == 0) {
            ++completed_cycles;
        }
        vTaskDelay(pdMS_TO_TICKS(frame_delay_ms));
    }
    return ESP_OK;
}

/* 通过 RMT 把当前发送缓冲区同步到物理灯带。 */
esp_err_t RgbLed_Refresh(void)
{
    if (g_rgb_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return led_strip_refresh(g_rgb_strip);
}

/* 同时清空原始颜色缓存、驱动发送缓冲区和物理灯带。 */
esp_err_t RgbLed_Clear(void)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    for (uint32_t i = 0; i < g_rgb_led_count; ++i) {
        g_rgb_pixels[i] = (rgb_color_t) {0};
    }
    return led_strip_clear(g_rgb_strip);
}

/* 修改亮度后从原始颜色缓存重新计算全部像素，并立即刷新。 */
esp_err_t RgbLed_SetBrightness(uint8_t brightness)
{
    if (g_rgb_strip == NULL || g_rgb_pixels == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    g_rgb_brightness = brightness;
    for (uint32_t i = 0; i < g_rgb_led_count; ++i) {
        esp_err_t err = RgbLed_WritePixel(i);
        if (err != ESP_OK) {
            return err;
        }
    }
    return RgbLed_Refresh();
}

/* 返回当前亮度系数；读取操作不依赖驱动初始化状态。 */
uint8_t RgbLed_GetBrightness(void)
{
    return g_rgb_brightness;
}
