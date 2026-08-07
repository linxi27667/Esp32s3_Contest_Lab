#include "config.h"
#include "timer_task.h"

static const char *g_tag = "TIMER_TASK";

static void Timer0_HandleEvent(const timer_event_t *event)
{
    switch (event->type) {
    case TIMER_EVENT_PERIODIC_TICK:
        ESP_LOGI(
            g_tag,
            "Timer0 periodic: period=%lums count=%llu us",
            (unsigned long)event->period_ms,
            (unsigned long long)event->count_us);
        break;
    case TIMER_EVENT_BOOT_COMPLETE:
        main_state = MAIN_STATE_RUNNING;
        (void)Led_Set(LED_ID_1, 1);
        (void)Led_Set(LED_ID_2, 1);
        (void)Led_Set(LED_ID_3, 1);
        (void)Led_Set(LED_ID_4, 1);
        (void)RgbLed_FillHsv(120, 255, 255);
        (void)RgbLed_Refresh();
        Uart_Sendf(UART_ID_1, "BOOT OK\r\n");
        ESP_LOGI(g_tag, "System Init Successful");
        break;
    default:
        ESP_LOGW(g_tag, "Unknown Timer0 event: %u", (unsigned)event->type);
        break;
    }
}

static void Timer1_HandleEvent(const timer_event_t *event)
{
    switch (event->type) {
    case TIMER_EVENT_LED_BLINK:
        /* TODO: 添加 TIMER_ID_1 的 LED 闪烁业务。 */
        break;
    case TIMER_EVENT_RGB_REFRESH:
        /* TODO: 添加 TIMER_ID_1 的 RGB 刷新业务。 */
        break;
    default:
        ESP_LOGW(g_tag, "Unknown Timer1 event: %u", (unsigned)event->type);
        break;
    }
}

static void Timer2_HandleEvent(const timer_event_t *event)
{
    ESP_LOGI(g_tag, "Timer2 event type=%u", (unsigned)event->type);
}

static void Timer3_HandleEvent(const timer_event_t *event)
{
    ESP_LOGI(g_tag, "Timer3 event type=%u", (unsigned)event->type);
}

void Timer_Task(void *argument)
{
    (void)argument;
    timer_event_t event;

    while (true) {
        if (Timer_GetEvent(&event, portMAX_DELAY) != ESP_OK) {
            continue;
        }

        /* 先区分硬件 Timer ID，再区分该 Timer 承担的业务事件。 */
        switch (event.id) {
        case TIMER_ID_0:
            Timer0_HandleEvent(&event);
            break;
        case TIMER_ID_1:
            Timer1_HandleEvent(&event);
            break;
        case TIMER_ID_2:
            Timer2_HandleEvent(&event);
            break;
        case TIMER_ID_3:
            Timer3_HandleEvent(&event);
            break;
        default:
            ESP_LOGW(g_tag, "Unknown timer id: %u", (unsigned)event.id);
            break;
        }
    }
}
