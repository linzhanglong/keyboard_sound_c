/**
 * @file config_manager.c
 * @brief 配置管理器实现
 */

#define _GNU_SOURCE

#include "config/config_manager.h"
#include "config/config_loader.h"
#include "common/error.h"
#include "audio/audio_config_types.h"
#include "input/keymapping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==============================================================================
 * 配置管理器结构
 *============================================================================*/

struct ConfigManager {
    char config_dir[256];          /**< 配置文件目录 */

    AudioConfig audio_config;      /**< 音频配置 */
    TimbreConfig timbres[TIMBRE_COUNT]; /**< 音色配置 */
    KeyMapping* key_mapping;       /**< 按键映射 */

    bool audio_config_loaded;      /**< 音频配置是否已加载 */
    bool timbres_loaded;           /**< 音色配置是否已加载 */
    bool keymap_loaded;            /**< 按键映射是否已加载 */
};

/*==============================================================================
 * 默认配置
 *============================================================================*/

static void load_default_audio_config(AudioConfig* config) {
    config->sample_rate = 44100;
    config->default_volume = 0.3f;  // 降低默认音量，使声音更柔和
    strncpy(config->audio_device, "default", sizeof(config->audio_device) - 1);
    config->min_key_interval_ms = 50;
    config->backend = AUDIO_BACKEND_PULSEAUDIO;
}

