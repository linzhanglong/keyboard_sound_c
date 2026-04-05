/**
 * @file config_manager.h
 * @brief 配置管理器接口
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "common/types.h"
#include "audio/audio_config_types.h"
#include "input/keymapping.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 前向声明
 *============================================================================*/

typedef struct ConfigManager ConfigManager;

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建配置管理器
 * @param config_dir 配置文件目录路径，NULL 使用默认路径
 * @return 配置管理器指针，失败返回 NULL
 */
ConfigManager* config_manager_create(const char* config_dir);

/**
 * @brief 销毁配置管理器
 * @param manager 配置管理器指针
 */
void config_manager_destroy(ConfigManager* manager);

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 加载所有配置文件
 * @param manager 配置管理器指针
 * @return 加载结果
 */
ConfigResult config_manager_load_all(ConfigManager* manager);

/**
 * @brief 加载主配置文件
 * @param manager 配置管理器指针
 * @param path 配置文件路径，NULL 使用默认路径
 * @return 加载结果
 */
ConfigResult config_manager_load_config(ConfigManager* manager, const char* path);

/**
 * @brief 加载音色配置文件
 * @param manager 配置管理器指针
 * @param path 配置文件路径，NULL 使用默认路径
 * @return 加载结果
 */
ConfigResult config_manager_load_timbres(ConfigManager* manager, const char* path);

/**
 * @brief 加载按键映射配置文件
 * @param manager 配置管理器指针
 * @param path 配置文件路径，NULL 使用默认路径
 * @return 加载结果
 */
ConfigResult config_manager_load_keymap(ConfigManager* manager, const char* path);

/**
 * @brief 加载默认配置（当文件加载失败时使用）
 * @param manager 配置管理器指针
 * @return 加载结果
 */
ConfigResult config_manager_load_defaults(ConfigManager* manager);

/*==============================================================================
 * 配置访问
 *============================================================================*/

/**
 * @brief 获取音频配置
 * @param manager 配置管理器指针
 * @return 音频配置指针
 */
const AudioConfig* config_manager_get_audio_config(const ConfigManager* manager);

/**
 * @brief 获取音色配置
 * @param manager 配置管理器指针
 * @param type 音色类型
 * @return 音色配置指针，类型无效返回 NULL
 */
const TimbreConfig* config_manager_get_timbre(const ConfigManager* manager, TimbreType type);

/**
 * @brief 获取按键映射
 * @param manager 配置管理器指针
 * @return 按键映射指针
 */
const KeyMapping* config_manager_get_key_mapping(const ConfigManager* manager);

/*==============================================================================
 * 运行时配置
 *============================================================================*/

/**
 * @brief 设置音量
 * @param manager 配置管理器指针
 * @param volume 音量 (0.0 ~ 1.0)
 */
void config_manager_set_volume(ConfigManager* manager, float volume);

/**
 * @brief 获取音量
 * @param manager 配置管理器指针
 * @return 当前音量
 */
float config_manager_get_volume(const ConfigManager* manager);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_MANAGER_H */
