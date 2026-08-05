# ESP32 BSP In Qiansai

## Project profile

- Framework: ESP-IDF 5.5.3
- Target: ESP32-S3
- Application project: `demo/`
- Build system: ESP-IDF CMake

## Layout

- `demo/main/main.c`: application entry point and task registration.
- `demo/main/config.h`: board pin mappings and task settings.
- `demo/main/hardware/`: GPIO, timer, UART, ADC, LED, buzzer, and key HAL drivers.
- `demo/main/app/`: application protocol and debug console modules.
- `demo/sdkconfig.defaults`: portable default configuration.
- `demo/sdkconfig`: current project configuration.

## Version-control notes

ESP-IDF build output under any `build/` directory and local caches are ignored. Source files, CMake files, and project configuration remain trackable.
