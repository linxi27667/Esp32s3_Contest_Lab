#include "config.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
    gpio_num_t pin;
    uint8_t active_level;
    bool enabled;
    bool initialized;
} key_slot_t;

/* 每个 KEY_ID 对应一个可独立配置的 GPIO 按键资源。 */
static key_slot_t g_keys[KEY_ID_COUNT] = {
    [KEY_ID_0] = {
        .pin = KEY_ID_0_PIN,
        .active_level = KEY_ID_0_ACTIVE_LEVEL,
        .enabled = KEY_ID_0_ENABLED,
    },
    [KEY_ID_1] = {
        .pin = KEY_ID_1_PIN,
        .active_level = KEY_ID_1_ACTIVE_LEVEL,
        .enabled = KEY_ID_1_ENABLED,
    },
    [KEY_ID_2] = {
        .pin = KEY_ID_2_PIN,
        .active_level = KEY_ID_2_ACTIVE_LEVEL,
        .enabled = KEY_ID_2_ENABLED,
    },
    [KEY_ID_3] = {
        .pin = KEY_ID_3_PIN,
        .active_level = KEY_ID_3_ACTIVE_LEVEL,
        .enabled = KEY_ID_3_ENABLED,
    },
};

/* 中断和任务之间通过队列传递按键事件。 */
static QueueHandle_t g_key_event_queue = NULL;
static bool g_key_initialized = false;
static bool g_key_scan_pressed[KEY_ID_COUNT];

static bool Key_Id_IsValid(key_id_t id)
{
    return id >= KEY_ID_0 && id < KEY_ID_COUNT && g_keys[id].enabled;
}

/* 按键 GPIO 中断回调：只采集边沿和来源，不在 ISR 中做业务逻辑。 */
static void IRAM_ATTR Key_Isr_Handler(void *argument)
{
    const key_id_t id = (key_id_t)(uintptr_t)argument;
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (!Key_Id_IsValid(id) || g_key_event_queue == NULL) {
        return;
    }

    const bool pressed =
        gpio_get_level(g_keys[id].pin) == g_keys[id].active_level;
    const key_event_t event = {
        .id = id,
        .type = pressed ? KEY_EVENT_PRESSED : KEY_EVENT_RELEASED,
        .pin = g_keys[id].pin,
    };

    xQueueSendFromISR(
        g_key_event_queue,
        &event,
        &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static esp_err_t Key_Gpio_Init(key_id_t id, gpio_int_type_t interrupt_type)
{
    if (!Key_Id_IsValid(id)) {
        return ESP_ERR_INVALID_ARG;
    }

    const key_slot_t *slot = &g_keys[id];
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << slot->pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = slot->active_level == 0
            ? GPIO_PULLUP_ENABLE
            : GPIO_PULLUP_DISABLE,
        .pull_down_en = slot->active_level == 1
            ? GPIO_PULLDOWN_ENABLE
            : GPIO_PULLDOWN_DISABLE,
        .intr_type = interrupt_type,
    };

    return gpio_config(&config);
}

esp_err_t Key_Init(void)
{
    if (g_key_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    bool any_enabled = false;
    for (int i = 0; i < KEY_ID_COUNT; ++i) {
        if (!g_keys[i].enabled) {
            continue;
        }

        esp_err_t err = Key_Gpio_Init((key_id_t)i, GPIO_INTR_DISABLE);
        if (err != ESP_OK) {
            return err;
        }

        g_keys[i].initialized = true;
        any_enabled = true;
    }

    if (!any_enabled) {
        return ESP_ERR_INVALID_ARG;
    }

    g_key_initialized = true;
    return ESP_OK;
}

esp_err_t Key_Interrupt_Init(void)
{
    if (g_key_event_queue != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = Key_Init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    g_key_event_queue = xQueueCreate(KEY_QUEUE_LENGTH, sizeof(key_event_t));
    if (g_key_event_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        vQueueDelete(g_key_event_queue);
        g_key_event_queue = NULL;
        return err;
    }

    for (int i = 0; i < KEY_ID_COUNT; ++i) {
        if (!g_keys[i].enabled) {
            continue;
        }

        err = gpio_set_intr_type(g_keys[i].pin, GPIO_INTR_ANYEDGE);
        if (err != ESP_OK) {
            vQueueDelete(g_key_event_queue);
            g_key_event_queue = NULL;
            return err;
        }

        err = gpio_isr_handler_add(
            g_keys[i].pin,
            Key_Isr_Handler,
            (void *)(uintptr_t)i);
        if (err != ESP_OK) {
            vQueueDelete(g_key_event_queue);
            g_key_event_queue = NULL;
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t Key_GetEvent(key_event_t *event, TickType_t timeout_ticks)
{
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_key_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return xQueueReceive(g_key_event_queue, event, timeout_ticks) == pdTRUE
        ? ESP_OK
        : ESP_ERR_TIMEOUT;
}

esp_err_t Key_IsPressed(key_id_t id, bool *pressed)
{
    if (pressed == NULL || !Key_Id_IsValid(id)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_key_initialized || !g_keys[id].initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    *pressed = gpio_get_level(g_keys[id].pin) == g_keys[id].active_level;
    return ESP_OK;
}

esp_err_t Key_Scan(key_id_t id, bool *pressed)
{
    if (pressed == NULL || !Key_Id_IsValid(id)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_key_initialized || !g_keys[id].initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    bool current_pressed = false;
    esp_err_t err = Key_IsPressed(id, &current_pressed);
    if (err != ESP_OK) {
        return err;
    }

    if (current_pressed != g_key_scan_pressed[id]) {
        vTaskDelay(pdMS_TO_TICKS(20));

        err = Key_IsPressed(id, &current_pressed);
        if (err != ESP_OK) {
            return err;
        }

        if (current_pressed != g_key_scan_pressed[id]) {
            g_key_scan_pressed[id] = current_pressed;
        }
    }

    *pressed = g_key_scan_pressed[id];
    return ESP_OK;
}
