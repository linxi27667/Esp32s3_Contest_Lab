#pragma once

/* 主任务状态需要被按键、ADC 等业务文件访问，因此放在头文件中。 */
typedef enum {
    MAIN_STATE_IDLE = 0, /**< 空闲状态。 */
    MAIN_STATE_RUNNING,  /**< 正常运行状态。 */
    MAIN_STATE_ALARM,    /**< 报警状态。 */
} main_state_t;

/** @brief 系统总状态机任务入口。 */
void Main_Task(void *argument);
