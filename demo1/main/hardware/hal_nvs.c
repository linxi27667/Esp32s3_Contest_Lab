#include "config.h"

#include "nvs.h"
#include "nvs_flash.h"

/* 记录默认 NVS 分区是否已经初始化，防止未初始化访问和重复初始化。 */
static bool g_nvs_ready = false;

/* 按指定读写模式打开比赛业务命名空间。 */
static esp_err_t Nvs_Open(nvs_open_mode_t mode, nvs_handle_t *handle_out)
{
    if (!g_nvs_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (handle_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return nvs_open(NVS_NAMESPACE, mode, handle_out);
}

/* 写操作成功后提交到 Flash，并保证无论成功失败都关闭句柄。 */
static esp_err_t Nvs_CommitAndClose(nvs_handle_t handle, esp_err_t err)
{
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

/* 初始化默认 NVS 分区；格式不兼容或空间布局异常时按官方流程恢复。 */
esp_err_t Nvs_Init(void)
{
    if (g_nvs_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = nvs_flash_init();
    /* 仅在分区无法继续使用时擦除；正常启动不会破坏已有数据。 */
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err == ESP_OK) {
        g_nvs_ready = true;
    }
    return err;
}

/* 保存一个 int32_t，并在返回前完成持久化提交。 */
esp_err_t Nvs_SetI32(const char *key, int32_t value)
{
    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    return Nvs_CommitAndClose(handle, nvs_set_i32(handle, key, value));
}

/* 读取一个 int32_t；类型不匹配时透传 ESP_ERR_NVS_TYPE_MISMATCH。 */
esp_err_t Nvs_GetI32(const char *key, int32_t *value_out)
{
    if (key == NULL || value_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_i32(handle, key, value_out);
    nvs_close(handle);
    return err;
}

/* 保存指定长度的二进制数据，并在返回前完成持久化提交。 */
esp_err_t Nvs_SetBlob(const char *key, const void *data, size_t length)
{
    if (key == NULL || data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    return Nvs_CommitAndClose(handle, nvs_set_blob(handle, key, data, length));
}

/* 读取 Blob；data 为 NULL 时 nvs_get_blob() 只返回所需长度。 */
esp_err_t Nvs_GetBlob(const char *key, void *data, size_t *length_in_out)
{
    if (key == NULL || length_in_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_blob(handle, key, data, length_in_out);
    nvs_close(handle);
    return err;
}

/* 保存 C 字符串；NVS 会连同结尾的 '\0' 一起存储。 */
esp_err_t Nvs_SetString(const char *key, const char *value)
{
    if (key == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    return Nvs_CommitAndClose(handle, nvs_set_str(handle, key, value));
}

/* 读取字符串；value 为 NULL 时 nvs_get_str() 只返回所需长度。 */
esp_err_t Nvs_GetString(const char *key, char *value, size_t *length_in_out)
{
    if (key == NULL || length_in_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(handle, key, value, length_in_out);
    nvs_close(handle);
    return err;
}

/* 删除单个键，并立即把删除结果提交到 Flash。 */
esp_err_t Nvs_EraseKey(const char *key)
{
    if (key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    return Nvs_CommitAndClose(handle, nvs_erase_key(handle, key));
}

/* 只清空比赛业务命名空间，不影响系统或其他模块的 NVS 数据。 */
esp_err_t Nvs_EraseAll(void)
{
    nvs_handle_t handle;
    esp_err_t err = Nvs_Open(NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    return Nvs_CommitAndClose(handle, nvs_erase_all(handle));
}
