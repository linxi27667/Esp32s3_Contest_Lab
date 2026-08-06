#include "config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

/* LEDC settings are implementation details of the passive-buzzer driver. */
#define BUZZER_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_TIMER LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define BUZZER_LEDC_INIT_FREQUENCY_HZ 2000
#define BUZZER_LEDC_DUTY 512

/* 短鸣提示音使用独立软件定时器，不占用应用层的四路 GPTimer。 */
static esp_timer_handle_t g_buzzer_stop_timer = NULL;

/* esp_timer 回调运行在系统定时器任务中，可以调用普通任务上下文 API。 */
static void Buzzer_Stop_Timer_Callback(void *argument)
{
    (void)argument;
    (void)Buzzer_Passive_Stop();
}

/* 初始化有源蜂鸣器输出GPIO。 */
static esp_err_t Active_Buzzer_Gpio_Init(void)
{
    /* 有源蜂鸣器只需要配置一个普通输出GPIO。 */
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << ACTIVE_BUZZER_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&config);
}

/* 初始化无源蜂鸣器的LEDC定时器与通道。 */
static esp_err_t Passive_Buzzer_Ledc_Init(void)
{
    /* 先配置LEDC定时器：决定PWM频率和占空比分辨率。 */
    const ledc_timer_config_t timer_config = {
        .speed_mode = BUZZER_LEDC_MODE,
        .duty_resolution = BUZZER_LEDC_DUTY_RESOLUTION,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = BUZZER_LEDC_INIT_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        return err;
    }

    const ledc_channel_config_t channel_config = {
        /* 再配置LEDC通道，把PWM连接到无源蜂鸣器引脚。 */
        .gpio_num = PASSIVE_BUZZER_PIN,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };

    return ledc_channel_config(&channel_config);
}

/* 初始化有源与无源蜂鸣器，初始均为关闭。 */
esp_err_t Buzzer_Init(void)
{
    /* 同时初始化有源和无源蜂鸣器，初始都关闭。 */
    esp_err_t err = Active_Buzzer_Gpio_Init();
    if (err != ESP_OK) {
        return err;
    }

    err = Passive_Buzzer_Ledc_Init();
    if (err != ESP_OK) {
        return err;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = Buzzer_Stop_Timer_Callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "buzzer_stop",
        .skip_unhandled_events = true,
    };

    err = esp_timer_create(&timer_args, &g_buzzer_stop_timer);
    if (err != ESP_OK) {
        return err;
    }

    err = Buzzer_Active_Set(false);
    if (err != ESP_OK) {
        return err;
    }

    return Buzzer_Passive_Stop();
}

/* 有源蜂鸣器：GPIO电平控制内部振荡器。 */
esp_err_t Buzzer_Active_Set(bool on)
{
    /* 有源蜂鸣器内部自带振荡器，GPIO输出有效电平即可发声。 */
    return gpio_set_level(
        ACTIVE_BUZZER_PIN,
        on ? ACTIVE_BUZZER_ON_LEVEL : ACTIVE_BUZZER_OFF_LEVEL);
}

/* 无源蜂鸣器：LEDC产生指定频率。 */
esp_err_t Buzzer_Passive_Play(uint32_t frequency_hz)
{
    /* frequency_hz是声音频率，单位为Hz。 */
    if (frequency_hz == 0) {
        /* 0Hz不是有效的播放频率。 */
        return ESP_ERR_INVALID_ARG;
    }

    /* 修改LEDC输出频率；返回0表示频率配置失败。 */
    uint32_t actual_frequency = ledc_set_freq(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_TIMER,
        frequency_hz);
    if (actual_frequency == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 设置50%左右的占空比，形成适合蜂鸣器的方波。 */
    esp_err_t err = ledc_set_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL,
        BUZZER_LEDC_DUTY);
    if (err != ESP_OK) {
        return err;
    }

    /* 让新的占空比立即写入LEDC硬件。 */
    return ledc_update_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL);
}

/* 停止无源蜂鸣器输出。 */
esp_err_t Buzzer_Passive_Stop(void)
{
    /* 将占空比设为0，停止无源蜂鸣器发声。 */
    esp_err_t err = ledc_set_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL,
        0);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL);
}

/* 播放一次短提示音，停止时间由蜂鸣器库自己的 esp_timer 管理。 */
esp_err_t Buzzer_Beep_Once(uint32_t frequency_hz, uint32_t duration_ms)
{
    if (frequency_hz == 0 || duration_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_buzzer_stop_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = Buzzer_Passive_Play(frequency_hz);
    if (err != ESP_OK) {
        return err;
    }

    if (esp_timer_is_active(g_buzzer_stop_timer)) {
        err = esp_timer_stop(g_buzzer_stop_timer);
        if (err != ESP_OK) {
            (void)Buzzer_Passive_Stop();
            return err;
        }
    }

    err = esp_timer_start_once(g_buzzer_stop_timer, (uint64_t)duration_ms * 1000ULL);
    if (err != ESP_OK) {
        /* 自动停止计时失败时立刻静音，避免蜂鸣器持续发声。 */
        (void)Buzzer_Passive_Stop();
    }

    return err;
}
