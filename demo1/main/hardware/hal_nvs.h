#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief 初始化默认 NVS 分区。
 *
 * 必须先调用本函数，之后才能使用本文件中的其他接口。若检测到分区没有空闲页，
 * 或分区格式来自更新版本的 ESP-IDF，本函数会擦除默认 NVS 分区并重新初始化。
 * 数据统一保存在 config.h 的 NVS_NAMESPACE 命名空间中。
 *
 * @retval ESP_OK 初始化成功。
 * @retval ESP_ERR_INVALID_STATE 已经初始化，禁止重复调用。
 * @retval 其他值 NVS 分区初始化或恢复失败，返回对应 ESP-IDF 错误码。
 */
esp_err_t Nvs_Init(void);

/**
 * @brief 保存一个 32 位有符号整数并立即提交到 Flash。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[in] value 待保存的整数。
 *
 * @retval ESP_OK 写入并提交成功。
 * @retval ESP_ERR_INVALID_ARG key 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval 其他值 打开命名空间、写入或提交失败。
 */
esp_err_t Nvs_SetI32(const char *key, int32_t value);

/**
 * @brief 读取一个 32 位有符号整数。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[out] value_out 接收读取结果的地址，不能为 NULL。
 *
 * @retval ESP_OK 读取成功。
 * @retval ESP_ERR_INVALID_ARG key 或 value_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval ESP_ERR_NVS_NOT_FOUND 指定键不存在。
 * @retval ESP_ERR_NVS_TYPE_MISMATCH 指定键保存的不是 int32_t。
 * @retval 其他值 打开命名空间或读取失败。
 */
esp_err_t Nvs_GetI32(const char *key, int32_t *value_out);

/**
 * @brief 保存一段二进制数据并立即提交到 Flash。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[in] data 待保存数据的首地址，不能为 NULL。
 * @param[in] length 待保存数据的字节数，必须大于 0。
 *
 * @retval ESP_OK 写入并提交成功。
 * @retval ESP_ERR_INVALID_ARG 参数无效。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval 其他值 NVS 写入或提交失败。
 */
esp_err_t Nvs_SetBlob(const char *key, const void *data, size_t length);

/**
 * @brief 读取一段二进制数据，或查询保存数据所需的缓冲区长度。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[out] data 接收数据的缓冲区；传 NULL 时仅查询所需长度。
 * @param[in,out] length_in_out 调用前填写 data 的容量；返回时写入实际长度。
 *                              当 data 为 NULL 时，返回所需缓冲区长度。
 *
 * @retval ESP_OK 读取成功或长度查询成功。
 * @retval ESP_ERR_INVALID_ARG key 或 length_in_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval ESP_ERR_NVS_INVALID_LENGTH 缓冲区容量不足，同时返回所需长度。
 * @retval ESP_ERR_NVS_NOT_FOUND 指定键不存在。
 * @retval 其他值 打开命名空间或读取失败。
 */
esp_err_t Nvs_GetBlob(const char *key, void *data, size_t *length_in_out);

/**
 * @brief 保存一个以 '\0' 结尾的字符串并立即提交到 Flash。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[in] value 待保存的 C 字符串，不能为 NULL。
 *
 * @retval ESP_OK 写入并提交成功。
 * @retval ESP_ERR_INVALID_ARG key 或 value 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval 其他值 NVS 写入或提交失败。
 */
esp_err_t Nvs_SetString(const char *key, const char *value);

/**
 * @brief 读取字符串，或查询包含结尾 '\0' 在内的所需缓冲区长度。
 *
 * @param[in] key 键名，不能为空且长度不能超过 15 个字符。
 * @param[out] value 接收字符串的缓冲区；传 NULL 时仅查询所需长度。
 * @param[in,out] length_in_out 调用前填写 value 的容量；返回时写入包含 '\0' 的实际长度。
 *
 * @retval ESP_OK 读取成功或长度查询成功。
 * @retval ESP_ERR_INVALID_ARG key 或 length_in_out 为 NULL。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval ESP_ERR_NVS_INVALID_LENGTH 缓冲区容量不足，同时返回所需长度。
 * @retval ESP_ERR_NVS_NOT_FOUND 指定键不存在。
 * @retval 其他值 打开命名空间或读取失败。
 */
esp_err_t Nvs_GetString(const char *key, char *value, size_t *length_in_out);

/**
 * @brief 删除指定键并立即提交到 Flash。
 *
 * @param[in] key 要删除的键名，不能为 NULL。
 *
 * @retval ESP_OK 删除并提交成功。
 * @retval ESP_ERR_INVALID_ARG key 为 NULL。
 * @retval ESP_ERR_NVS_NOT_FOUND 指定键不存在。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval 其他值 删除或提交失败。
 */
esp_err_t Nvs_EraseKey(const char *key);

/**
 * @brief 删除 NVS_NAMESPACE 命名空间内的全部键并立即提交。
 *
 * 本函数不会擦除其他命名空间，也不会擦除整个 NVS 分区。
 *
 * @retval ESP_OK 删除并提交成功。
 * @retval ESP_ERR_INVALID_STATE 尚未调用 Nvs_Init()。
 * @retval 其他值 打开命名空间、删除或提交失败。
 */
esp_err_t Nvs_EraseAll(void);