static void load_default_timbres(TimbreConfig* timbres) {
    /* 钢琴 - 轻柔版本 */
    timbres[TIMBRE_PIANO].type = TIMBRE_PIANO;
    strcpy(timbres[TIMBRE_PIANO].name, "钢琴");
    timbres[TIMBRE_PIANO].harmonic_count = 6;
    timbres[TIMBRE_PIANO].harmonics[0] = (Harmonic){1.0f, 0.80f};  // 降低基频强度
    timbres[TIMBRE_PIANO].harmonics[1] = (Harmonic){2.0f, 0.15f};  // 大幅降低二次谐波
    timbres[TIMBRE_PIANO].harmonics[2] = (Harmonic){3.0f, 0.08f};  // 降低三次谐波
    timbres[TIMBRE_PIANO].harmonics[3] = (Harmonic){4.0f, 0.04f};  // 降低四次谐波
    timbres[TIMBRE_PIANO].harmonics[4] = (Harmonic){5.0f, 0.02f};  // 降低五次谐波
    timbres[TIMBRE_PIANO].harmonics[5] = (Harmonic){6.0f, 0.01f};  // 降低六次谐波
    timbres[TIMBRE_PIANO].attack = 0.020f;  // 稍慢起音，更柔和
    timbres[TIMBRE_PIANO].decay = 0.200f;   // 稍长衰减
    timbres[TIMBRE_PIANO].sustain = 0.60f;  // 降低持续音量
    timbres[TIMBRE_PIANO].release = 0.300f; // 稍长释音
    timbres[TIMBRE_PIANO].harmonic_decay_base = 2.0f;  // 更快的高频衰减
    timbres[TIMBRE_PIANO].vibrato_depth = 0.0f;
    timbres[TIMBRE_PIANO].vibrato_rate = 0.0f;

    /* 电子琴 - 使用钢琴基本音符 */
    timbres[TIMBRE_ELECTRIC].type = TIMBRE_ELECTRIC;
    strcpy(timbres[TIMBRE_ELECTRIC].name, "电子琴");
    timbres[TIMBRE_ELECTRIC].harmonic_count = 6;
    timbres[TIMBRE_ELECTRIC].harmonics[0] = (Harmonic){1.0f, 0.80f};  // 钢琴基频强度
    timbres[TIMBRE_ELECTRIC].harmonics[1] = (Harmonic){2.0f, 0.15f};  // 钢琴二次谐波
    timbres[TIMBRE_ELECTRIC].harmonics[2] = (Harmonic){3.0f, 0.08f};  // 钢琴三次谐波
    timbres[TIMBRE_ELECTRIC].harmonics[3] = (Harmonic){4.0f, 0.04f};  // 钢琴四次谐波
    timbres[TIMBRE_ELECTRIC].harmonics[4] = (Harmonic){5.0f, 0.02f};  // 钢琴五次谐波
    timbres[TIMBRE_ELECTRIC].harmonics[5] = (Harmonic){6.0f, 0.01f};  // 钢琴六次谐波
    timbres[TIMBRE_ELECTRIC].attack = 0.020f;  // 钢琴起音
    timbres[TIMBRE_ELECTRIC].decay = 0.200f;   // 钢琴衰减
    timbres[TIMBRE_ELECTRIC].sustain = 0.60f;  // 钢琴持续音量
    timbres[TIMBRE_ELECTRIC].release = 0.300f; // 钢琴释音
    timbres[TIMBRE_ELECTRIC].harmonic_decay_base = 2.0f;  // 钢琴高频衰减
    timbres[TIMBRE_ELECTRIC].vibrato_depth = 0.0f;
    timbres[TIMBRE_ELECTRIC].vibrato_rate = 0.0f;

    /* 钟琴 - 使用钢琴基本音符 */
    timbres[TIMBRE_BELL].type = TIMBRE_BELL;
    strcpy(timbres[TIMBRE_BELL].name, "钟琴");
    timbres[TIMBRE_BELL].harmonic_count = 6;
    timbres[TIMBRE_BELL].harmonics[0] = (Harmonic){1.0f, 0.80f};  // 钢琴基频强度
    timbres[TIMBRE_BELL].harmonics[1] = (Harmonic){2.0f, 0.15f};  // 钢琴二次谐波
    timbres[TIMBRE_BELL].harmonics[2] = (Harmonic){3.0f, 0.08f};  // 钢琴三次谐波
    timbres[TIMBRE_BELL].harmonics[3] = (Harmonic){4.0f, 0.04f};  // 钢琴四次谐波
    timbres[TIMBRE_BELL].harmonics[4] = (Harmonic){5.0f, 0.02f};  // 钢琴五次谐波
    timbres[TIMBRE_BELL].harmonics[5] = (Harmonic){6.0f, 0.01f};  // 钢琴六次谐波
    timbres[TIMBRE_BELL].attack = 0.020f;  // 钢琴起音
    timbres[TIMBRE_BELL].decay = 0.200f;   // 钢琴衰减
    timbres[TIMBRE_BELL].sustain = 0.60f;  // 钢琴持续音量
    timbres[TIMBRE_BELL].release = 0.300f; // 钢琴释音
    timbres[TIMBRE_BELL].harmonic_decay_base = 2.0f;  // 钢琴高频衰减
    timbres[TIMBRE_BELL].vibrato_depth = 0.0f;
    timbres[TIMBRE_BELL].vibrato_rate = 0.0f;

    /* 弦乐 - 使用钢琴基本音符 */
    timbres[TIMBRE_STRING].type = TIMBRE_STRING;
    strcpy(timbres[TIMBRE_STRING].name, "弦乐");
    timbres[TIMBRE_STRING].harmonic_count = 6;
    timbres[TIMBRE_STRING].harmonics[0] = (Harmonic){1.0f, 0.80f};  // 钢琴基频强度
    timbres[TIMBRE_STRING].harmonics[1] = (Harmonic){2.0f, 0.15f};  // 钢琴二次谐波
    timbres[TIMBRE_STRING].harmonics[2] = (Harmonic){3.0f, 0.08f};  // 钢琴三次谐波
    timbres[TIMBRE_STRING].harmonics[3] = (Harmonic){4.0f, 0.04f};  // 钢琴四次谐波
    timbres[TIMBRE_STRING].harmonics[4] = (Harmonic){5.0f, 0.02f};  // 钢琴五次谐波
    timbres[TIMBRE_STRING].harmonics[5] = (Harmonic){6.0f, 0.01f};  // 钢琴六次谐波
    timbres[TIMBRE_STRING].attack = 0.020f;  // 钢琴起音
    timbres[TIMBRE_STRING].decay = 0.200f;   // 钢琴衰减
    timbres[TIMBRE_STRING].sustain = 0.60f;  // 钢琴持续音量
    timbres[TIMBRE_STRING].release = 0.300f; // 钢琴释音
    timbres[TIMBRE_STRING].harmonic_decay_base = 2.0f;  // 钢琴高频衰减
    timbres[TIMBRE_STRING].vibrato_depth = 0.0f;  // 无颤音
    timbres[TIMBRE_STRING].vibrato_rate = 0.0f;   // 无颤音
}

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

ConfigManager* config_manager_create(const char* config_dir) {
    ConfigManager* manager = (ConfigManager*)calloc(1, sizeof(ConfigManager));
    if (!manager) {
        return NULL;
    }

    /* 设置配置目录 */
    if (config_dir) {
        strncpy(manager->config_dir, config_dir, sizeof(manager->config_dir) - 1);
    } else {
        strncpy(manager->config_dir, "configs", sizeof(manager->config_dir) - 1);
    }

    /* 创建默认按键映射 */
    manager->key_mapping = key_mapping_create_default(TIMBRE_PIANO);
    if (!manager->key_mapping) {
        free(manager);
        return NULL;
    }

    return manager;
}

void config_manager_destroy(ConfigManager* manager) {
    if (!manager) {
        return;
    }

    if (manager->key_mapping) {
        key_mapping_destroy(manager->key_mapping);
    }

    free(manager);
}

/*==============================================================================
 * 配置加载
 *============================================================================*/

