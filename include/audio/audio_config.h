/**
 * @file audio_config.h
 * @brief 音频配置管理器接口
 */

#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include "audio_config_types.h"
#include "common/types.h"
#include "input/keymapping.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 音频配置常量
 *============================================================================*/

#define SAMPLE_RATE         44100  /**< 采样率 (Hz) */

/* ADSR 默认参数 */
#define ADSR_ATTACK_TIME    0.008f  /**< 默认起音时间 (秒) */
#define ADSR_DECAY_TIME     0.12f   /**< 默认衰减时间 (秒) */
#define ADSR_SUSTAIN_LEVEL  0.65f   /**< 默认持续音量 */
#define ADSR_RELEASE_TIME   0.18f   /**< 默认释音时间 (秒) */

/*==============================================================================
 * 音频配置管理器
 *============================================================================*/

typedef struct AudioConfigManager AudioConfigManager;

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建音频配置管理器
 * @param config_dir 配置文件目录，NULL 使用默认 "configs"
 * @return 音频配置管理器指针，失败返回 NULL
 */
AudioConfigManager* audio_config_manager_create(const char* config_dir);

/**
 * @brief 销毁音频配置管理器
 * @param manager 音频配置管理器指针
 */
void audio_config_manager_destroy(AudioConfigManager* manager);

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 加载默认配置
 * @param manager 音频配置管理器指针
 * @return 配置加载结果
 */
ConfigResult audio_config_manager_load_defaults(AudioConfigManager* manager);

/*==============================================================================
 * 配置访问
 *============================================================================*/

/**
 * @brief 获取音频配置
 * @param manager 音频配置管理器指针
 * @return 音频配置指针，失败返回 NULL
 */
const AudioConfig* audio_config_manager_get_audio_config(const AudioConfigManager* manager);

/**
 * @brief 获取音色配置
 * @param manager 音频配置管理器指针
 * @param type 音色类型
 * @return 音色配置指针，失败返回 NULL
 */
const TimbreConfig* audio_config_manager_get_timbre(const AudioConfigManager* manager, TimbreType type);

/**
 * @brief 根据名称获取音色配置
 * @param manager 音频配置管理器指针
 * @param name 音色名称
 * @return 音色配置指针，失败返回 NULL
 */
const TimbreConfig* audio_config_manager_get_timbre_by_name(const AudioConfigManager* manager, const char* name);

/**
 * @brief 获取按键映射
 * @param manager 音频配置管理器指针
 * @return 按键映射指针，失败返回 NULL
 */
const KeyMapping* audio_config_manager_get_key_mapping(const AudioConfigManager* manager);

/*==============================================================================
 * 运行时配置
 *============================================================================*/

/**
 * @brief 设置音量
 * @param manager 音频配置管理器指针
 * @param volume 音量 (0.0 ~ 1.0)
 */
void audio_config_manager_set_volume(AudioConfigManager* manager, float volume);

/**
 * @brief 获取音量
 * @param manager 音频配置管理器指针
 * @return 音量 (0.0 ~ 1.0)
 */
float audio_config_manager_get_volume(const AudioConfigManager* manager);

/**
 * @brief 检查配置是否已加载
 * @param manager 音频配置管理器指针
 * @return true 如果配置已加载
 */
bool audio_config_manager_is_loaded(const AudioConfigManager* manager);

/*==============================================================================
 * 音色配置访问 (兼容性接口)
 *============================================================================*/

/**
 * @brief 获取音色配置
 * @param type 音色类型
 * @return 音色配置指针
 */
const TimbreConfig* timbre_get_config(TimbreType type);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_CONFIG_H */
