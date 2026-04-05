/**
 * @file config_loader_ini.c
 * @brief INI 配置加载器实现
 */

#define _GNU_SOURCE

#include "config/config_loader.h"
#include "common/error.h"
#include "input/keymapping.h"
#include "audio/audio_config_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*==============================================================================
 * INI 解析器结构
 *============================================================================*/

typedef struct {
    char* config_dir;
    char config_path[512];
} IniConfigLoader;

typedef struct {
    char* key;
    char* value;
} IniKeyValue;

typedef struct {
    char* section;
    IniKeyValue* items;
    size_t count;
    size_t capacity;
} IniSection;

typedef struct {
    IniSection* sections;
    size_t count;
    size_t capacity;
} IniFile;

/*==============================================================================
 * INI 文件解析
 *============================================================================*/

/**
 * @brief 去除字符串首尾的空白字符
 * @param str 输入字符串指针
 * @return 去除空白后的字符串指针
 * @note 修改原字符串，返回指向去除前导空白后位置的指针
 */
static char* trim_whitespace(char* str) {
    if (!str) return NULL;
    while (*str && isspace((unsigned char)*str)) str++;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) *end-- = '\0';
    return str;
}

/**
 * @brief 解析 INI 文件的一行
 * @param line 输入行字符串
 * @param section 输出 section 名称指针
 * @param key 输出键名指针
 * @param value 输出键值指针
 * @return 解析结果: 0=空行/注释, 1=section, 2=键值对
 * @note 修改 line 字符串，设置输出指针指向解析结果
 */
static int parse_ini_line(char* line, char** section, char** key, char** value) {
    line = trim_whitespace(line);
    if (!line || *line == '\0' || *line == ';' || *line == '#') {
        return 0;  // 空行或注释
    }

    // 检查是否是 [section] 行
    if (*line == '[') {
        char* end = strchr(line, ']');
        if (end) {
            *end = '\0';
            *section = trim_whitespace(line + 1);  // 也要 trim section 名称
            return 1;  // 新 section
        }
    }

    // 检查是否是 key=value 行
    char* equals = strchr(line, '=');
    if (equals) {
        *equals = '\0';
        *key = trim_whitespace(line);
        *value = trim_whitespace(equals + 1);
        return 2;  // 键值对
    }

    return 0;
}

static IniFile* ini_file_create(void) {
    IniFile* ini = (IniFile*)calloc(1, sizeof(IniFile));
    return ini;
}

static void ini_file_destroy(IniFile* ini) {
    if (!ini) return;

    for (size_t i = 0; i < ini->count; i++) {
        free(ini->sections[i].section);
        for (size_t j = 0; j < ini->sections[i].count; j++) {
            free(ini->sections[i].items[j].key);
            free(ini->sections[i].items[j].value);
        }
        free(ini->sections[i].items);
    }
    free(ini->sections);
    free(ini);
}

static IniSection* ini_file_get_section(IniFile* ini, const char* name) {
    if (!ini || !name) return NULL;

    for (size_t i = 0; i < ini->count; i++) {
        if (strcmp(ini->sections[i].section, name) == 0) {
            return &ini->sections[i];
        }
    }
    return NULL;
}

static const char* ini_section_get_value(IniSection* section, const char* key) {
    if (!section || !key) return NULL;

    for (size_t i = 0; i < section->count; i++) {
        if (strcmp(section->items[i].key, key) == 0) {
            return section->items[i].value;
        }
    }
    return NULL;
}

static IniSection* ini_file_add_section(IniFile* ini, const char* name) {
    if (!ini || !name) return NULL;

    // 扩展数组
    if (ini->count >= ini->capacity) {
        size_t new_capacity = ini->capacity == 0 ? 8 : ini->capacity * 2;
        IniSection* new_sections = (IniSection*)realloc(ini->sections, new_capacity * sizeof(IniSection));
        if (!new_sections) return NULL;
        ini->sections = new_sections;
        ini->capacity = new_capacity;
    }

    IniSection* section = &ini->sections[ini->count++];
    memset(section, 0, sizeof(IniSection));
    section->section = strdup(name);
    return section;
}

