#pragma once

/* 各任务入口只供 task_manager.c 注册，业务代码写在对应的 .c 文件中。 */
void Main_Task(void *argument);
void Key_Task(void *argument);
void Touch_Task(void *argument);
void Timer_Task(void *argument);
void Uart_Task(void *argument);
void Adc_Task(void *argument);
