#pragma once


#include "hal_adc.h"

/*
 * 最新一次 ADC_ID_0 采样结果。
 * 变量只在 adc_task.c 定义一次；其他业务文件包含 config.h 后可直接读取。
 * 赛场上需要跨任务共享的业务数据按这种“extern 声明 + 单一 .c 定义”方式添加。
 */
extern adc_sample_t g_adc0_sample;

/** @brief ADC 周期采样和采样业务任务入口。 */
void Adc_Task(void *argument);
