#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* 像 STM32 选 TIMx 一样，用 ID 选择硬件定时器。
 * ESP32-S3 共 4 路 GPTimer（2 组 × 每组 2 个），全部暴露给应用。 */
typedef enum {
    TIMER_ID_0 = 0, /**< 第1路 GPTimer。 */
    TIMER_ID_1,     /**< 第2路 GPTimer。 */
    TIMER_ID_2,     /**< 第3路 GPTimer。 */
    TIMER_ID_3,     /**< 第4路 GPTimer。 */
    TIMER_ID_COUNT,
} timer_id_t;

/* Timer events delivered to the application layer. */
typedef enum {
    TIMER_EVENT_NONE = 0,
    TIMER_EVENT_PERIODIC_TICK,
    TIMER_EVENT_BUZZER_STOP,
    TIMER_EVENT_BOOT_COMPLETE,
    TIMER_EVENT_LED_BLINK,
    TIMER_EVENT_RGB_REFRESH,
    TIMER_EVENT_COUNT
} timer_event_type_t;

typedef struct {
    timer_event_type_t type;
    timer_id_t id;     /**< 产生事件的定时器编号。 */
    uint64_t count_us; /**< 触发闹钟时的计数值，单位微秒。 */
    uint32_t period_ms;/**< 当前配置的周期或单次延时，单位毫秒。 */
} timer_event_t;

/**
 * @brief 初始化四路 GPTimer 和共享事件队列，但不启动计数。
 *
 * @retval ESP_OK 初始化成功。
 * @retval ESP_ERR_INVALID_STATE 定时器库已经初始化。
 * @retval ESP_ERR_NO_MEM 事件队列或 GPTimer 资源创建失败。
 * @retval 其他值 GPTimer 创建、回调注册或使能失败。
 */
esp_err_t Timer_Init(void);

/**
 * @brief 启动或重启一路周期定时器。
 *
 * @param[in] id 定时器编号，可填 TIMER_ID_0、TIMER_ID_1、TIMER_ID_2、TIMER_ID_3。
 * @param[in] period_ms 触发周期，单位毫秒，必须大于 0。例如 1000 表示每秒触发一次。
 *
 * @retval ESP_OK 启动成功。
 * @retval ESP_ERR_INVALID_ARG id 无效或 period_ms 为 0。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Timer_Init()。
 * @retval 其他值 GPTimer 停止、配置或启动失败。
 * @note 如果该定时器正在运行，本函数会先停止，再从 0 开始按新周期运行。
 */
esp_err_t Timer_Start_Periodic_Ms(
    timer_id_t id,
    timer_event_type_t type,
    uint32_t period_ms);

/**
 * @brief 启动或重启一路单次定时器，触发一次后自动停止。
 *
 * @param[in] id 定时器编号，可填 TIMER_ID_0、TIMER_ID_1、TIMER_ID_2、TIMER_ID_3。
 * @param[in] delay_ms 延时时间，单位毫秒，必须大于 0。例如 500 表示 500 ms 后触发。
 *
 * @retval ESP_OK 启动成功。
 * @retval ESP_ERR_INVALID_ARG id 无效或 delay_ms 为 0。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Timer_Init()。
 * @retval 其他值 GPTimer 停止、配置或启动失败。
 */
esp_err_t Timer_Start_OneShot_Ms(
    timer_id_t id,
    timer_event_type_t type,
    uint32_t delay_ms);

/**
 * @brief 停止指定定时器，之后仍可重新启动。
 *
 * @param[in] id 定时器编号，可填 TIMER_ID_0、TIMER_ID_1、TIMER_ID_2、TIMER_ID_3。
 *
 * @retval ESP_OK 停止成功；定时器本来未运行时也返回 ESP_OK。
 * @retval ESP_ERR_INVALID_ARG id 无效。
 * @retval ESP_ERR_INVALID_STATE 尚未初始化定时器库。
 * @retval 其他值 GPTimer 停止失败。
 */
esp_err_t Timer_Stop(timer_id_t id);

/**
 * @brief 在任务中等待任意一路定时器的闹钟事件。
 *
 * @param[out] event 接收事件的结构体地址，不能为 NULL；通过 event->id 区分来源。
 * @param[in] timeout_ticks 等待时间，单位为 FreeRTOS tick。可填 0、
 *                          pdMS_TO_TICKS(ms) 或 portMAX_DELAY。
 *
 * @retval ESP_OK 成功取得一个定时器事件。
 * @retval ESP_ERR_TIMEOUT 等待期间没有事件。
 * @retval ESP_ERR_INVALID_ARG event 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Timer_Init()。
 * @note 只能在任务上下文调用，不能在 ISR 中调用。
 */
esp_err_t Timer_GetEvent(timer_event_t *event, TickType_t timeout_ticks);

/**
 * @brief 读取指定定时器当前计数值。
 *
 * @param[in] id 定时器编号，可填 TIMER_ID_0、TIMER_ID_1、TIMER_ID_2、TIMER_ID_3。
 * @param[out] count_us 接收计数值的地址，单位微秒，不能为 NULL。
 *
 * @retval ESP_OK 读取成功。
 * @retval ESP_ERR_INVALID_ARG id 无效或 count_us 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未初始化定时器库。
 * @retval 其他值 GPTimer 读取失败。
 */
esp_err_t Timer_GetCount_Us(timer_id_t id, uint64_t *count_us);
