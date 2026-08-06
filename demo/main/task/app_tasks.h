#pragma once

#include "esp_err.h"

/**
 * @brief 创建并启动全部应用任务。
 *
 * @retval ESP_OK 全部任务创建成功。
 * @retval ESP_ERR_NO_MEM 任一任务创建失败。
 */
esp_err_t AppTasks_Start(void);
