#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* KEY_ID_x 是按键资源编号，和 TIMER_ID_x 的使用方式一致。 */
typedef enum {
    KEY_ID_0 = 0, /**< 默认按键，对应 config.h 中的 KEY_ID_0_PIN。 */
    KEY_ID_1,     /**< 第二个按键资源。 */
    KEY_ID_2,     /**< 第三个按键资源。 */
    KEY_ID_3,     /**< 第四个按键资源。 */
    KEY_ID_COUNT,
} key_id_t;

/* 按键事件类型；可以在应用层按需继续扩展。 */
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESSED,
    KEY_EVENT_RELEASED,
    KEY_EVENT_COUNT,
} key_event_type_t;

typedef struct {
    key_id_t id;              /**< 产生事件的按键编号。 */
    key_event_type_t type;    /**< 按下或释放事件类型。 */
    gpio_num_t pin;           /**< 产生事件的 GPIO，便于日志和调试。 */
} key_event_t;

/**
 * @brief 初始化所有启用按键的 GPIO，供轮询读取或中断模式使用。
 *
 * 每个按键是否启用、GPIO 和有效电平由 config.h 中的 KEY_ID_x_* 宏决定。
 *
 * @retval ESP_OK 初始化成功。
 * @retval ESP_ERR_INVALID_STATE 按键 GPIO 已经初始化。
 * @retval 其他值 GPIO 配置失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Key_Init(void);

/**
 * @brief 初始化所有启用按键的 GPIO、双边沿中断和事件队列。
 *
 * 可以直接调用，无需先调用 Key_Init()。中断服务程序只发送事件，消抖应在任务中完成。
 *
 * @retval ESP_OK 初始化成功。
 * @retval ESP_ERR_INVALID_STATE 中断事件队列已经创建，即本函数已调用过。
 * @retval ESP_ERR_NO_MEM 事件队列创建失败。
 * @retval 其他值 GPIO 中断配置或 ISR 注册失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Key_Interrupt_Init(void);

/**
 * @brief 在任务中等待一个按键中断事件。
 *
 * @param[out] event 接收事件的结构体地址，不能为 NULL。成功后可通过
 *                   event->id 和 event->type 区分按键及其动作。
 * @param[in] timeout_ticks 等待时间，单位为 FreeRTOS tick。可填：
 *                          0 表示立即返回；
 *                          pdMS_TO_TICKS(ms) 表示等待指定毫秒；
 *                          portMAX_DELAY 表示一直等待。
 *
 * @retval ESP_OK 成功取得一个按键事件。
 * @retval ESP_ERR_TIMEOUT 等待期间没有收到事件。
 * @retval ESP_ERR_INVALID_ARG event 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Key_Interrupt_Init()。
 * @note 只能在任务上下文调用，不能在 ISR 中调用。
 */
esp_err_t Key_GetEvent(key_event_t *event, TickType_t timeout_ticks);

/**
 * @brief 立即读取按键当前电平并判断是否按下，不执行消抖。
 *
 * @param[out] pressed 输出按键状态，true 表示按下，false 表示松开，不能为 NULL。
 *
 * @retval ESP_OK 读取成功。
 * @retval ESP_ERR_INVALID_ARG pressed 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未初始化按键 GPIO。
 */
esp_err_t Key_IsPressed(key_id_t id, bool *pressed);

/**
 * @brief 轮询扫描按键，并在状态变化时进行 20 ms 软件消抖。
 *
 * @param[out] pressed 输出消抖后的稳定状态，true 表示按下，false 表示松开，不能为 NULL。
 *
 * @retval ESP_OK 扫描成功。
 * @retval ESP_ERR_INVALID_ARG pressed 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未初始化按键 GPIO。
 * @note 调用前先执行 Key_Init() 或 Key_Interrupt_Init()。本函数内部可能阻塞 20 ms，
 *       只能在任务中调用，不能在 ISR 中调用。
 */
esp_err_t Key_Scan(key_id_t id, bool *pressed);
