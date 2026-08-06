#include "config.h"
#include "main_task.h"
#include "task_entries.h"

static const char *g_tag = "MAIN_TASK";

/* 这些状态会被按键任务和定时器任务访问，用临界区保护读写。 */
static portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t g_main_task_handle = NULL;
static uint8_t g_mode = 0;
static bool g_mode_dirty = false;
static bool g_boot_complete = false;

/* 统一检查显示 API，单个外设失败时记录错误但不终止整个状态机。 */
static void MainTask_LogResult(const char *operation, esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(g_tag, "%s failed: %s", operation, esp_err_to_name(err));
    }
}

static void MainTask_SetRgbHsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    esp_err_t err = RgbLed_FillHsv(hue, saturation, value);
    if (err == ESP_OK) {
        err = RgbLed_Refresh();
    }
    MainTask_LogResult("RGB update", err);
}

/* 上电后的等待画面：LED1、LED3 和蓝色 RGB。 */
static void MainTask_ShowBootWaiting(void)
{
    MainTask_LogResult("LED1 on", Led_Set(LED_ID_1, true));
    MainTask_LogResult("LED2 off", Led_Set(LED_ID_2, false));
    MainTask_LogResult("LED3 on", Led_Set(LED_ID_3, true));
    MainTask_LogResult("LED4 off", Led_Set(LED_ID_4, false));
    MainTask_SetRgbHsv(240, 255, 255);
    MainTask_LogResult("active buzzer off", Buzzer_Active_Set(false));
}

/* 启动等待结束后的确认画面：四灯全亮、RGB 绿色并发送串口消息。 */
static void MainTask_ShowBootComplete(void)
{
    MainTask_LogResult("all LEDs on", Led_All_Set(true));
    MainTask_SetRgbHsv(120, 255, 255);
    MainTask_LogResult(
        "send BOOT OK",
        Uart_SendStr(UART_ID_1, "BOOT OK\r\n"));
}

/* 根据题目要求，只在模式发生变化时刷新一次灯光。 */
static void MainTask_ShowMode(uint8_t mode)
{
    MainTask_LogResult("all LEDs off", Led_All_Set(false));

    switch (mode) {
    case 0:
        MainTask_SetRgbHsv(0, 0, 0);
        break;
    case 1:
        MainTask_LogResult("LED1 on", Led_Set(LED_ID_1, true));
        MainTask_SetRgbHsv(0, 255, 255);
        break;
    case 2:
        MainTask_LogResult("LED2 on", Led_Set(LED_ID_2, true));
        MainTask_SetRgbHsv(120, 255, 255);
        break;
    case 3:
        MainTask_LogResult("LED3 on", Led_Set(LED_ID_3, true));
        MainTask_SetRgbHsv(240, 255, 255);
        break;
    default:
        ESP_LOGW(g_tag, "Unknown mode: %u", (unsigned)mode);
        break;
    }
}

uint8_t MainTask_NextMode(void)
{
    taskENTER_CRITICAL(&g_state_lock);
    g_mode = (uint8_t)((g_mode + 1U) % 4U);
    g_mode_dirty = true;
    const uint8_t mode = g_mode;
    taskEXIT_CRITICAL(&g_state_lock);

    if (g_main_task_handle != NULL) {
        xTaskNotifyGive(g_main_task_handle);
    }
    return mode;
}

void MainTask_SetBootComplete(void)
{
    taskENTER_CRITICAL(&g_state_lock);
    g_boot_complete = true;
    taskEXIT_CRITICAL(&g_state_lock);

    if (g_main_task_handle != NULL) {
        xTaskNotifyGive(g_main_task_handle);
    }
}

void Main_Task(void *argument)
{
    (void)argument;
    g_main_task_handle = xTaskGetCurrentTaskHandle();
    bool boot_screen_rendered = false;

    MainTask_ShowBootWaiting();
    MainTask_LogResult("startup beep", Buzzer_Beep_Once(1000, 100));
    MainTask_LogResult(
        "boot timer",
        Timer_Start_OneShot_Ms(
            TIMER_ID_0,
            TIMER_EVENT_BOOT_COMPLETE,
            BOOT_COMPLETE_DELAY_MS));

    while (true) {
        /* 按键或定时器改变状态后会唤醒本任务，无事件时不占用 CPU。 */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        taskENTER_CRITICAL(&g_state_lock);
        const bool boot_complete = g_boot_complete;
        const bool mode_dirty = g_mode_dirty;
        const uint8_t mode = g_mode;
        if (boot_complete && mode_dirty) {
            g_mode_dirty = false;
        }
        taskEXIT_CRITICAL(&g_state_lock);

        if (boot_complete && !boot_screen_rendered) {
            MainTask_ShowBootComplete();
            boot_screen_rendered = true;
        }

        /* 启动画面保持到第一次有效的按键释放事件。 */
        if (boot_complete && mode_dirty) {
            MainTask_ShowMode(mode);
        }
    }
}
