/**
 * @file keymapping.c
 * @brief 键盘按键映射管理实现
 */

#define _GNU_SOURCE

#include "input/keymapping.h"
#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建按键映射对象
 * @return 成功返回映射对象指针，失败返回NULL
 * @note 创建后需要调用key_mapping_destroy释放资源
 */
KeyMapping* key_mapping_create(void) {
    KeyMapping* mapping = (KeyMapping*)calloc(1, sizeof(KeyMapping));
    if (!mapping) {
        return NULL;
    }

    mapping->mappings = NULL;
    mapping->count = 0;
    mapping->capacity = 0;
    mapping->default_timbre = TIMBRE_PIANO;

    return mapping;
}

/**
 * @brief 销毁按键映射对象
 * @param mapping 按键映射对象指针
 * @note 释放映射对象占用的所有内存资源
 */
void key_mapping_destroy(KeyMapping* mapping) {
    if (!mapping) {
        return;
    }

    if (mapping->mappings) {
        free(mapping->mappings);
    }

    free(mapping);
}

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 创建默认按键映射（钢琴键盘布局）
 * @param default_timbre 默认音色类型
 * @return 成功返回映射对象指针，失败返回NULL
 * @note 默认映射包含40个按键，覆盖C3到E6音域
 */
KeyMapping* key_mapping_create_default(TimbreType default_timbre) {
    KeyMapping* mapping = key_mapping_create();
    if (!mapping) {
        return NULL;
    }

    mapping->default_timbre = default_timbre;

    /* 默认按键映射 - 钢琴键盘布局 */
    KeyNoteMap default_mappings[40];
    int idx = 0;

    default_mappings[idx] = (KeyNoteMap){KEY_A, 261.63f, "C4", "中央C", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_S, 293.66f, "D4", "D4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_D, 329.63f, "E4", "E4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_F, 349.23f, "F4", "F4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_G, 392.00f, "G4", "G4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_H, 440.00f, "A4", "A4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_J, 493.88f, "B4", "B4", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_K, 523.25f, "C5", "C5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_L, 587.33f, "D5", "D5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_SEMICOLON, 659.25f, "E5", "E5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_APOSTROPHE, 698.46f, "F5", "F5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_Z, 130.81f, "C3", "C3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_X, 146.83f, "D3", "D3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_C, 164.81f, "E3", "E3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_V, 174.61f, "F3", "F3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_B, 196.00f, "G3", "G3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_N, 220.00f, "A3", "A3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_M, 246.94f, "B3", "B3", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_Q, 523.25f, "C5", "C5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_W, 587.33f, "D5", "D5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_E, 659.25f, "E5", "E5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_R, 698.46f, "F5", "F5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_T, 783.99f, "G5", "G5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_Y, 880.00f, "A5", "A5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_U, 987.77f, "B5", "B5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_I, 1046.50f, "C6", "C6", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_O, 1174.66f, "D6", "D6", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_P, 1318.51f, "E6", "E6", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_1, 523.25f, "C5", "C5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_2, 587.33f, "D5", "D5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_3, 659.25f, "E5", "E5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_4, 698.46f, "F5", "F5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_5, 783.99f, "G5", "G5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_6, 880.00f, "A5", "A5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_7, 987.77f, "B5", "B5", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_8, 1046.50f, "C6", "C6", TIMBRE_PIANO};
    idx++;

    default_mappings[idx] = (KeyNoteMap){KEY_9, 1174.66f, "D6", "D6", TIMBRE_PIANO};
    idx++;

    default_mappings[idx++] = (KeyNoteMap){KEY_0, 1318.51f, "E6", "E6", TIMBRE_PIANO};

    size_t count = sizeof(default_mappings) / sizeof(default_mappings[0]);
    mapping->mappings = (KeyNoteMap*)malloc(count * sizeof(KeyNoteMap));
    if (!mapping->mappings) {
        key_mapping_destroy(mapping);
        return NULL;
    }

    memcpy(mapping->mappings, default_mappings, count * sizeof(KeyNoteMap));
    mapping->count = count;
    mapping->capacity = count;

    return mapping;
}

/**
 * @brief 从文件加载按键映射配置
 * @param path 配置文件路径
 * @param default_timbre 默认音色类型
 * @return 成功返回映射对象指针，失败返回NULL
 * @note 当前实现返回默认映射，实际应解析JSON配置文件
 */
KeyMapping* key_mapping_load_from_file(const char* path, TimbreType default_timbre) {
    /* 简化实现：目前返回默认映射 */
    /* 实际实现应解析 JSON 配置文件 */
    return key_mapping_create_default(default_timbre);
}

/*==============================================================================
 * 查找和访问
 *============================================================================*/

/**
 * @brief 查找按键对应的音符映射
 * @param code 按键码
 * @param mapping 按键映射对象指针
 * @return 找到返回音符映射指针，未找到返回NULL
 */
const KeyNoteMap* key_mapping_find(KeyCode code, const KeyMapping* mapping) {
    if (!mapping || !mapping->mappings) {
        return NULL;
    }

    for (size_t i = 0; i < mapping->count; i++) {
        if (mapping->mappings[i].code == code) {
            return &mapping->mappings[i];
        }
    }

    return NULL;
}

/**
 * @brief 获取按键对应的音色类型
 * @param code 按键码
 * @param mapping 按键映射对象指针
 * @return 按键对应的音色类型，未找到返回默认音色
 */
TimbreType key_mapping_get_timbre_for_key(KeyCode code, const KeyMapping* mapping) {
    if (!mapping) {
        return TIMBRE_PIANO;
    }

    const KeyNoteMap* note_map = key_mapping_find(code, mapping);
    if (note_map) {
        return note_map->timbre;
    }

    return mapping->default_timbre;
}
