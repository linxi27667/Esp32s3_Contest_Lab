#include "config.h"
#include "main_task.h"

main_state_t main_state = MAIN_STATE_IDLE;
uint8_t f_mode = 3;
static uint8_t g_last_mode = 0xff;

void Main_Task(void *argument)
{
    (void)argument;

    (void)Led_Set(LED_ID_1, true);
    (void)Led_Set(LED_ID_3, true);
    (void)Led_Set(LED_ID_2, false);
    (void)Led_Set(LED_ID_4, false);
    (void)RgbLed_FillHsv(240, 255, 255);
    (void)RgbLed_Refresh();
    (void)Buzzer_Beep_Once(1000, 100);
    (void)Timer_Start_OneShot_Ms(
        TIMER_ID_0, TIMER_EVENT_BOOT_COMPLETE, 1000);

    while (true) {
        switch (main_state) {
        case MAIN_STATE_IDLE:
            /* TODO: 等待开始条件，满足后切换到 MAIN_STATE_RUNNING。 */
            break;

        case MAIN_STATE_RUNNING:
            /* 模式没有变化时不刷新，保留定时完成后的绿色画面。 */
            if (f_mode == g_last_mode) {
                break;
            }

            switch (f_mode) {
            case 0:
                (void)Led_Set(LED_ID_1, false);
                (void)Led_Set(LED_ID_2, false);
                (void)Led_Set(LED_ID_3, false);
                (void)Led_Set(LED_ID_4, false);
                (void)RgbLed_FillHsv(0, 0, 0);
                (void)RgbLed_Refresh();
                break;
            case 1:
                (void)Led_Set(LED_ID_1, true);
                (void)Led_Set(LED_ID_2, false);
                (void)Led_Set(LED_ID_3, false);
                (void)Led_Set(LED_ID_4, false);
                (void)RgbLed_FillHsv(0, 255, 255);
                (void)RgbLed_Refresh();
                break;
            case 2:
                (void)Led_Set(LED_ID_1, false);
                (void)Led_Set(LED_ID_2, true);
                (void)Led_Set(LED_ID_3, false);
                (void)Led_Set(LED_ID_4, false);
                (void)RgbLed_FillHsv(120, 255, 255);
                (void)RgbLed_Refresh();
                break;
            case 3:
                (void)Led_Set(LED_ID_1, false);
                (void)Led_Set(LED_ID_2, false);
                (void)Led_Set(LED_ID_3, true);
                (void)Led_Set(LED_ID_4, false);
                (void)RgbLed_FillHsv(240, 255, 255);
                (void)RgbLed_Refresh();
                break;
            default:
                break;
            }

            g_last_mode = f_mode;
            break;

        case MAIN_STATE_ALARM:
            /* TODO: 编写报警处理以及恢复条件。 */
            break;

        default:
            main_state = MAIN_STATE_IDLE;
            break;
        }

        /* 状态机必须让出 CPU，周期可在 config.h 中修改。 */
        vTaskDelay(pdMS_TO_TICKS(MAIN_TASK_INTERVAL_MS));
    }
}