ConfigResult config_manager_load_all(ConfigManager* manager) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    ConfigResult result;

    /* 加载音频配置 */
    result = config_manager_load_config(manager, NULL);
    if (result != CONFIG_SUCCESS) {
        fprintf(stderr, "Warning: Failed to load audio config: %s\n",
                config_result_to_string(result));
    }

    /* 加载音色配置 */
    result = config_manager_load_timbres(manager, NULL);
    if (result != CONFIG_SUCCESS) {
        fprintf(stderr, "Warning: Failed to load timbre config: %s\n",
                config_result_to_string(result));
    }

    /* 加载按键映射 */
    result = config_manager_load_keymap(manager, NULL);
    if (result != CONFIG_SUCCESS) {
        fprintf(stderr, "Warning: Failed to load keymap config: %s\n",
                config_result_to_string(result));
    }

    return CONFIG_SUCCESS;
}

ConfigResult config_manager_load_config(ConfigManager* manager, const char* path) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    ConfigLoader* loader = config_loader_create_ini(manager->config_dir);
    if (!loader) {
        return CONFIG_ERROR_OUT_OF_MEMORY;
    }

    ConfigResult result = config_loader_load_audio(loader, &manager->audio_config);
    if (result == CONFIG_SUCCESS) {
        manager->audio_config_loaded = true;
    } else {
        /* 加载失败时使用默认配置 */
        load_default_audio_config(&manager->audio_config);
        manager->audio_config_loaded = false;
    }

    config_loader_destroy(loader);
    return result;
}

ConfigResult config_manager_load_timbres(ConfigManager* manager, const char* path) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    /* 先加载默认配置，这样 harmonics 数组会被正确初始化，
     * 即使 INI 文件中没有定义 harmonics 参数 */
    load_default_timbres(manager->timbres);

    ConfigLoader* loader = config_loader_create_ini(manager->config_dir);
    if (!loader) {
        return CONFIG_ERROR_OUT_OF_MEMORY;
    }

    ConfigResult result = config_loader_load_timbres(loader, manager->timbres);
    if (result == CONFIG_SUCCESS) {
        manager->timbres_loaded = true;
    } else {
        /* 加载失败时使用默认配置（已经加载过了） */
        manager->timbres_loaded = false;
    }

    config_loader_destroy(loader);
    return result;
}

ConfigResult config_manager_load_keymap(ConfigManager* manager, const char* path) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    ConfigLoader* loader = config_loader_create_ini(manager->config_dir);
    if (!loader) {
        return CONFIG_ERROR_OUT_OF_MEMORY;
    }

    /* 销毁旧的按键映射 */
    if (manager->key_mapping) {
        key_mapping_destroy(manager->key_mapping);
    }

    /* 创建新的按键映射 */
    manager->key_mapping = key_mapping_create();
    if (!manager->key_mapping) {
        config_loader_destroy(loader);
        return CONFIG_ERROR_OUT_OF_MEMORY;
    }

    /* 加载按键映射配置 */
    ConfigResult result = config_loader_load_keymap(loader, manager->key_mapping);
    if (result == CONFIG_SUCCESS) {
        manager->keymap_loaded = true;
    } else {
        /* 加载失败时使用默认按键映射 */
        key_mapping_destroy(manager->key_mapping);
        manager->key_mapping = key_mapping_create_default(TIMBRE_PIANO);
        if (!manager->key_mapping) {
            config_loader_destroy(loader);
            return CONFIG_ERROR_OUT_OF_MEMORY;
        }
        manager->keymap_loaded = false;
    }

    config_loader_destroy(loader);
    return result;
}

ConfigResult config_manager_load_defaults(ConfigManager* manager) {
    if (!manager) {
        return CONFIG_ERROR_INVALID_PATH;
    }

    load_default_audio_config(&manager->audio_config);
    load_default_timbres(manager->timbres);

    manager->audio_config_loaded = true;
    manager->timbres_loaded = true;

    return CONFIG_SUCCESS;
}

/*==============================================================================
 * 配置访问
 *============================================================================*/

const AudioConfig* config_manager_get_audio_config(const ConfigManager* manager) {
    if (!manager) {
        return NULL;
    }
    return &manager->audio_config;
}

const TimbreConfig* config_manager_get_timbre(const ConfigManager* manager, TimbreType type) {
    if (!manager || type < 0 || type >= TIMBRE_COUNT) {
        return NULL;
    }
    return &manager->timbres[type];
}

const KeyMapping* config_manager_get_key_mapping(const ConfigManager* manager) {
    if (!manager) {
        return NULL;
    }
    return manager->key_mapping;
}

/*==============================================================================
 * 运行时配置
 *============================================================================*/

void config_manager_set_volume(ConfigManager* manager, float volume) {
    if (!manager) {
        return;
    }
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    manager->audio_config.default_volume = volume;
}

float config_manager_get_volume(const ConfigManager* manager) {
    if (!manager) {
        return 0.4f;
    }
    return manager->audio_config.default_volume;
}
