#include "config.h"

#include "driver/touch_sensor_legacy.h"

/* ESP32-S3 新版 Touch 驱动在部分 v0.2 芯片上会出现扫描超时，
 * 此处使用稳定的兼容驱动，上层 HAL 接口保持不变。 */
static bool g_touch_initialized = false;
static uint8_t g_touch_threshold_percent = TOUCH_THRESHOLD_PERCENT;

static touch_pad_t Touch_Channel(void)
{
    return (touch_pad_t)TOUCH_CHANNEL_ID;
}

/* 初始化中途失败时释放兼容驱动占用的硬件资源。 */
static void Touch_Cleanup(void)
{
    (void)touch_pad_fsm_stop();
    (void)touch_pad_filter_disable();
    (void)touch_pad_deinit();
    g_touch_initialized = false;
}

/* 根据当前基准值设置硬件触摸阈值。 */
static esp_err_t Touch_UpdateHardwareThreshold(uint8_t percent)
{
    uint32_t benchmark = 0;
    esp_err_t err = touch_pad_read_benchmark(Touch_Channel(), &benchmark);
    if (err != ESP_OK) {
        return err;
    }
    if (benchmark == 0 || benchmark == TOUCH_PAD_THRESHOLD_MAX) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    const uint32_t threshold = (benchmark * percent) / 100U;
    return touch_pad_set_thresh(Touch_Channel(), threshold);
}

esp_err_t Touch_Init(void)
{
    if (g_touch_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (TOUCH_CHANNEL_ID < TOUCH_PAD_NUM1 ||
        TOUCH_CHANNEL_ID >= TOUCH_PAD_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = touch_pad_init();
    if (err != ESP_OK) {
        return err;
    }

    err = touch_pad_config(Touch_Channel());
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    /* IIR 滤波和硬件去抖可降低导线及人体附近的瞬时干扰。 */
    const touch_filter_config_t filter_config = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 2,
        .noise_thr = 1,
        .jitter_step = 4,
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
    };
    err = touch_pad_filter_set_config(&filter_config);
    if (err == ESP_OK) {
        err = touch_pad_filter_enable();
    }
    if (err == ESP_OK) {
        err = touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    }
    if (err == ESP_OK) {
        err = touch_pad_fsm_start();
    }
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    /* 等待若干轮自动扫描，使 raw、smooth 和 benchmark 数据稳定。 */
    vTaskDelay(pdMS_TO_TICKS(TOUCH_INITIAL_STABILIZE_MS));

    err = Touch_UpdateHardwareThreshold(g_touch_threshold_percent);
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    g_touch_initialized = true;
    return ESP_OK;
}

esp_err_t Touch_Read(touch_sample_t *sample_out)
{
    if (sample_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_touch_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = touch_pad_read_raw_data(
        Touch_Channel(), &sample_out->raw);
    if (err == ESP_OK) {
        err = touch_pad_filter_read_smooth(
            Touch_Channel(), &sample_out->smooth);
    }
    if (err == ESP_OK) {
        err = touch_pad_read_benchmark(
            Touch_Channel(), &sample_out->benchmark);
    }
    return err;
}

esp_err_t Touch_IsPressed(bool *pressed_out)
{
    if (pressed_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    touch_sample_t sample;
    esp_err_t err = Touch_Read(&sample);
    if (err != ESP_OK) {
        return err;
    }

    const uint32_t threshold =
        (sample.benchmark * g_touch_threshold_percent) / 100U;
    *pressed_out = sample.smooth >= sample.benchmark + threshold;
    return ESP_OK;
}

esp_err_t Touch_SetThresholdPercent(uint8_t percent)
{
    if (percent == 0 || percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_touch_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = Touch_UpdateHardwareThreshold(percent);
    if (err == ESP_OK) {
        g_touch_threshold_percent = percent;
    }
    return err;
}

uint8_t Touch_GetThresholdPercent(void)
{
    return g_touch_threshold_percent;
}
