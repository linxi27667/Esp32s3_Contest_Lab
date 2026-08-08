# ESP32 Embedded Contest Workspace

## Environment

- Framework: ESP-IDF 5.5.3
- Target: ESP32-S3

## Current layout

- `main/main.c` is the application entry point.
- `main/main.c` also owns FreeRTOS task templates and creates all application tasks.
- `main/config.h` holds board-level pin definitions and includes all HAL headers.
- `main/hardware/hal_led.c` provides the LED driver.
- `main/hardware/hal_buzzer.c` provides active-buzzer GPIO and passive-buzzer LEDC drivers.
- `main/hardware/hal_key.c` provides the GPIO interrupt key driver and event queue.

## Current LED default

- `LED1_PIN`--`LED4_PIN` are the four LED mappings in `main/config.h`; the current examples are GPIO1, GPIO2, GPIO4, and GPIO5.
- `LED_ALL_PIN_MASK` combines the four pins with `1ULL << LEDx_PIN`.
- `Led_Init()` configures all four outputs and leaves them off.

## Buzzer driver

- `ACTIVE_BUZZER_PIN` and `PASSIVE_BUZZER_PIN` are example mappings in `main/config.h` and must be replaced with the board schematic pins.
- `Buzzer_Active_Set()` controls an active buzzer as a GPIO switch.
- `Buzzer_Passive_Play()` and `Buzzer_Passive_Stop()` control a passive buzzer with LEDC frequency output.
- LEDC mode, timer, channel, resolution, and default duty are private implementation settings in `hal_buzzer.c`.
- `Buzzer_Task` is created by `Task_CreateAll()` and is the application layer for buzzer timing/behavior.

## Key input driver

- `KEY_PIN` is an example GPIO8 mapping in `main/config.h` and must match the board wiring.
- The key uses an internal pull-up and falling-edge interrupt; the pressed level is `KEY_ACTIVE_LEVEL` (currently 0).
- The ISR only queues an event. `Key_Task` waits for the event, delays 20 ms for debounce, then reads the stable level and runs application behavior.
- `Key_Init()` + `Key_Scan()` provide polling-only mode with 20 ms delay debounce.
- `Key_Interrupt_Init()` + `Key_GetEvent()` provide interrupt mode; do not use both processing modes for the same key at once.

## Hardware timer driver

- `main/hardware/hal_timer.c` uses ESP-IDF GPTimer at 1 MHz resolution (1 tick = 1 µs).
- Select timers like STM32 TIMx via `timer_id_t`: `TIMER_ID_0`..`TIMER_ID_3` (`TIMER_ID_COUNT` is 4, full ESP32-S3 GPTimer set).
- `Timer_Init()` creates all GPTimer instances, one shared event queue, registers ISRs, and enables timers without starting them.
- `Timer_Start_Periodic_Ms(id, type, ms)` / `Timer_Start_OneShot_Ms(id, type, ms)` / `Timer_Stop(id)` operate on the selected timer.
- The alarm ISR queues a `timer_event_t` with `.id` and `.type`; `Timer_Task` waits with `Timer_GetEvent()` and dispatches the business event by `event.type`.
- GPTimer callbacks request a yield by returning `true`; do not call `portYIELD_FROM_ISR` inside the alarm callback.
- Default demo period is `TIMER_DEFAULT_PERIOD_MS` in `main/config.h` (demo starts `TIMER_ID_0` only).

## FreeRTOS task template

- `Task_CreateAll()` is the only task-registration point.
- Task functions use the `<Peripheral>_Task` naming style, currently `Led_Task`.
- File-scope/global variables use the `g_` prefix, such as `g_tag`, `g_led_task_handle`, and `g_led_pins`.
- Task settings are defined in `main/config.h`, currently `LED_TASK_STACK_SIZE` and `LED_TASK_PRIORITY`; task delays stay as explicit values in the task body.
- Every task must always block with `vTaskDelay()` or an event wait.
- `Led_Task()` and `Task_CreateAll()` are defined above `app_main()` so no forward declarations are needed.

## UART driver

- `main/hardware/hal_uart.c` exposes two business ports via `uart_id_t`: `UART_ID_0`, `UART_ID_1` (`UART_ID_COUNT` is 2).
- Does **not** use `UART_NUM_0` (left for ESP_LOG console).
- Mapping in `main/config.h`:
  - `UART_ID_0` → `UART0_PORT_NUM` (default `UART_NUM_1`), TX/RX `UART0_TX_PIN`/`UART0_RX_PIN` (GPIO17/18), baud `UART0_BAUD_RATE`
  - `UART_ID_1` → `UART1_PORT_NUM` (default `UART_NUM_2`), TX/RX `UART1_TX_PIN`/`UART1_RX_PIN` (GPIO15/16), baud `UART1_BAUD_RATE`
- `Uart_Init()` installs both ports once; `Uart_Send`/`SendStr`/`Sendf`/`Read` all take `uart_id_t`.
- `Uart_Sendf()` uses a stack buffer of `UART_PRINTF_BUFFER_SIZE` bytes; over-long output is truncated.
- `Uart_Read()` returns bytes read, 0 on timeout, -1 on error.
- `Uart_Task` demo uses `UART_ID_0` echo only; use `UART_ID_1` for a second device.

## ADC driver

- `main/hardware/hal_adc.c` uses `ADC_UNIT_1` only (ADC2 is unstable with Wi-Fi).
- Select channels via `adc_id_t`: `ADC_ID_0`..`ADC_ID_3` (`ADC_ID_COUNT` is 4).
- Channel macros in `main/config.h`: `ADC0_CHANNEL`..`ADC3_CHANNEL` (defaults CH8/9/6/7 → GPIO9/10/7/8); change to match the board and avoid output-pin conflicts.
- Attenuation `ADC_ATTEN_DB_12` ≈ 0~3.3V; bitwidth `ADC_BITWIDTH_DEFAULT` (12-bit on S3).
- `Adc_Init()` creates the oneshot unit, configures all channels, tries curve-fitting calibration per channel.
- `Adc_Read_Raw(id, &raw)` → 0~4095; `Adc_Read_Mv(id, &mv)` → mV if calibration exists, else `ESP_ERR_INVALID_STATE`.
- `Adc_Task` demo samples `ADC_ID_0` every `ADC_SAMPLE_INTERVAL_MS`.

## ESP32-S3 pin safety

- Avoid GPIO26--GPIO32 because they are normally Flash pins.
- Avoid boot strapping pins GPIO0, GPIO3, GPIO45, and GPIO46.
- Keep GPIO19/GPIO20 available when native USB is used.
