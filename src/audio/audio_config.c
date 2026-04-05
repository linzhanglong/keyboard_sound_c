/**
 * @file audio_config.c
 * @brief 音频配置管理实现
 */

#define _GNU_SOURCE

#include "audio/audio_config.h"
#include "config/config_manager.h"
#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==============================================================================
 * 音频配置管理器结构
 *============================================================================*/

struct AudioConfigManager {
    ConfigManager* config_manager;
    AudioConfig audio_config;
    TimbreConfig timbres[TIMBRE_COUNT];
    bool config_loaded;
};

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建音频配置管理器
 * @param config_dir 配置文件目录路径
 * @return 成功返回管理器指针，失败返回NULL
 * @note 自动加载配置文件，失败时使用默认配置
 */
AudioConfigManager* audio_config_manager_create(const char* config_dir) {
    AudioConfigManager* manager = (AudioConfigManager*)calloc(1, sizeof(AudioConfigManager));
    if (!manager) {
        return NULL;
    }

    /* 创建配置管理器 */
    manager->config_manager = config_manager_create(config_dir);
    if (!manager->config_manager) {
        free(manager);
        return NULL;
    }

    /* 加载配置 */
    ConfigResult result = config_manager_load_all(manager->config_manager);
    if (result == CONFIG_SUCCESS) {
        manager->config_loaded = true;

        /* 获取音频配置 */
        const AudioConfig* audio_config = config_manager_get_audio_config(manager->config_manager);
        if (audio_config) {
            memcpy(&manager->audio_config, audio_config, sizeof(AudioConfig));
        }

        /* 获取音色配置 */
        for (int i = 0; i < TIMBRE_COUNT; i++) {
            const TimbreConfig* timbre = config_manager_get_timbre(manager->config_manager, (TimbreType)i);
            if (timbre) {
                memcpy(&manager->timbres[i], timbre, sizeof(TimbreConfig));
            }
        }
    } else {
        /* 加载失败，使用默认配置 */
        audio_config_manager_load_defaults(manager);
    }

    return manager;
}

/**
 * @brief 销毁音频配置管理器
 * @param manager 音频配置管理器指针
 * @note 释放管理器占用的所有内存资源
 */
void audio_config_manager_destroy(AudioConfigManager* manager) {
    if (!manager) {
        return;
    }

    if (manager->config_manager) {
        config_manager_destroy(manager->config_manager);
    }

    free(manager);
}

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 加载默认音频配置
 * @param manager 配置管理器指针
 * @return 配置加载结果
 * @note 当配置文件加载失败时调用此函数
 */
ConfigResult audio_config_manager_load_defaults(AudioConfigManager* manager) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    ConfigResult result = config_manager_load_defaults(manager->config_manager);
    if (result == CONFIG_SUCCESS) {
        /* 获取默认配置 */
        const AudioConfig* audio_config = config_manager_get_audio_config(manager->config_manager);
        if (audio_config) {
            memcpy(&manager->audio_config, audio_config, sizeof(AudioConfig));
        }

        /* 获取默认音色配置 */
        for (int i = 0; i < TIMBRE_COUNT; i++) {
            const TimbreConfig* timbre = config_manager_get_timbre(manager->config_manager, (TimbreType)i);
            if (timbre) {
                memcpy(&manager->timbres[i], timbre, sizeof(TimbreConfig));
            }
        }

        manager->config_loaded = true;
    }

    return result;
}

/*==============================================================================
 * 配置访问
 *============================================================================*/

/**
 * @brief 获取音频配置
 * @param manager 音频配置管理器指针
 * @return 音频配置指针，失败返回NULL
 */
const AudioConfig* audio_config_manager_get_audio_config(const AudioConfigManager* manager) {
    if (!manager) {
        return NULL;
    }
    return &manager->audio_config;
}

/**
 * @brief 获取指定类型的音色配置
 * @param manager 音频配置管理器指针
 * @param type 音色类型
 * @return 音色配置指针，类型无效返回NULL
 */
