#include "task_manager.h"

#include "adc_task.h"
#include "config.h"
#include "key_task.h"
#include "main_task.h"
#include "timer_task.h"
#include "touch_task.h"
#include "uart_task.h"

static const char *g_tag = "TASK_MANAGER";

static TaskHandle_t g_main_task_handle = NULL;
static TaskHandle_t g_key_task_handle = NULL;
static TaskHandle_t g_touch_task_handle = NULL;
static TaskHandle_t g_timer_task_handle = NULL;
static TaskHandle_t g_uart_task_handle = NULL;
static TaskHandle_t g_adc_task_handle = NULL;

/* 统一创建任务，并把 FreeRTOS 返回值转换成 ESP-IDF 错误码。 */
static esp_err_t Task_Create(
    TaskFunction_t entry,
    const char *name,
    uint32_t stack_size,
    UBaseType_t priority,
    TaskHandle_t *handle)
{
    if (xTaskCreate(entry, name, stack_size, NULL, priority, handle) != pdPASS) {
        ESP_LOGE(g_tag, "Failed to create task: %s", name);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t AppTasks_Start(void)
{
    /* Main_Task 先创建，使其他任务可以立即向总状态机发送通知。 */
    esp_err_t err = Task_Create(
        Main_Task, "Main_Task", MAIN_TASK_STACK_SIZE,
        MAIN_TASK_PRIORITY, &g_main_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Key_Task, "Key_Task", KEY_TASK_STACK_SIZE,
        KEY_TASK_PRIORITY, &g_key_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Touch_Task, "Touch_Task", TOUCH_TASK_STACK_SIZE,
        TOUCH_TASK_PRIORITY, &g_touch_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Timer_Task, "Timer_Task", TIMER_TASK_STACK_SIZE,
        TIMER_TASK_PRIORITY, &g_timer_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = Task_Create(
        Uart_Task, "Uart_Task", EXTERNAL_UART_TASK_STACK_SIZE,
        EXTERNAL_UART_TASK_PRIORITY, &g_uart_task_handle);
    if (err != ESP_OK) {
        return err;
    }

    return Task_Create(
        Adc_Task, "Adc_Task", ADC_TASK_STACK_SIZE,
        ADC_TASK_PRIORITY, &g_adc_task_handle);
}
