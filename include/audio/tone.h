/**
 * @file tone.h
 * @brief 音调生成器 - 泛音叠加和颤音
 *
 * 真实乐器发声原理:
 * - 物体振动产生基频 + 多次谐波
 * - 各次谐波有不同的衰减特性
 * - 弦乐等乐器有 vibrato (音高颤动)
 *
 * 钢琴特性:
 * - 琴弦被锤击后产生多级谐波
 * - 高频谐波比低频谐波衰减更快
 * - 声音从强到弱自然衰减
 */

#ifndef TONE_H
#define TONE_H

#include <stdint.h>
#include <stddef.h>
#include "audio/audio_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 音调生成器
 *============================================================================*/

/** 音调生成器配置 */
typedef struct {
    float frequency;      /**< 基频 (Hz) */
    float duration;       /**< 时长 (秒) */
    float volume;         /**< 音量 (0.0 ~ 1.0) */
    const TimbreConfig* timbre;  /**< 音色配置 */
} ToneConfig;

/** 音调生成器上下文 */
typedef struct {
    float frequency;      /**< 当前频率 */
    float sample_rate;    /**< 采样率 */
    float volume;         /**< 音量 */

    float phase;          /**< 当前相位 */
    float vibrato_phase;  /**< 颤音相位 */

    uint32_t sample_index;/**< 采样索引 */
    uint32_t num_samples;  /**< 总采样数 */

    const TimbreConfig* timbre;  /**< 音色配置 */
} ToneGenerator;

/*==============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化音调生成器
 * @param gen 生成器指针
 * @param freq 基频 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 ~ 1.0)
 * @param timbre 音色配置
 */
void tone_init(ToneGenerator* gen, float freq, float duration,
               float volume, const TimbreConfig* timbre);

/**
 * @brief 生成下一个采样值
 * @param gen 生成器指针
 * @return 采样值 (-1.0 ~ 1.0)
 */
float tone_process(ToneGenerator* gen);

/**
 * @brief 生成一组采样值
 * @param gen 生成器指针
 * @param output 输出缓冲区
 * @param num_samples 采样数
 * @return 实际生成的采样数
 */
uint32_t tone_process_buffer(ToneGenerator* gen, float* output, uint32_t num_samples);

/**
 * @brief 检查生成器是否完成
 * @param gen 生成器指针
 * @return 1 if 完成, 0 if 未完成
 */
int tone_is_finished(const ToneGenerator* gen);

/**
 * @brief 重置生成器
 * @param gen 生成器指针
 */
void tone_reset(ToneGenerator* gen);

/**
 * @brief 生成单音并输出到缓冲区
 * @param freq 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量
 * @param timbre 音色配置
 * @param output 输出缓冲区
 * @param num_samples 缓冲区大小 (采样数)
 * @return 实际写入的采样数
 */
uint32_t tone_generate(float freq, float duration, float volume,
                       const TimbreConfig* timbre,
                       float* output, uint32_t max_samples);

#ifdef __cplusplus
}
#endif

#endif /* TONE_H */