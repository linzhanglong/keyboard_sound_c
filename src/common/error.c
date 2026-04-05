/**
 * @file error.c
 * @brief 错误处理实现
 */

#define _GNU_SOURCE

#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==============================================================================
 * 错误消息映射
 *============================================================================*/

static const char* CONFIG_RESULT_STRINGS[] = {
    "Success",
    "File not found",
    "Parse error",
    "Validation error",
    "Out of memory",
    "Invalid path"
};

const char* config_result_to_string(ConfigResult result) {
    if (result >= 0 && result < sizeof(CONFIG_RESULT_STRINGS) / sizeof(CONFIG_RESULT_STRINGS[0])) {
        return CONFIG_RESULT_STRINGS[result];
    }
    return "Unknown error";
}
