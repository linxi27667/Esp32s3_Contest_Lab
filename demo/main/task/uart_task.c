#include "config.h"
#include "app_protocol.h"
#include "uart_task.h"

/* 模板示例变量：正式做题时替换为 main_task.c 中对应的业务变量或函数。 */
static uint8_t g_template_mode = 0;

/*
 * 赛场改通信命令时，只修改这个函数。
 * 一条命令已经由 Protocol_Feed() 拼接完整，不包含 \r 和 \n。
 *
 * 当前支持：MODE 0 到 MODE 3。
 * 下面依次是：固定文字命令区、带数字参数命令区、错误回复区。
 */
static void Uart1_HandleCommand(const char *command)
{
    unsigned int requested_mode = 0;
    char trailing_character = '\0';

    /* ===== 1. 固定文字命令：在这里添加，例如 MOTOR ON / MOTOR OFF。 ===== */
    /*
    if (strcmp(command, "MOTOR ON") == 0) {
        (void)Motor_Set(true);      // 替换为你的电机 HAL 函数。
        (void)Uart_SendStr(UART_ID_1, "OK MOTOR=ON\r\n");
        return;
    }
    */

    /* ===== 2. 带数字参数命令：当前格式为 MODE n。 ===== */
    /*
     * &requested_mode：交给 sscanf() 写入解析出的数字。
     * &trailing_character：用于发现 "MODE 2abc" 这类多余字符。
     * 返回值为 1 表示只成功解析出一个模式数字，格式干净。
     */
    if (sscanf(command, "MODE %u %c", &requested_mode, &trailing_character) == 1 &&
        requested_mode <= 3U) {
        g_template_mode = (uint8_t)requested_mode;
        /* 正式做题时：在此处调用业务函数或修改 main_task 的状态变量。 */
        (void)Uart_Sendf(UART_ID_1, "OK MODE=%u\r\n", requested_mode);
        return;
    }

    /* ===== 3. 所有不认识或格式错误的命令统一回复错误。 ===== */
    (void)Uart_SendStr(UART_ID_1, "ERR FORMAT\r\n");
}

void Uart_Task(void *argument)
{
    (void)argument;
    uint8_t buffer[64];

    /* 行结束符和收包拼接规则在 app/app_protocol.c 中修改。 */
    (void)Protocol_Init(Uart1_HandleCommand);

    while (true) {
        int length = Uart_Read(
            UART_ID_1,
            buffer,
            sizeof(buffer),
            pdMS_TO_TICKS(UART_READ_TIMEOUT_MS));
        if (length > 0) {
            /* 一次读取不一定是一条完整命令，交给解析器持续拼接。 */
            if (Protocol_Feed(buffer, (size_t)length) == ESP_ERR_INVALID_SIZE) {
                (void)Uart_SendStr(UART_ID_1, "ERR TOO LONG\r\n");
            }
        }
    }
}
