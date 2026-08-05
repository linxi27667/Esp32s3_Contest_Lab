#pragma once

#include "driver/gpio.h"

/* LED示例引脚：根据开发板原理图修改。 */
#define LED1_PIN GPIO_NUM_1
#define LED2_PIN GPIO_NUM_2
#define LED3_PIN GPIO_NUM_4
#define LED4_PIN GPIO_NUM_5

#define LED_ON_LEVEL 1
#define LED_OFF_LEVEL 0
#define LED_ALL_PIN_MASK \
    ((1ULL << LED1_PIN) | (1ULL << LED2_PIN) | \
     (1ULL << LED3_PIN) | (1ULL << LED4_PIN))

/* ESP-IDF中任务栈参数的单位是字节。 */
#define LED_TASK_STACK_SIZE 2048
#define LED_TASK_PRIORITY 5

/* 蜂鸣器示例引脚：根据开发板原理图修改。 */
#define ACTIVE_BUZZER_PIN GPIO_NUM_6
#define PASSIVE_BUZZER_PIN GPIO_NUM_7
#define ACTIVE_BUZZER_ON_LEVEL 1
#define ACTIVE_BUZZER_OFF_LEVEL 0

/* 蜂鸣器任务配置。 */
#define BUZZER_TASK_STACK_SIZE 2048
#define BUZZER_TASK_PRIORITY 5

/* 按键示例引脚：按键另一端接GND，使用内部上拉，按下时为低电平。 */
#define KEY_PIN GPIO_NUM_8
#define KEY_ACTIVE_LEVEL 0
#define KEY_QUEUE_LENGTH 8

/* 按键任务配置。 */
#define KEY_TASK_STACK_SIZE 2048
#define KEY_TASK_PRIORITY 5

/* 硬件定时器：GPTimer 分辨率 1MHz，事件队列长度。 */
#define TIMER_QUEUE_LENGTH 8
#define TIMER_DEFAULT_PERIOD_MS 1000

/* 定时器任务配置。 */
#define TIMER_TASK_STACK_SIZE 2048
#define TIMER_TASK_PRIORITY 5

/* UART：UART_NUM_0 由 ESP-IDF console 管理，下面只配置外部通信 UART。 */
#include "driver/uart.h"
#define UART_EXTERNAL_PORT_NUM UART_NUM_1
#define UART_EXTERNAL_TX_PIN GPIO_NUM_17
#define UART_EXTERNAL_RX_PIN GPIO_NUM_18
#define UART_EXTERNAL_BAUD_RATE 115200

#define UART_TX_BUFFER_SIZE 1024
#define UART_RX_BUFFER_SIZE 1024
#define UART_PRINTF_BUFFER_SIZE 128
#define UART_COMMAND_MAX_LENGTH 63
#define DEBUG_CONSOLE_MAX_CMDLINE_LENGTH 128

/* 外部 UART 通信任务配置。 */
#define EXTERNAL_UART_TASK_STACK_SIZE 3072
#define EXTERNAL_UART_TASK_PRIORITY 5

/* ADC：仅用 ADC1（ADC2 开 WiFi 时不稳）。S3 上 ADC1_CHx ↔ GPIOx+1 规律见手册。
 * ADC_DEFAULT_ATTEN 是 ADC 引脚的输入衰减，不代表可直接安全测量 3.3 V。
 * 要测 3.3 V 电源，请先用电阻分压将 ADC 引脚电压限制在芯片 ADC 范围内。 */
#include "esp_adc/adc_oneshot.h"
#define ADC0_CHANNEL ADC_CHANNEL_8 /* GPIO9  相对空闲 */
#define ADC1_CHANNEL ADC_CHANNEL_9 /* GPIO10 相对空闲 */
#define ADC2_CHANNEL ADC_CHANNEL_6 /* GPIO7  默认与无源蜂鸣示例冲突，用前改脚 */
#define ADC3_CHANNEL ADC_CHANNEL_7 /* GPIO8  默认与按键示例冲突，用前改脚 */
#define ADC0_ENABLED 1
#define ADC1_ENABLED 1
#define ADC2_ENABLED 0 /* 改到无冲突引脚后再启用 */
#define ADC3_ENABLED 0 /* 改到无冲突引脚后再启用 */
#define ADC_DEFAULT_ATTEN ADC_ATTEN_DB_12

/* ADC任务配置。 */
#define ADC_TASK_STACK_SIZE 3072
#define ADC_TASK_PRIORITY 5
#define ADC_SAMPLE_INTERVAL_MS 1000

/* HAL headers */
#include "hal_led.h"
#include "hal_buzzer.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "hal_adc.h"
