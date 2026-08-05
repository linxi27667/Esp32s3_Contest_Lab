#include "config.h"

#include <stdint.h>

#include "driver/gptimer.h"
#include "esp_attr.h"
#include "freertos/queue.h"

/* 1MHz → 1 tick = 1µs，period_ms * 1000 即为 alarm 计数值。 */
#define TIMER_RESOLUTION_HZ 1000000

typedef struct {
    gptimer_handle_t handle;
    uint32_t period_ms;
    bool oneshot;
    bool running;
} timer_slot_t;

static timer_slot_t g_timers[TIMER_ID_COUNT];
static QueueHandle_t g_timer_event_queue = NULL;
static bool g_driver_ready = false;

/* GPTimer闹钟回调：单次模式停表，并将带 id 的事件入队。 */
static bool IRAM_ATTR Timer_Isr_Handler(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    const timer_id_t id = (timer_id_t)(uintptr_t)user_ctx;
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (id < 0 || id >= TIMER_ID_COUNT) {
        return false;
    }

    timer_slot_t *slot = &g_timers[id];

    if (slot->oneshot) {
        /* 单次模式：回调里停表，避免继续空转。 */
        gptimer_stop(timer);
        slot->running = false;
    }

    if (g_timer_event_queue != NULL) {
        const timer_event_t event = {
            .id = id,
            .count_us = edata->count_value,
            .period_ms = slot->period_ms,
        };
        xQueueSendFromISR(
            g_timer_event_queue,
            &event,
            &higher_priority_task_woken);
    }

    /* GPTimer 用返回值请求切换，不用 portYIELD_FROM_ISR。 */
    return higher_priority_task_woken == pdTRUE;
}

/* 配置指定定时器闹钟并启动；oneshot为true表示单次，false表示周期。 */
static esp_err_t Timer_Start_Internal(
    timer_id_t id,
    uint32_t period_ms,
    bool oneshot)
{
    if (!g_driver_ready || g_timer_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (id < 0 || id >= TIMER_ID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (period_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    timer_slot_t *slot = &g_timers[id];
    if (slot->handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (slot->running) {
        esp_err_t err = gptimer_stop(slot->handle);
        if (err != ESP_OK) {
            return err;
        }
        slot->running = false;
    }

    slot->period_ms = period_ms;
    slot->oneshot = oneshot;

    const gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = (uint64_t)period_ms * 1000ULL,
        .flags.auto_reload_on_alarm = !oneshot,
    };

    esp_err_t err = gptimer_set_alarm_action(slot->handle, &alarm_config);
    if (err != ESP_OK) {
        return err;
    }

    err = gptimer_set_raw_count(slot->handle, 0);
    if (err != ESP_OK) {
        return err;
    }

    err = gptimer_start(slot->handle);
    if (err != ESP_OK) {
        return err;
    }

    slot->running = true;
    return ESP_OK;
}

/* 失败时回收已创建的定时器资源。 */
static void Timer_Deinit_All(void)
{
    for (int i = 0; i < TIMER_ID_COUNT; ++i) {
        if (g_timers[i].handle != NULL) {
            if (g_timers[i].running) {
                gptimer_stop(g_timers[i].handle);
                g_timers[i].running = false;
            }
            gptimer_disable(g_timers[i].handle);
            gptimer_del_timer(g_timers[i].handle);
            g_timers[i].handle = NULL;
        }
    }

    if (g_timer_event_queue != NULL) {
        vQueueDelete(g_timer_event_queue);
        g_timer_event_queue = NULL;
    }

    g_driver_ready = false;
}

/* 初始化全部 GPTimer 与共享事件队列；此时尚未开始计数。 */
esp_err_t Timer_Init(void)
{
    if (g_driver_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    g_timer_event_queue = xQueueCreate(
        TIMER_QUEUE_LENGTH,
        sizeof(timer_event_t));
    if (g_timer_event_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };

    for (int i = 0; i < TIMER_ID_COUNT; ++i) {
        esp_err_t err = gptimer_new_timer(&timer_config, &g_timers[i].handle);
        if (err != ESP_OK) {
            Timer_Deinit_All();
            return err;
        }

        const gptimer_event_callbacks_t callbacks = {
            .on_alarm = Timer_Isr_Handler,
        };
        /* user_ctx 传入定时器 ID，回调里区分来源。 */
        err = gptimer_register_event_callbacks(
            g_timers[i].handle,
            &callbacks,
            (void *)(uintptr_t)i);
        if (err != ESP_OK) {
            Timer_Deinit_All();
            return err;
        }

        err = gptimer_enable(g_timers[i].handle);
        if (err != ESP_OK) {
            Timer_Deinit_All();
            return err;
        }

        g_timers[i].period_ms = 0;
        g_timers[i].oneshot = false;
        g_timers[i].running = false;
    }

    g_driver_ready = true;
    return ESP_OK;
}

/* 周期闹钟：指定定时器每 period_ms 毫秒触发一次，ISR入队。 */
esp_err_t Timer_Start_Periodic_Ms(timer_id_t id, uint32_t period_ms)
{
    return Timer_Start_Internal(id, period_ms, false);
}

/* 单次闹钟：指定定时器 delay_ms 毫秒后触发一次，ISR入队后自动停止。 */
esp_err_t Timer_Start_OneShot_Ms(timer_id_t id, uint32_t delay_ms)
{
    return Timer_Start_Internal(id, delay_ms, true);
}

/* 停止指定定时器计数；可再次 Start。 */
esp_err_t Timer_Stop(timer_id_t id)
{
    if (!g_driver_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (id < 0 || id >= TIMER_ID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    timer_slot_t *slot = &g_timers[id];
    if (slot->handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!slot->running) {
        return ESP_OK;
    }

    esp_err_t err = gptimer_stop(slot->handle);
    if (err != ESP_OK) {
        return err;
    }

    slot->running = false;
    return ESP_OK;
}

/* 在任务中等待任一定时器事件；用 event->id 区分来源。 */
esp_err_t Timer_GetEvent(timer_event_t *event, TickType_t timeout_ticks)
{
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_timer_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return xQueueReceive(
               g_timer_event_queue,
               event,
               timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

/* 读取指定定时器当前计数值，单位微秒（1MHz分辨率）。 */
esp_err_t Timer_GetCount_Us(timer_id_t id, uint64_t *count_us)
{
    if (count_us == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_driver_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (id < 0 || id >= TIMER_ID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_timers[id].handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return gptimer_get_raw_count(g_timers[id].handle, count_us);
}