const TimbreConfig* audio_config_manager_get_timbre(const AudioConfigManager* manager, TimbreType type) {
    if (!manager || type < 0 || type >= TIMBRE_COUNT) {
        return NULL;
    }
    return &manager->timbres[type];
}

/**
 * @brief 根据名称获取音色配置
 * @param manager 音频配置管理器指针
 * @param name 音色名称
 * @return 音色配置指针，未找到返回NULL
 */
const TimbreConfig* audio_config_manager_get_timbre_by_name(const AudioConfigManager* manager, const char* name) {
    if (!manager || !name) {
        return NULL;
    }

    /* 简化的名称匹配 */
    for (int i = 0; i < TIMBRE_COUNT; i++) {
        if (strcmp(manager->timbres[i].name, name) == 0) {
            return &manager->timbres[i];
        }
    }

    return NULL;
}

/**
 * @brief 获取按键映射配置
 * @param manager 音频配置管理器指针
 * @return 按键映射指针，失败返回NULL
 */
const KeyMapping* audio_config_manager_get_key_mapping(const AudioConfigManager* manager) {
    if (!manager) {
        return NULL;
    }
    return config_manager_get_key_mapping(manager->config_manager);
}

/*==============================================================================
 * 运行时配置
 *============================================================================*/

/**
 * @brief 设置默认音量
 * @param manager 音频配置管理器指针
 * @param volume 音量值 (0.0 - 1.0)
 * @note 自动限制音量在有效范围内
 */
void audio_config_manager_set_volume(AudioConfigManager* manager, float volume) {
    if (!manager) {
        return;
    }

    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    manager->audio_config.default_volume = volume;

    /* 同步到配置管理器 */
    if (manager->config_manager) {
        config_manager_set_volume(manager->config_manager, volume);
    }
}

/**
 * @brief 获取默认音量
 * @param manager 音频配置管理器指针
 * @return 默认音量值，默认0.4
 */
float audio_config_manager_get_volume(const AudioConfigManager* manager) {
    if (!manager) {
        return 0.4f;
    }
    return manager->audio_config.default_volume;
}

/**
 * @brief 检查配置是否已加载
 * @param manager 配置管理器指针
 * @return 配置已加载返回true，否则返回false
 */
bool audio_config_manager_is_loaded(const AudioConfigManager* manager) {
    if (!manager) {
        return false;
    }
    return manager->config_loaded;
}

/*==============================================================================
 * 音色配置访问 (兼容性接口)
 *============================================================================*/

/**
 * @brief 获取音色配置
 * @param type 音色类型
 * @return 音色配置指针，如果类型无效返回 NULL
 */
const TimbreConfig* timbre_get_config(TimbreType type)
{
    if (type < 0 || type >= TIMBRE_COUNT) {
        return NULL;
    }

    /* 使用全局配置管理器获取音色配置 */
    /* 注意：这需要全局配置管理器实例，这里返回默认配置 */
    static TimbreConfig default_timbres[TIMBRE_COUNT];
    static bool initialized = false;

    if (!initialized) {
        /* 初始化默认音色配置 */
        for (int i = 0; i < TIMBRE_COUNT; i++) {
            default_timbres[i].type = (TimbreType)i;
            strcpy(default_timbres[i].name, "Default");
            default_timbres[i].harmonic_count = 0;
            default_timbres[i].attack = ADSR_ATTACK_TIME;
            default_timbres[i].decay = ADSR_DECAY_TIME;
            default_timbres[i].sustain = ADSR_SUSTAIN_LEVEL;
            default_timbres[i].release = ADSR_RELEASE_TIME;
            default_timbres[i].harmonic_decay_base = 1.0f;
            default_timbres[i].vibrato_depth = 0.0f;
            default_timbres[i].vibrato_rate = 0.0f;
        }
        initialized = true;
    }

    return &default_timbres[type];
}
