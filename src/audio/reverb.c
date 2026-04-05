/**
 * @file reverb.c
 * @brief 混响效果器实现
 */

#include "audio/reverb.h"
#include "audio/audio_config.h"
#include <stdlib.h>
#include <string.h>

/*==============================================================================
 * 辅助函数
 *============================================================================*/

/**
 * @brief 计算基于房间大小的延迟时间
 */
static inline uint32_t calc_delay_samples(float room_size, float multiplier)
{
    /* 基础延迟 0.5ms，最大 50ms */
    float base_delay_ms = 0.5f + room_size * 20.0f;
    return (uint32_t)(base_delay_ms * multiplier * SAMPLE_RATE / 1000.0f);
}

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 获取默认混响配置
 * @param config 混响配置结构体指针
 * @note 设置中等房间大小的默认混响参数
 */
void reverb_get_default_config(ReverbConfig* config)
{
    if (!config) return;

    config->room_size = 0.25f;
    config->damp = 0.5f;
    config->wet = 0.25f;
    config->num_delays = 4;

    /* 默认延迟线: 基础延迟的 1x, 2x, 3x, 4x */
    config->delays[0].delay_samples = calc_delay_samples(config->room_size, 1.0f);
    config->delays[0].gain = 1.0f;

    config->delays[1].delay_samples = calc_delay_samples(config->room_size, 2.0f);
    config->delays[1].gain = 0.6f;

    config->delays[2].delay_samples = calc_delay_samples(config->room_size, 3.0f);
    config->delays[2].gain = 0.4f;

    config->delays[3].delay_samples = calc_delay_samples(config->room_size, 4.0f);
    config->delays[3].gain = 0.3f;
}

/**
 * @brief 设置小房间混响配置
 * @param config 混响配置结构体指针
 * @note 适合小型房间或近距离音效
 */
void reverb_small_room(ReverbConfig* config)
{
    if (!config) return;

    config->room_size = 0.1f;
    config->damp = 0.7f;
    config->wet = 0.15f;
    config->num_delays = 2;

    config->delays[0].delay_samples = calc_delay_samples(config->room_size, 1.0f);
    config->delays[0].gain = 1.0f;

    config->delays[1].delay_samples = calc_delay_samples(config->room_size, 2.0f);
    config->delays[1].gain = 0.5f;
}

/**
 * @brief 设置中等房间混响配置
 * @param config 混响配置结构体指针
 * @note 使用默认配置，适合中等大小房间
 */
void reverb_medium_room(ReverbConfig* config)
{
    reverb_get_default_config(config);
}

/**
 * @brief 设置大厅混响配置
 * @param config 混响配置结构体指针
 * @note 适合大型空间或音乐厅效果
 */
void reverb_large_hall(ReverbConfig* config)
{
    if (!config) return;

    config->room_size = 0.5f;
    config->damp = 0.3f;
    config->wet = 0.35f;
    config->num_delays = 6;

    config->delays[0].delay_samples = calc_delay_samples(config->room_size, 1.0f);
    config->delays[0].gain = 1.0f;

    config->delays[1].delay_samples = calc_delay_samples(config->room_size, 1.5f);
    config->delays[1].gain = 0.7f;

    config->delays[2].delay_samples = calc_delay_samples(config->room_size, 2.0f);
    config->delays[2].gain = 0.5f;

    config->delays[3].delay_samples = calc_delay_samples(config->room_size, 3.0f);
    config->delays[3].gain = 0.35f;

    config->delays[4].delay_samples = calc_delay_samples(config->room_size, 4.5f);
    config->delays[4].gain = 0.25f;

    config->delays[5].delay_samples = calc_delay_samples(config->room_size, 6.0f);
    config->delays[5].gain = 0.15f;
}

/**
 * @brief 初始化混响处理器
 * @param rev 混响处理器结构体指针
 * @param config 混响配置参数
 * @param max_delay_ms 最大延迟时间(毫秒)
 * @return 成功返回0，失败返回-1
 */
int reverb_init(ReverbProcessor* rev, const ReverbConfig* config, uint32_t max_delay_ms)
{
    if (!rev || !config) {
        return -1;
    }

    memset(rev, 0, sizeof(ReverbProcessor));

    /* 保存配置 */
    rev->config = *config;

    /* 计算最大延迟采样数 */
    rev->max_delay_samples = (uint32_t)((float)max_delay_ms * SAMPLE_RATE / 1000.0f);
    if (rev->max_delay_samples < 1) {
        rev->max_delay_samples = (uint32_t)(50 * SAMPLE_RATE / 1000); /* 最小 50ms */
    }

    /* 分配延迟缓冲区 */
    rev->buffer_size = rev->max_delay_samples + 1;
    rev->buffer = (float*)calloc(rev->buffer_size, sizeof(float));
    if (!rev->buffer) {
        return -1;
    }

    rev->write_pos = 0;
    return 0;
}

/**
 * @brief 销毁混响处理器
 * @param rev 混响处理器结构体指针
 * @note 释放延迟缓冲区内存
 */
void reverb_destroy(ReverbProcessor* rev)
{
    if (rev) {
        if (rev->buffer) {
            free(rev->buffer);
            rev->buffer = NULL;
        }
    }
}

/**
 * @brief 重置混响处理器状态
 * @param rev 混响处理器结构体指针
 * @note 清空延迟缓冲区，重置写入位置
 */
void reverb_reset(ReverbProcessor* rev)
{
    if (rev && rev->buffer) {
        memset(rev->buffer, 0, rev->buffer_size * sizeof(float));
    }
    rev->write_pos = 0;
}

/**
 * @brief 处理单个采样点的混响效果
 * @param rev 混响处理器结构体指针
 * @param input 输入采样值
 * @return 应用混响后的输出采样值
 */
float reverb_process(ReverbProcessor* rev, float input)
{
    if (!rev || !rev->buffer) {
        return input;
    }

    /* 保存当前输入到延迟线 */
    rev->buffer[rev->write_pos] = input;

    /* 计算混响输出 */
    float wet = 0.0f;
    float damp_factor = 1.0f - rev->config.damp * 0.5f;

    for (int i = 0; i < rev->config.num_delays; i++) {
        uint32_t delay_samples = rev->config.delays[i].delay_samples;

        /* 确保延迟不超过缓冲区大小 */
        if (delay_samples >= rev->buffer_size) {
            delay_samples = rev->buffer_size - 1;
        }

        /* 计算读取位置 */
        uint32_t read_pos = rev->write_pos + rev->buffer_size - delay_samples;
        read_pos %= rev->buffer_size;

        /* 累加延迟信号 */
        wet += rev->buffer[read_pos] * rev->config.delays[i].gain * damp_factor;

        /* 高频衰减 */
        damp_factor *= (1.0f - rev->config.damp * 0.3f);
    }

    /* 移动写入位置 */
    rev->write_pos++;
    if (rev->write_pos >= rev->buffer_size) {
        rev->write_pos = 0;
    }

    /* 混合干湿信号 */
    float output = input * (1.0f - rev->config.wet) + wet * rev->config.wet;

    return output;
}

/**
 * @brief 处理音频缓冲区的混响效果
 * @param rev 混响处理器结构体指针
 * @param input 输入音频缓冲区
 * @param output 输出音频缓冲区
 * @param num_samples 采样点数
 */
void reverb_process_buffer(ReverbProcessor* rev,
                          const float* input, float* output,
                          uint32_t num_samples)
{
    if (!rev || !input || !output) {
        return;
    }

    for (uint32_t i = 0; i < num_samples; i++) {
        output[i] = reverb_process(rev, input[i]);
    }
}