static int ini_section_add_item(IniSection* section, const char* key, const char* value) {
    if (!section || !key || !value) return -1;

    // 扩展数组
    if (section->count >= section->capacity) {
        size_t new_capacity = section->capacity == 0 ? 8 : section->capacity * 2;
        IniKeyValue* new_items = (IniKeyValue*)realloc(section->items, new_capacity * sizeof(IniKeyValue));
        if (!new_items) return -1;
        section->items = new_items;
        section->capacity = new_capacity;
    }

    IniKeyValue* item = &section->items[section->count++];
    item->key = strdup(key);
    item->value = strdup(value);
    return 0;
}

static IniFile* ini_file_parse(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) return NULL;

    IniFile* ini = ini_file_create();
    if (!ini) {
        fclose(fp);
        return NULL;
    }

    char line[512];
    char current_section[256] = "";
    IniSection* current_section_ptr = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char* section_name = NULL;
        char* key = NULL;
        char* value = NULL;

        int result = parse_ini_line(line, &section_name, &key, &value);

        if (result == 1) {
            // 新 section
            strncpy(current_section, section_name, sizeof(current_section) - 1);
            current_section_ptr = ini_file_add_section(ini, section_name);
        } else if (result == 2 && current_section_ptr) {
            // 键值对
            ini_section_add_item(current_section_ptr, key, value);
        }
    }

    fclose(fp);
    return ini;
}

/*==============================================================================
 * 配置加载器实现
 *============================================================================*/

ConfigLoader* config_loader_create_ini(const char* config_dir) {
    IniConfigLoader* loader = (IniConfigLoader*)calloc(1, sizeof(IniConfigLoader));
    if (!loader) return NULL;

    if (config_dir) {
        loader->config_dir = strdup(config_dir);
    } else {
        loader->config_dir = strdup("configs");
    }

    return (ConfigLoader*)loader;
}

void config_loader_destroy(ConfigLoader* loader) {
    if (!loader) return;
    IniConfigLoader* ini_loader = (IniConfigLoader*)loader;
    free(ini_loader->config_dir);
    free(ini_loader);
}

bool config_loader_is_valid(const ConfigLoader* loader) {
    return loader != NULL;
}

/*==============================================================================
 * 音频配置加载
 *============================================================================*/

ConfigResult config_loader_load_audio(const ConfigLoader* loader, void* output) {
    if (!loader || !output) return CONFIG_ERROR_INVALID_PATH;

    IniConfigLoader* ini_loader = (IniConfigLoader*)loader;
    AudioConfig* audio_config = (AudioConfig*)output;

    // 构建配置文件路径
    snprintf(ini_loader->config_path, sizeof(ini_loader->config_path),
             "%s/config.ini", ini_loader->config_dir);

    IniFile* ini = ini_file_parse(ini_loader->config_path);
    if (!ini) {
        return CONFIG_ERROR_FILE_NOT_FOUND;
    }

    // 解析 [audio] 部分
    IniSection* audio_section = ini_file_get_section(ini, "audio");
    if (audio_section) {
        const char* value;

        value = ini_section_get_value(audio_section, "sample_rate");
        if (value) audio_config->sample_rate = atoi(value);

        value = ini_section_get_value(audio_section, "default_volume");
        if (value) audio_config->default_volume = atof(value);

        value = ini_section_get_value(audio_section, "audio_device");
        if (value) strncpy(audio_config->audio_device, value, sizeof(audio_config->audio_device) - 1);

        value = ini_section_get_value(audio_section, "min_key_interval_ms");
        if (value) audio_config->min_key_interval_ms = atoi(value);
    }

    // 解析 [adsr] 部分
    IniSection* adsr_section = ini_file_get_section(ini, "adsr");
    if (adsr_section) {
        const char* value;

        value = ini_section_get_value(adsr_section, "attack_ms");
        if (value) audio_config->adsr.attack = atof(value) / 1000.0f;

        value = ini_section_get_value(adsr_section, "decay_ms");
        if (value) audio_config->adsr.decay = atof(value) / 1000.0f;

        value = ini_section_get_value(adsr_section, "sustain_level");
        if (value) audio_config->adsr.sustain = atof(value);

        value = ini_section_get_value(adsr_section, "release_ms");
        if (value) audio_config->adsr.release = atof(value) / 1000.0f;
    }

    // 解析 [reverb] 部分
    IniSection* reverb_section = ini_file_get_section(ini, "reverb");
    if (reverb_section) {
        const char* value;

        value = ini_section_get_value(reverb_section, "enabled");
        if (value) audio_config->reverb.enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);

        value = ini_section_get_value(reverb_section, "room_size");
        if (value) audio_config->reverb.room_size = atof(value);

        value = ini_section_get_value(reverb_section, "damp");
        if (value) audio_config->reverb.damp = atof(value);

        value = ini_section_get_value(reverb_section, "wet");
        if (value) audio_config->reverb.wet = atof(value);
    }

    ini_file_destroy(ini);
    return CONFIG_SUCCESS;
}

