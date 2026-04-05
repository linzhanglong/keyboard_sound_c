/**
 * @file error.h
 * @brief 错误处理宏和工具函数
 */

#ifndef COMMON_ERROR_H
#define COMMON_ERROR_H

#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 错误处理宏
 *============================================================================*/

/* 打印错误信息并返回 */
#define ERROR_RETURN(msg, ret) do { \
    fprintf(stderr, "Error: %s\n", msg); \
    return ret; \
} while(0)

/* 检查指针是否为 NULL */
#define CHECK_NULL(ptr, ret) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "Error: NULL pointer at %s:%d\n", __FILE__, __LINE__); \
        return ret; \
    } \
} while(0)

/* 检查条件是否为真 */
#define CHECK_TRUE(cond, msg, ret) do { \
    if (!(cond)) { \
        fprintf(stderr, "Error: %s at %s:%d\n", msg, __FILE__, __LINE__); \
        return ret; \
    } \
} while(0)

/* 内存分配失败处理 */
#define CHECK_ALLOC(ptr) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "Error: Memory allocation failed at %s:%d\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

/*==============================================================================
 * 工具函数
 *============================================================================*/

/**
 * @brief 将 ConfigResult 转换为字符串
 */
const char* config_result_to_string(ConfigResult result);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_ERROR_H */
