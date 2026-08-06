#include "config.h"

#include <stdbool.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

typedef struct {
    bool enabled;
    adc_unit_t unit;
    adc_channel_t channel;
    adc_oneshot_unit_handle_t *unit_handle;
    adc_cali_handle_t cali;
} adc_slot_t;

static adc_oneshot_unit_handle_t g_adc1_unit = NULL;
static bool g_adc_ready = false;

/* 通道表：unit/channel 来自 config，便于现场改接线。 */
static adc_slot_t g_adcs[ADC_ID_COUNT];

/* 为某通道创建曲线拟合校准句柄；失败不影响读原始值。 */
static esp_err_t Adc_Cali_Create(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_cali_handle_t *out)
{
    const adc_cali_curve_fitting_config_t config = {
        .unit_id = unit,
        .chan = channel,
        .atten = ADC_DEFAULT_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    return adc_cali_create_scheme_curve_fitting(&config, out);
}

/* 初始化通道表默认映射。 */
static void Adc_Slots_Reset(void)
{
    const adc_channel_t channels[ADC_ID_COUNT] = {
        ADC0_CHANNEL,
        ADC1_CHANNEL,
        ADC2_CHANNEL,
        ADC3_CHANNEL,
    };
    const bool enabled[ADC_ID_COUNT] = {
        ADC0_ENABLED,
        ADC1_ENABLED,
        ADC2_ENABLED,
        ADC3_ENABLED,
    };

    for (int i = 0; i < ADC_ID_COUNT; ++i) {
        g_adcs[i].enabled = enabled[i];
        g_adcs[i].unit = ADC_UNIT_1;
        g_adcs[i].channel = channels[i];
        g_adcs[i].unit_handle = &g_adc1_unit;
        g_adcs[i].cali = NULL;
    }
}

/* 初始化失败时回收。 */
static void Adc_Deinit_All(void)
{
    for (int i = 0; i < ADC_ID_COUNT; ++i) {
        if (g_adcs[i].cali != NULL) {
            adc_cali_delete_scheme_curve_fitting(g_adcs[i].cali);
            g_adcs[i].cali = NULL;
        }
    }

    if (g_adc1_unit != NULL) {
        adc_oneshot_del_unit(g_adc1_unit);
        g_adc1_unit = NULL;
    }

    g_adc_ready = false;
}

static esp_err_t Adc_Get_Slot(
    adc_id_t id,
    adc_slot_t **slot_out)
{
    if (id < 0 || id >= ADC_ID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (slot_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!g_adc_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!g_adcs[id].enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (g_adcs[id].unit_handle == NULL || *g_adcs[id].unit_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    *slot_out = &g_adcs[id];
    return ESP_OK;
}

/* 初始化 ADC 单元与全部通道，并尝试创建校准句柄。 */
esp_err_t Adc_Init(void)
{
    if (g_adc_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    Adc_Slots_Reset();

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err = adc_oneshot_new_unit(&unit_config, &g_adc1_unit);
    if (err != ESP_OK) {
        return err;
    }

    const adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_DEFAULT_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    for (int i = 0; i < ADC_ID_COUNT; ++i) {
        if (!g_adcs[i].enabled) {
            continue;
        }

        err = adc_oneshot_config_channel(
            g_adc1_unit,
            g_adcs[i].channel,
            &chan_config);
        if (err != ESP_OK) {
            Adc_Deinit_All();
            return err;
        }

        /* 校准失败允许（无 efuse 时），Read_Raw 仍可用。 */
        (void)Adc_Cali_Create(
            g_adcs[i].unit,
            g_adcs[i].channel,
            &g_adcs[i].cali);
    }

    g_adc_ready = true;
    return ESP_OK;
}

/* 读取指定通道的原始码值（0~4095，12bit）。 */
esp_err_t Adc_Read_Raw(adc_id_t id, int *raw_out)
{
    if (raw_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_slot_t *slot = NULL;
    esp_err_t err = Adc_Get_Slot(id, &slot);
    if (err != ESP_OK) {
        return err;
    }

    return adc_oneshot_read(
        *slot->unit_handle,
        slot->channel,
        raw_out);
}

esp_err_t Adc_Read_Sample(adc_id_t id, adc_sample_t *sample_out)
{
    if (sample_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_slot_t *slot = NULL;
    esp_err_t err = Adc_Get_Slot(id, &slot);
    if (err != ESP_OK) {
        return err;
    }

    sample_out->pin_mv = -1;
    err = adc_oneshot_read(
        *slot->unit_handle,
        slot->channel,
        &sample_out->raw);
    if (err != ESP_OK || slot->cali == NULL) {
        return err;
    }

    return adc_cali_raw_to_voltage(
        slot->cali,
        sample_out->raw,
        &sample_out->pin_mv);
}

/* 读取指定通道的电压，单位毫伏（依赖校准，未校准返回错误）。 */
esp_err_t Adc_Read_Mv(adc_id_t id, int *mv_out)
{
    if (mv_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_slot_t *slot = NULL;
    esp_err_t err = Adc_Get_Slot(id, &slot);
    if (err != ESP_OK) {
        return err;
    }

    if (slot->cali == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    err = adc_oneshot_read(*slot->unit_handle, slot->channel, &raw);
    if (err != ESP_OK) {
        return err;
    }

    return adc_cali_raw_to_voltage(slot->cali, raw, mv_out);
}
