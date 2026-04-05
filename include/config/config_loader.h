/**
 * @file config_loader.h
 * @brief INI 配置加载器接口
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 前向声明
 *============================================================================*/

typedef struct ConfigLoader ConfigLoader;

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建 INI 配置加载器
 * @param config_dir 配置文件目录路径
 * @return 配置加载器指针，失败返回 NULL
 */
ConfigLoader* config_loader_create_ini(const char* config_dir);

/**
 * @brief 销毁配置加载器
 * @param loader 配置加载器指针
 */
void config_loader_destroy(ConfigLoader* loader);

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 加载音频配置
 * @param loader 配置加载器指针
 * @param output 输出缓冲区 (AudioConfig*)
 * @return 加载结果
 */
ConfigResult config_loader_load_audio(const ConfigLoader* loader, void* output);

/**
 * @brief 加载音色配置
 * @param loader 配置加载器指针
 * @param output 输出缓冲区 (TimbreConfig 数组)
 * @return 加载结果
 */
ConfigResult config_loader_load_timbres(const ConfigLoader* loader, void* output);

/**
 * @brief 加载按键映射配置
 * @param loader 配置加载器指针
 * @param output 输出缓冲区 (KeyMapping*)
 * @return 加载结果
 */
ConfigResult config_loader_load_keymap(const ConfigLoader* loader, void* output);

/*==============================================================================
 * 验证
 *============================================================================*/

/**
 * @brief 检查配置加载器是否有效
 * @param loader 配置加载器指针
 * @return true 如果有效
 */
bool config_loader_is_valid(const ConfigLoader* loader);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LOADER_H */
