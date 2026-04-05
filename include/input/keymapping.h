/**
 * @file keymapping.h
 * @brief 键盘按键映射配置
 */

#ifndef KEY_MAPPING_H
#define KEY_MAPPING_H

#include "keyboard.h"
#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 按键音符映射
 *============================================================================*/

typedef struct {
    KeyCode code;           /**< 按键码 */
    float frequency;        /**< 频率 (Hz) */
    char note[16];          /**< 音符名称 (如 "C4") */
    char desc[32];          /**< 描述 (如 "中央C") */
    TimbreType timbre;      /**< 音色类型 */
} KeyNoteMap;

/*==============================================================================
 * 按键映射管理器
 *============================================================================*/

typedef struct {
    KeyNoteMap* mappings;   /**< 映射数组 */
    size_t count;           /**< 映射数量 */
    size_t capacity;        /**< 数组容量 */
    TimbreType default_timbre; /**< 默认音色 */
} KeyMapping;

/*==============================================================================
 * 创建和销毁
 *============================================================================*/

/**
 * @brief 创建按键映射管理器
 * @return 按键映射管理器指针，失败返回 NULL
 */
KeyMapping* key_mapping_create(void);

/**
 * @brief 销毁按键映射管理器
 * @param mapping 按键映射管理器指针
 */
void key_mapping_destroy(KeyMapping* mapping);

/*==============================================================================
 * 配置加载
 *============================================================================*/

/**
 * @brief 从文件加载按键映射
 * @param path 配置文件路径
 * @param default_timbre 默认音色类型
 * @return 按键映射管理器指针，失败返回 NULL
 */
KeyMapping* key_mapping_load_from_file(const char* path, TimbreType default_timbre);

/**
 * @brief 创建默认按键映射
 * @param default_timbre 默认音色类型
 * @return 按键映射管理器指针，失败返回 NULL
 */
KeyMapping* key_mapping_create_default(TimbreType default_timbre);

/*==============================================================================
 * 查找和访问
 *============================================================================*/

/**
 * @brief 根据按键码查找映射
 * @param code 按键码
 * @param mapping 按键映射管理器指针
 * @return 映射指针，未找到返回 NULL
 */
const KeyNoteMap* key_mapping_find(KeyCode code, const KeyMapping* mapping);

/**
 * @brief 获取按键对应的音色类型
 * @param code 按键码
 * @param mapping 按键映射管理器指针
 * @return 音色类型
 */
TimbreType key_mapping_get_timbre_for_key(KeyCode code, const KeyMapping* mapping);

#ifdef __cplusplus
}
#endif

#endif /* KEY_MAPPING_H */
