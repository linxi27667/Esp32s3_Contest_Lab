#pragma once

#include "esp_err.h"

/* 像 STM32 选 ADC 通道一样，用 ID 选择已配置通道。
 * 通道映射和启用开关在 config.h 修改。 */
typedef enum {
    ADC_ID_0 = 0, /**< 第1路 ADC，映射和使能状态在 config.h 中配置。 */
    ADC_ID_1,     /**< 第2路 ADC，映射和使能状态在 config.h 中配置。 */
    ADC_ID_2,     /**< 第3路 ADC，映射和使能状态在 config.h 中配置。 */
    ADC_ID_3,     /**< 第4路 ADC，映射和使能状态在 config.h 中配置。 */
    ADC_ID_COUNT,
} adc_id_t;

typedef struct {
    int raw;    /**< 12 位 ADC 原始码值，正常范围为 0~4095。 */
    int pin_mv; /**< 校准后的 ADC 引脚电压，单位 mV；无校准句柄时为 -1。 */
} adc_sample_t;

/**
 * @brief 初始化 ADC1 单元、所有已启用通道和曲线拟合校准句柄。
 *
 * 通道、启用开关和衰减档位由 config.h 中的 ADCx_CHANNEL、ADCx_ENABLED、
 * ADC_DEFAULT_ATTEN 配置。校准创建失败不会阻止原始值读取。
 *
 * @retval ESP_OK ADC 初始化成功。
 * @retval ESP_ERR_INVALID_STATE ADC 库已经初始化。
 * @retval 其他值 ADC 单元创建或通道配置失败。
 */
esp_err_t Adc_Init(void);

/**
 * @brief 读取指定 ADC 通道的一次 12 位原始码值。
 *
 * @param[in] id ADC 编号，可填 ADC_ID_0、ADC_ID_1、ADC_ID_2、ADC_ID_3；
 *               对应通道必须在 config.h 中启用。
 * @param[out] raw_out 接收原始码值的地址，正常范围 0~4095，不能为 NULL。
 *
 * @retval ESP_OK 读取成功。
 * @retval ESP_ERR_INVALID_ARG id 无效或 raw_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Adc_Init()。
 * @retval ESP_ERR_NOT_SUPPORTED 对应 ADCx_ENABLED 为 0。
 * @retval 其他值 ADC 硬件读取失败。
 */
esp_err_t Adc_Read_Raw(adc_id_t id, int *raw_out);

/**
 * @brief 一次采样同时返回原始码值和校准后的 ADC 引脚电压。
 *
 * @param[in] id ADC 编号，可填 ADC_ID_0、ADC_ID_1、ADC_ID_2、ADC_ID_3；
 *               对应通道必须启用。
 * @param[out] sample_out 接收 adc_sample_t 结果的地址，不能为 NULL。
 *
 * @retval ESP_OK 原始值读取成功；若校准不可用，sample_out->pin_mv 为 -1。
 * @retval ESP_ERR_INVALID_ARG id 无效或 sample_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Adc_Init()。
 * @retval ESP_ERR_NOT_SUPPORTED 对应 ADCx_ENABLED 为 0。
 * @retval 其他值 ADC 读取或电压校准转换失败。
 * @note pin_mv 表示 ADC 引脚上的电压，不会自动补偿外部电阻分压比例。
 */
esp_err_t Adc_Read_Sample(adc_id_t id, adc_sample_t *sample_out);

/**
 * @brief 读取指定 ADC 通道的校准电压，单位毫伏。
 *
 * @param[in] id ADC 编号，可填 ADC_ID_0、ADC_ID_1、ADC_ID_2、ADC_ID_3；
 *               对应通道必须启用。
 * @param[out] mv_out 接收 ADC 引脚电压的地址，单位 mV，不能为 NULL。
 *
 * @retval ESP_OK 读取并转换成功。
 * @retval ESP_ERR_INVALID_ARG id 无效或 mv_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE ADC 未初始化或该通道没有校准句柄。
 * @retval ESP_ERR_NOT_SUPPORTED 对应 ADCx_ENABLED 为 0。
 * @retval 其他值 ADC 读取或校准转换失败。
 * @note ADC_ATTEN_DB_12 只是最高衰减档，不代表可以直接准确测量 3.3 V。
 *       测量较高电压时应使用外部分压，并在应用层换算原始输入电压。
 */
esp_err_t Adc_Read_Mv(adc_id_t id, int *mv_out);
