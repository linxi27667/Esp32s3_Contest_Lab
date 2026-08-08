#pragma once

/* 常用 C 语言类型、字符串、格式化和内存操作。 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* ESP-IDF 基础错误、日志、系统和计时接口。 */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_system.h"
#include "esp_timer.h"

/* FreeRTOS 任务、队列、信号量和基础类型。 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/* 本模板使用到的 ESP-IDF 外设驱动。 */
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "driver/touch_sensor_legacy.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "led_strip.h"

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

/* 蜂鸣器示例引脚：根据开发板原理图修改。 */
#define ACTIVE_BUZZER_PIN GPIO_NUM_6
#define PASSIVE_BUZZER_PIN GPIO_NUM_7
#define ACTIVE_BUZZER_ON_LEVEL 1
#define ACTIVE_BUZZER_OFF_LEVEL 0

/* 按键资源配置：默认启用 KEY_ID_0，按键另一端接 GND，按下时为低电平。 */
#define KEY_ID_0_ENABLED 1
#define KEY_ID_0_PIN GPIO_NUM_8
#define KEY_ID_0_ACTIVE_LEVEL 0

/* 其余按键默认关闭；使用时修改 ENABLED、PIN 和 ACTIVE_LEVEL。 */
#define KEY_ID_1_ENABLED 0
#define KEY_ID_1_PIN GPIO_NUM_NC
#define KEY_ID_1_ACTIVE_LEVEL 0
#define KEY_ID_2_ENABLED 0
#define KEY_ID_2_PIN GPIO_NUM_NC
#define KEY_ID_2_ACTIVE_LEVEL 0
#define KEY_ID_3_ENABLED 0
#define KEY_ID_3_PIN GPIO_NUM_NC
#define KEY_ID_3_ACTIVE_LEVEL 0

#define KEY_QUEUE_LENGTH 8

/* 主业务任务：负责总状态机和多个外设之间的联动逻辑。 */
#define MAIN_TASK_STACK_SIZE 4096
#define MAIN_TASK_PRIORITY 6
#define MAIN_TASK_INTERVAL_MS 10

/* 按键任务配置。 */
#define KEY_TASK_STACK_SIZE 3072
#define KEY_TASK_PRIORITY 5

/* 触摸任务配置：触摸硬件连续扫描，应用层按固定周期读取状态。 */
#define TOUCH_TASK_STACK_SIZE 3072
#define TOUCH_TASK_PRIORITY 5

/* 硬件定时器：GPTimer 分辨率 1MHz，事件队列长度。 */
#define TIMER_QUEUE_LENGTH 8
#define TIMER_DEFAULT_PERIOD_MS 1000

/* 定时器任务配置。 */
#define TIMER_TASK_STACK_SIZE 4096
#define TIMER_TASK_PRIORITY 5

/* UART：UART_NUM_0 由 ESP-IDF console 管理，下面只配置外部通信 UART。 */
#define UART_EXTERNAL_PORT_NUM UART_NUM_1
#define UART_EXTERNAL_TX_PIN GPIO_NUM_17
#define UART_EXTERNAL_RX_PIN GPIO_NUM_18
#define UART_EXTERNAL_BAUD_RATE 115200

/* UART 资源配置：UART0 通常由 Type-C 调试控制台使用，默认关闭业务驱动。 */
#define UART_ID_0_ENABLED 0
#define UART_ID_0_PORT_NUM UART_NUM_0
#define UART_ID_0_TX_PIN UART_PIN_NO_CHANGE
#define UART_ID_0_RX_PIN UART_PIN_NO_CHANGE
#define UART_ID_0_BAUD_RATE 115200

/* UART1 通过 GPIO17/GPIO18 连接外部上位机，UART_EXTERNAL 是 UART_ID_1 的别名。 */
#define UART_ID_1_ENABLED 1
#define UART_ID_1_PORT_NUM UART_EXTERNAL_PORT_NUM
#define UART_ID_1_TX_PIN UART_EXTERNAL_TX_PIN
#define UART_ID_1_RX_PIN UART_EXTERNAL_RX_PIN
#define UART_ID_1_BAUD_RATE UART_EXTERNAL_BAUD_RATE

/* UART2 是备用资源，配置实际引脚后可将此值改为 1。 */
#define UART_ID_2_ENABLED 0
#define UART_ID_2_PORT_NUM UART_NUM_2
#define UART_ID_2_TX_PIN UART_PIN_NO_CHANGE
#define UART_ID_2_RX_PIN UART_PIN_NO_CHANGE
#define UART_ID_2_BAUD_RATE 115200

#define UART_TX_BUFFER_SIZE 1024
#define UART_RX_BUFFER_SIZE 1024
#define UART_PRINTF_BUFFER_SIZE 128
#define UART_READ_TIMEOUT_MS 20 /* 收到一批字节后最多等待 20 ms，不要使用 portMAX_DELAY。 */
#define UART_COMMAND_MAX_LENGTH 63
#define DEBUG_CONSOLE_MAX_CMDLINE_LENGTH 128

/* 外部 UART 通信任务配置。 */
#define EXTERNAL_UART_TASK_STACK_SIZE 3072
#define EXTERNAL_UART_TASK_PRIORITY 5

/* ADC：仅用 ADC1（ADC2 开 WiFi 时不稳）。S3 上 ADC1_CHx ↔ GPIOx+1 规律见手册。
 * ADC_DEFAULT_ATTEN 是 ADC 引脚的输入衰减，不代表可直接安全测量 3.3 V。
 * 要测 3.3 V 电源，请先用电阻分压将 ADC 引脚电压限制在芯片 ADC 范围内。 */
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
#define ADC_SAMPLE_INTERVAL_MS 2000

/* 电容触摸输入：ESP32-S3 的触摸通道 14 对应 GPIO14。 */
#define TOUCH_CHANNEL_ID 14
#define TOUCH_INITIAL_STABILIZE_MS 50
#define TOUCH_THRESHOLD_PERCENT 3

/* 非易失存储：NVS 命名空间和键名的长度均不能超过 15 个字符。 */
#define NVS_NAMESPACE "contest"

/* 可编程全彩灯：使用 RMT 驱动兼容 WS2812 的 RGB 灯带。 */
#define RGB_LED_PIN GPIO_NUM_11
#define RGB_LED_DEFAULT_COUNT 1
#define RGB_LED_RMT_RESOLUTION_HZ 10000000
#define RGB_LED_DEFAULT_BRIGHTNESS 64
#define RGB_LED_RAINBOW_STEP_DEGREES 3

/* 硬件抽象层头文件。 */
#include "hal_led.h"
#include "hal_buzzer.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "hal_adc.h"
#include "hal_touch.h"
#include "hal_nvs.h"
#include "hal_rgb_led.h"

/* 应用任务头文件：业务文件只需包含 config.h 即可访问全部任务接口。 */
#include "task_manager.h"
#include "main_task.h"
#include "key_task.h"
#include "timer_task.h"
#include "touch_task.h"
#include "uart_task.h"
#include "adc_task.h"