/*==============================================================================
 * 音色配置加载
 *============================================================================*/

ConfigResult config_loader_load_timbres(const ConfigLoader* loader, void* output) {
    if (!loader || !output) return CONFIG_ERROR_INVALID_PATH;

    IniConfigLoader* ini_loader = (IniConfigLoader*)loader;
    TimbreConfig* timbres = (TimbreConfig*)output;

    snprintf(ini_loader->config_path, sizeof(ini_loader->config_path),
             "%s/timbres.ini", ini_loader->config_dir);

    IniFile* ini = ini_file_parse(ini_loader->config_path);
    if (!ini) {
        return CONFIG_ERROR_FILE_NOT_FOUND;
    }

    // 音色类型映射
    const char* timbre_names[] = { "piano", "electric", "bell", "string" };

    for (int i = 0; i < TIMBRE_COUNT; i++) {
        IniSection* section = ini_file_get_section(ini, timbre_names[i]);
        if (section) {
            timbres[i].type = (TimbreType)i;
            strcpy(timbres[i].name, timbre_names[i]);

            const char* value;

            value = ini_section_get_value(section, "attack_ms");
            if (value) timbres[i].attack = atof(value) / 1000.0f;

            value = ini_section_get_value(section, "decay_ms");
            if (value) timbres[i].decay = atof(value) / 1000.0f;

            value = ini_section_get_value(section, "sustain");
            if (value) timbres[i].sustain = atof(value);

            value = ini_section_get_value(section, "release_ms");
            if (value) timbres[i].release = atof(value) / 1000.0f;

            value = ini_section_get_value(section, "harmonic_count");
            if (value) timbres[i].harmonic_count = atoi(value);

            value = ini_section_get_value(section, "harmonic_decay_base");
            if (value) timbres[i].harmonic_decay_base = atof(value);

            value = ini_section_get_value(section, "vibrato_depth");
            if (value) timbres[i].vibrato_depth = atof(value);

            value = ini_section_get_value(section, "vibrato_rate");
            if (value) timbres[i].vibrato_rate = atof(value);
        }
    }

    ini_file_destroy(ini);
    return CONFIG_SUCCESS;
}

/*==============================================================================
 * 按键映射配置加载
 *============================================================================*/

