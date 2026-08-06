#pragma once

#include <stdint.h>

/** @brief 系统总状态机任务入口。 */
void Main_Task(void *argument);

/**
 * @brief 切换到下一个显示模式，并通知主任务刷新界面。
 *
 * @return 切换后的模式编号，范围为 0~3。
 */
uint8_t MainTask_NextMode(void);

/**
 * @brief 通知主任务启动等待已经结束。
 */
void MainTask_SetBootComplete(void);
