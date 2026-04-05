/**
 * @file types.h
 * @brief 通用类型定义
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 配置加载结果
 *============================================================================*/

typedef enum {
    CONFIG_SUCCESS = 0,
    CONFIG_ERROR_FILE_NOT_FOUND,
    CONFIG_ERROR_PARSE,
    CONFIG_ERROR_VALIDATION,
    CONFIG_ERROR_OUT_OF_MEMORY,
    CONFIG_ERROR_INVALID_PATH,
    CONFIG_ERROR_ALLOC_FAILED
} ConfigResult;

/*==============================================================================
 * 音频后端类型
 *============================================================================*/

typedef enum {
    AUDIO_BACKEND_PULSEAUDIO = 0,
    AUDIO_BACKEND_ALSA,
    AUDIO_BACKEND_DEFAULT
} AudioBackend;

/*==============================================================================
 * 音色类型
 *============================================================================*/

typedef enum {
    TIMBRE_PIANO = 0,
    TIMBRE_ELECTRIC,
    TIMBRE_BELL,
    TIMBRE_STRING,
    TIMBRE_COUNT
} TimbreType;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TYPES_H */
