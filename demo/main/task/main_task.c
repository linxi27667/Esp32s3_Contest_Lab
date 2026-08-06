#include "config.h"
#include "main_task.h"

/* 比赛中的总模式和跨外设联动状态机放在本文件。 */
typedef enum {
    APP_STATE_IDLE = 0,
    APP_STATE_RUNNING,
    APP_STATE_ALARM,
} app_state_t;

void Main_Task(void *argument)
{
    (void)argument;
    app_state_t state = APP_STATE_IDLE;

    while (true) {
        switch (state) {
        case APP_STATE_IDLE:
            /* TODO: 等待开始条件，满足后切换到 APP_STATE_RUNNING。 */
            break;

        case APP_STATE_RUNNING:
            /* TODO: 编写正常运行时的跨外设联动。 */
            break;

        case APP_STATE_ALARM:
            /* TODO: 编写报警处理以及恢复条件。 */
            break;

        default:
            state = APP_STATE_IDLE;
            break;
        }

        /* 状态机必须让出 CPU，周期可在 config.h 中修改。 */
        vTaskDelay(pdMS_TO_TICKS(MAIN_TASK_INTERVAL_MS));
    }
}
