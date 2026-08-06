#include "config.h"

#include "driver/touch_sens.h"

/* 单通道触摸控制器、通道句柄和运行时灵敏度配置。 */
static touch_sensor_handle_t g_touch_sensor = NULL;
static touch_channel_handle_t g_touch_channel = NULL;
static uint8_t g_touch_threshold_percent = TOUCH_THRESHOLD_PERCENT;

/* 根据当前基准值和百分比生成 ESP32-S3 相对触发阈值。 */
static esp_err_t Touch_ReconfigureThreshold(uint32_t benchmark, uint8_t percent)
{
    if (percent == 0 || percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    const touch_channel_config_t channel_config = {
        .active_thresh = {(benchmark * percent) / 100U},
        .charge_speed = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };
    return touch_sensor_reconfig_channel(g_touch_channel, &channel_config);
}

/* 初始化中途失败时按“控制器、通道、驱动”的逆序释放资源。 */
static void Touch_Cleanup(void)
{
    if (g_touch_sensor != NULL) {
        (void)touch_sensor_disable(g_touch_sensor);
    }
    if (g_touch_channel != NULL) {
        (void)touch_sensor_del_channel(g_touch_channel);
        g_touch_channel = NULL;
    }
    if (g_touch_sensor != NULL) {
        (void)touch_sensor_del_controller(g_touch_sensor);
        g_touch_sensor = NULL;
    }
}

/* 创建触摸控制器和通道，完成初始校准后进入连续扫描状态。 */
esp_err_t Touch_Init(void)
{
    if (g_touch_sensor != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* ESP32-S3 使用 Touch V2：500 次充放电，电压范围 0.5~2.2 V。 */
    touch_sensor_sample_config_t sample_config[TOUCH_SAMPLE_CFG_NUM] = {
        TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(
            500,
            TOUCH_VOLT_LIM_L_0V5,
            TOUCH_VOLT_LIM_H_2V2),
    };
    const touch_sensor_config_t sensor_config =
        TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(TOUCH_SAMPLE_CFG_NUM, sample_config);
    esp_err_t err = touch_sensor_new_controller(&sensor_config, &g_touch_sensor);
    if (err != ESP_OK) {
        return err;
    }

    /* 初始阈值只用于建立通道；完成基准扫描后会按百分比重新配置。 */
    const touch_channel_config_t channel_config = {
        .active_thresh = {2000},
        .charge_speed = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };
    err = touch_sensor_new_channel(
        g_touch_sensor,
        TOUCH_CHANNEL_ID,
        &channel_config,
        &g_touch_channel);
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    /* 启用基准 IIR、数据平滑、迟滞和硬件消抖。 */
    const touch_sensor_filter_config_t filter_config =
        TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    err = touch_sensor_config_filter(g_touch_sensor, &filter_config);
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    /* 多次单次扫描用于消除上电初期波动并建立稳定 benchmark。 */
    err = touch_sensor_enable(g_touch_sensor);
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }
    for (uint32_t i = 0; i < TOUCH_INITIAL_SCAN_TIMES; ++i) {
        err = touch_sensor_trigger_oneshot_scanning(
            g_touch_sensor,
            TOUCH_SCAN_TIMEOUT_MS);
        if (err != ESP_OK) {
            Touch_Cleanup();
            return err;
        }
    }

    /* 读取校准后的动态基准，用它计算与现场电极匹配的相对阈值。 */
    uint32_t benchmark[TOUCH_SAMPLE_CFG_NUM] = {0};
    err = touch_channel_read_data(
        g_touch_channel,
        TOUCH_CHAN_DATA_TYPE_BENCHMARK,
        benchmark);
    if (err != ESP_OK) {
        Touch_Cleanup();
        return err;
    }

    /* 通道重配置要求控制器处于禁用状态，配置后再恢复连续扫描。 */
    err = touch_sensor_disable(g_touch_sensor);
    if (err == ESP_OK) {
        err = Touch_ReconfigureThreshold(benchmark[0], g_touch_threshold_percent);
    }
    if (err == ESP_OK) {
        err = touch_sensor_enable(g_touch_sensor);
    }
    if (err == ESP_OK) {
        err = touch_sensor_start_continuous_scanning(g_touch_sensor);
    }
    if (err != ESP_OK) {
        Touch_Cleanup();
    }
    return err;
}

/* 依次读取原始值、平滑值和动态基准值，组合成一次完整采样。 */
esp_err_t Touch_Read(touch_sample_t *sample_out)
{
    if (sample_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_touch_sensor == NULL || g_touch_channel == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t data[TOUCH_SAMPLE_CFG_NUM] = {0};
    /* RAW 便于现场观察噪声与触摸前后的绝对变化。 */
    esp_err_t err = touch_channel_read_data(
        g_touch_channel,
        TOUCH_CHAN_DATA_TYPE_RAW,
        data);
    if (err != ESP_OK) {
        return err;
    }
    sample_out->raw = data[0];

    /* SMOOTH 是经过驱动滤波的数据，应用层判断优先使用该值。 */
    err = touch_channel_read_data(
        g_touch_channel,
        TOUCH_CHAN_DATA_TYPE_SMOOTH,
        data);
    if (err != ESP_OK) {
        return err;
    }
    sample_out->smooth = data[0];

    /* BENCHMARK 会随环境缓慢漂移，用于抵消温湿度和器件差异。 */
    err = touch_channel_read_data(
        g_touch_channel,
        TOUCH_CHAN_DATA_TYPE_BENCHMARK,
        data);
    if (err == ESP_OK) {
        sample_out->benchmark = data[0];
    }
    return err;
}

/* 使用平滑值相对动态基准的增量判断当前触摸状态。 */
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

/* 停止扫描后更新硬件阈值，再恢复到连续扫描状态。 */
esp_err_t Touch_SetThresholdPercent(uint8_t percent)
{
    if (percent == 0 || percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_touch_sensor == NULL || g_touch_channel == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    touch_sample_t sample;
    esp_err_t err = Touch_Read(&sample);
    if (err != ESP_OK) {
        return err;
    }
    /* touch_sensor_reconfig_channel() 只能在控制器禁用时调用。 */
    err = touch_sensor_stop_continuous_scanning(g_touch_sensor);
    if (err != ESP_OK) {
        return err;
    }
    err = touch_sensor_disable(g_touch_sensor);
    if (err == ESP_OK) {
        err = Touch_ReconfigureThreshold(sample.benchmark, percent);
    }
    if (err == ESP_OK) {
        g_touch_threshold_percent = percent;
    }

    /* 即使重配置失败也尝试恢复扫描，避免触摸外设一直停机。 */
    esp_err_t restart_err = touch_sensor_enable(g_touch_sensor);
    if (restart_err == ESP_OK) {
        restart_err = touch_sensor_start_continuous_scanning(g_touch_sensor);
    }
    return err != ESP_OK ? err : restart_err;
}

/* 返回当前软件判定和硬件触发共同使用的阈值百分比。 */
uint8_t Touch_GetThresholdPercent(void)
{
    return g_touch_threshold_percent;
}
