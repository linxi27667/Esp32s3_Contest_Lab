#include "config.h"
#include "main_task.h"

main_state_t main_state = MAIN_STATE_IDLE;
uint8_t f_mode = 0;

void Main_Task(void *argument)
{
    (void)argument;

    while (true) {
        switch (main_state) {
        case MAIN_STATE_IDLE:
            /* TODO: 等待开始条件，满足后切换到 MAIN_STATE_RUNNING。 */
            break;

        case MAIN_STATE_RUNNING:
            /* TODO: 编写正常运行时的跨外设联动。 */
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
