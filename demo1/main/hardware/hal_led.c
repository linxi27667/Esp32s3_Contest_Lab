#include "config.h"

/* LED编号与GPIO引脚的对应表。 */
static const gpio_num_t g_led_pins[LED_ID_COUNT] = {
    LED1_PIN,
    LED2_PIN,
    LED3_PIN,
    LED4_PIN,
};

/* 按位图批量初始化GPIO为推挽输出。 */
static esp_err_t Gpio_Init(uint64_t pin_mask)
{
    /* 一次性把位图中的所有GPIO配置成推挽输出。 */
    const gpio_config_t config = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&config);
}

/* 初始化四个LED。 */
esp_err_t Led_Init(void)
{
    /* 配置四个LED，并让它们初始熄灭。 */
    esp_err_t err = Gpio_Init(LED_ALL_PIN_MASK);
    if (err != ESP_OK) {
        return err;
    }

    return Led_All_Set(false);
}

/* 设置单个LED的状态。 */
esp_err_t Led_Set(led_id_t led, bool on)
{
    /* 防止数组下标越界。 */
    if (led < 0 || led >= LED_ID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    return gpio_set_level(g_led_pins[led], on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

/* 同时设置四个LED的状态。 */
esp_err_t Led_All_Set(bool on)
{
    /* 逐个设置四个LED。 */
    for (int led = 0; led < LED_ID_COUNT; ++led) {
        esp_err_t err = Led_Set((led_id_t)led, on);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}