ConfigResult config_loader_load_keymap(const ConfigLoader* loader, void* output) {
    if (!loader || !output) return CONFIG_ERROR_INVALID_PATH;

    IniConfigLoader* ini_loader = (IniConfigLoader*)loader;
    KeyMapping* mapping = (KeyMapping*)output;

    snprintf(ini_loader->config_path, sizeof(ini_loader->config_path),
             "%s/keymap.ini", ini_loader->config_dir);

    IniFile* ini = ini_file_parse(ini_loader->config_path);
    if (!ini) {
        return CONFIG_ERROR_FILE_NOT_FOUND;
    }

    // 计算映射数量
    size_t count = 0;
    for (size_t i = 0; i < ini->count; i++) {
        count += ini->sections[i].count;
    }

    if (count > 0) {
        mapping->mappings = (KeyNoteMap*)malloc(count * sizeof(KeyNoteMap));
        if (!mapping->mappings) {
            ini_file_destroy(ini);
            return CONFIG_ERROR_ALLOC_FAILED;
        }

        mapping->capacity = count;
        mapping->count = 0;

        // 解析每个 section 的按键映射
        for (size_t i = 0; i < ini->count; i++) {
            IniSection* section = &ini->sections[i];

            for (size_t j = 0; j < section->count; j++) {
                IniKeyValue* item = &section->items[j];

                // 解析按键名称到 KeyCode
                KeyCode code = KEY_UNKNOWN;
                if (strcmp(item->key, "A") == 0) code = KEY_A;
                else if (strcmp(item->key, "S") == 0) code = KEY_S;
                else if (strcmp(item->key, "D") == 0) code = KEY_D;
                else if (strcmp(item->key, "F") == 0) code = KEY_F;
                else if (strcmp(item->key, "G") == 0) code = KEY_G;
                else if (strcmp(item->key, "H") == 0) code = KEY_H;
                else if (strcmp(item->key, "J") == 0) code = KEY_J;
                else if (strcmp(item->key, "K") == 0) code = KEY_K;
                else if (strcmp(item->key, "L") == 0) code = KEY_L;
                else if (strcmp(item->key, "Z") == 0) code = KEY_Z;
                else if (strcmp(item->key, "X") == 0) code = KEY_X;
                else if (strcmp(item->key, "C") == 0) code = KEY_C;
                else if (strcmp(item->key, "V") == 0) code = KEY_V;
                else if (strcmp(item->key, "B") == 0) code = KEY_B;
                else if (strcmp(item->key, "N") == 0) code = KEY_N;
                else if (strcmp(item->key, "M") == 0) code = KEY_M;
                else if (strcmp(item->key, "Q") == 0) code = KEY_Q;
                else if (strcmp(item->key, "W") == 0) code = KEY_W;
                else if (strcmp(item->key, "E") == 0) code = KEY_E;
                else if (strcmp(item->key, "R") == 0) code = KEY_R;
                else if (strcmp(item->key, "T") == 0) code = KEY_T;
                else if (strcmp(item->key, "Y") == 0) code = KEY_Y;
                else if (strcmp(item->key, "U") == 0) code = KEY_U;
                else if (strcmp(item->key, "I") == 0) code = KEY_I;
                else if (strcmp(item->key, "O") == 0) code = KEY_O;
                else if (strcmp(item->key, "P") == 0) code = KEY_P;
                else if (strcmp(item->key, "1") == 0) code = KEY_1;
                else if (strcmp(item->key, "2") == 0) code = KEY_2;
                else if (strcmp(item->key, "3") == 0) code = KEY_3;
                else if (strcmp(item->key, "4") == 0) code = KEY_4;
                else if (strcmp(item->key, "5") == 0) code = KEY_5;
                else if (strcmp(item->key, "6") == 0) code = KEY_6;
                else if (strcmp(item->key, "7") == 0) code = KEY_7;
                else if (strcmp(item->key, "8") == 0) code = KEY_8;
                else if (strcmp(item->key, "9") == 0) code = KEY_9;
                else if (strcmp(item->key, "0") == 0) code = KEY_0;
                else if (strcmp(item->key, "SEMICOLON") == 0) code = KEY_SEMICOLON;
                else if (strcmp(item->key, "APOSTROPHE") == 0) code = KEY_APOSTROPHE;

                if (code != KEY_UNKNOWN) {
                    KeyNoteMap* note_map = &mapping->mappings[mapping->count++];
                    note_map->code = code;
                    note_map->frequency = atof(item->value);

                    /* 所有音色都使用钢琴基本音符 */
                    note_map->timbre = TIMBRE_PIANO;

                    /* 初始化描述字段 */
                    strcpy(note_map->desc, "");

                    /* 根据频率确定音符名称 */
                    float freq = atof(item->value);
                    if (freq >= 261.63f && freq < 277.18f) strcpy(note_map->note, "C4");
                    else if (freq >= 277.18f && freq < 293.66f) strcpy(note_map->note, "C#4");
                    else if (freq >= 293.66f && freq < 311.13f) strcpy(note_map->note, "D4");
                    else if (freq >= 311.13f && freq < 329.63f) strcpy(note_map->note, "D#4");
                    else if (freq >= 329.63f && freq < 349.23f) strcpy(note_map->note, "E4");
                    else if (freq >= 349.23f && freq < 369.99f) strcpy(note_map->note, "F4");
                    else if (freq >= 369.99f && freq < 392.00f) strcpy(note_map->note, "F#4");
                    else if (freq >= 392.00f && freq < 415.30f) strcpy(note_map->note, "G4");
                    else if (freq >= 415.30f && freq < 440.00f) strcpy(note_map->note, "G#4");
                    else if (freq >= 440.00f && freq < 466.16f) strcpy(note_map->note, "A4");
                    else if (freq >= 466.16f && freq < 493.88f) strcpy(note_map->note, "A#4");
                    else if (freq >= 493.88f && freq < 523.25f) strcpy(note_map->note, "B4");
                    else if (freq >= 523.25f && freq < 554.37f) strcpy(note_map->note, "C5");
                    else if (freq >= 554.37f && freq < 587.33f) strcpy(note_map->note, "C#5");
                    else if (freq >= 587.33f && freq < 622.25f) strcpy(note_map->note, "D5");
                    else if (freq >= 622.25f && freq < 659.25f) strcpy(note_map->note, "D#5");
                    else if (freq >= 659.25f && freq < 698.46f) strcpy(note_map->note, "E5");
                    else if (freq >= 698.46f && freq < 739.99f) strcpy(note_map->note, "F5");
                    else if (freq >= 739.99f && freq < 783.99f) strcpy(note_map->note, "F#5");
                    else if (freq >= 783.99f && freq < 830.61f) strcpy(note_map->note, "G5");
                    else if (freq >= 830.61f && freq < 880.00f) strcpy(note_map->note, "G#5");
                    else if (freq >= 880.00f && freq < 932.33f) strcpy(note_map->note, "A5");
                    else if (freq >= 932.33f && freq < 987.77f) strcpy(note_map->note, "A#5");
                    else if (freq >= 987.77f && freq < 1046.50f) strcpy(note_map->note, "B5");
                    else if (freq >= 1046.50f && freq < 1108.73f) strcpy(note_map->note, "C6");
                    else if (freq >= 1108.73f && freq < 1174.66f) strcpy(note_map->note, "C#6");
                    else if (freq >= 1174.66f && freq < 1244.51f) strcpy(note_map->note, "D6");
                    else if (freq >= 1244.51f && freq < 1318.51f) strcpy(note_map->note, "D#6");
                    else if (freq >= 1318.51f) strcpy(note_map->note, "E6");
                    else if (freq >= 130.81f && freq < 146.83f) strcpy(note_map->note, "C3");
                    else if (freq >= 146.83f && freq < 164.81f) strcpy(note_map->note, "D3");
                    else if (freq >= 164.81f && freq < 174.61f) strcpy(note_map->note, "E3");
                    else if (freq >= 174.61f && freq < 196.00f) strcpy(note_map->note, "F3");
                    else if (freq >= 196.00f && freq < 220.00f) strcpy(note_map->note, "G3");
                    else if (freq >= 220.00f && freq < 246.94f) strcpy(note_map->note, "A3");
                    else if (freq >= 246.94f && freq < 261.63f) strcpy(note_map->note, "B3");
                    else strcpy(note_map->note, "Unknown");
                }
            }
        }
    }

    ini_file_destroy(ini);
    return CONFIG_SUCCESS;
}
