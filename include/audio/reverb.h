/**
 * @file reverb.h
 * @brief 混响效果器
 *
 * 混响 (Reverb) 模拟声音在房间中的反射和衰减
 *
 * 原理:
 * - 原始声音 (Dry) 直接输出
 * - 延迟后的声音 (Wet) 带有不同衰减
 * - 多级延迟叠加产生空间感
 *
 * 混响参数:
 * - Room Size: 房间大小，影响延迟时间
 * - Damping: 高频衰减，模拟空气吸收
 * - Wet Mix: 混响音量比例
 */

#ifndef REVERB_H
#define REVERB_H

#include <stdint.h>
#include <stddef.h>
#include "audio/audio_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 混响配置
 *============================================================================*/

/** 混响处理器 */
typedef struct {
    ReverbConfig config;         /**< 混响配置 */
    float* buffer;              /**< 延迟缓冲区 */
    uint32_t buffer_size;       /**< 缓冲区大小 */
    uint32_t write_pos;         /**< 写入位置 */
    uint32_t max_delay_samples; /**< 最大延迟采样数 */
} ReverbProcessor;

/*==============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化混响处理器
 * @param rev 处理器指针
 * @param config 混响配置
 * @param max_delay_ms 最大延迟时间 (毫秒)
 * @return 0 if 成功, -1 if 失败
 */
int reverb_init(ReverbProcessor* rev, const ReverbConfig* config, uint32_t max_delay_ms);

/**
 * @brief 销毁混响处理器，释放内存
 * @param rev 处理器指针
 */
void reverb_destroy(ReverbProcessor* rev);

/**
 * @brief 处理单个采样
 * @param rev 处理器指针
 * @param input 输入采样值
 * @return 处理后的采样值
 *
 * @note 这个函数会修改内部状态，不是纯函数
 */
float reverb_process(ReverbProcessor* rev, float input);

/**
 * @brief 处理一组采样
 * @param rev 处理器指针
 * @param input 输入缓冲区
 * @param output 输出缓冲区
 * @param num_samples 采样数
 */
void reverb_process_buffer(ReverbProcessor* rev,
                          const float* input, float* output,
                          uint32_t num_samples);

/**
 * @brief 重置混响状态
 * @param rev 处理器指针
 */
void reverb_reset(ReverbProcessor* rev);

/**
 * @brief 获取默认混响配置
 * @param config 配置指针
 */
void reverb_get_default_config(ReverbConfig* config);

/*==============================================================================
 * 预设配置
 *============================================================================*/

/** 小房间混响 */
void reverb_small_room(ReverbConfig* config);

/** 中房间混响 */
void reverb_medium_room(ReverbConfig* config);

/** 大厅混响 */
void reverb_large_hall(ReverbConfig* config);

#ifdef __cplusplus
}
#endif

#endif /* REVERB_H */