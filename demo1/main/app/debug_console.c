#include "debug_console.h"

#include <stdio.h>

#include "config.h"
#include "esp_console.h"
#include "esp_log.h"

#include "hal_adc.h"
#include "hal_led.h"

static const char *g_tag = "CONSOLE";

/* 命令：ping；用途：确认调试控制台收发正常。 */
static int Debug_Command_Ping(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("PONG\n");
    return 0;
}

/* 命令：led1_on；用途：点亮第1路 LED。 */
static int Debug_Command_Led1_On(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    esp_err_t err = Led_Set(LED_ID_1, true);
    if (err != ESP_OK) {
        ESP_LOGE(g_tag, "led1_on failed: %s", esp_err_to_name(err));
        printf("ERR %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("OK LED1 ON\n");
    return 0;
}

/* 命令：led1_off；用途：熄灭第1路 LED。 */
static int Debug_Command_Led1_Off(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    esp_err_t err = Led_Set(LED_ID_1, false);
    if (err != ESP_OK) {
        ESP_LOGE(g_tag, "led1_off failed: %s", esp_err_to_name(err));
        printf("ERR %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("OK LED1 OFF\n");
    return 0;
}

/* 命令：adc_read；用途：读取ADC_ID_0的校准电压。 */
static int Debug_Command_Adc_Read(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int mv = 0;
    esp_err_t err = Adc_Read_Mv(ADC_ID_0, &mv);
    if (err != ESP_OK) {
        ESP_LOGE(g_tag, "adc_read failed: %s", esp_err_to_name(err));
        printf("ERR %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("ADC0=%d mV\n", mv);
    return 0;
}

/* 注册一个无需参数的调试命令。 */
static esp_err_t Debug_Command_Register(
    const char *command,
    const char *help,
    esp_console_cmd_func_t function)
{
    const esp_console_cmd_t command_config = {
        .command = command,
        .help = help,
        .func = function,
    };

    return esp_console_cmd_register(&command_config);
}

esp_err_t Debug_Console_Init(void)
{
    esp_err_t err = Debug_Command_Register(
        "ping",
        "test debug console communication",
        Debug_Command_Ping);
    if (err != ESP_OK) {
        return err;
    }

    err = Debug_Command_Register(
        "led1_on",
        "turn on LED1",
        Debug_Command_Led1_On);
    if (err != ESP_OK) {
        return err;
    }

    err = Debug_Command_Register(
        "led1_off",
        "turn off LED1",
        Debug_Command_Led1_Off);
    if (err != ESP_OK) {
        return err;
    }

    err = Debug_Command_Register(
        "adc_read",
        "read ADC0 calibrated voltage",
        Debug_Command_Adc_Read);
    if (err != ESP_OK) {
        return err;
    }

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "esp32>";
    repl_config.max_cmdline_length = DEBUG_CONSOLE_MAX_CMDLINE_LENGTH;

    esp_console_dev_uart_config_t uart_config =
        ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    esp_console_repl_t *repl = NULL;
    err = esp_console_new_repl_uart(
        &uart_config,
        &repl_config,
        &repl);
    if (err != ESP_OK) {
        return err;
    }

    return esp_console_start_repl(repl);
}
