/**
 * @file tone.c
 * @brief 音调生成器实现
 */

#include "audio/tone.h"
#include "audio/adsr.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0 * M_PI)

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 初始化音调生成器
 * @param gen 音调生成器结构体指针
 * @param freq 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 - 1.0)
 * @param timbre 音色配置指针，为NULL时使用默认音色
 */
void tone_init(ToneGenerator* gen, float freq, float duration,
               float volume, const TimbreConfig* timbre)
{
    memset(gen, 0, sizeof(ToneGenerator));

    gen->frequency = freq;
    gen->sample_rate = (float)SAMPLE_RATE;
    gen->volume = volume;
    gen->timbre = timbre;
    gen->num_samples = (uint32_t)(duration * gen->sample_rate);
    gen->phase = 0.0f;
    gen->vibrato_phase = 0.0f;
    gen->sample_index = 0;
}

/**
 * @brief 处理音调生成，计算下一个采样点
 * @param gen 音调生成器结构体指针
 * @return 当前采样点的振幅值
 * @note 使用谐波叠加和颤音效果生成波形
 */
float tone_process(ToneGenerator* gen)
{
    if (gen->sample_index >= gen->num_samples) {
        return 0.0f;
    }

    const TimbreConfig* timbre = gen->timbre;
    float t = (float)gen->sample_index / gen->sample_rate;

    /* 计算音高颤动 (Vibrato) */
    float freq = gen->frequency;
    if (timbre && timbre->vibrato_depth > 0.0f && timbre->vibrato_rate > 0.0f) {
        float vibrato = timbre->vibrato_depth *
                        sinf(TWO_PI * timbre->vibrato_rate * t);
        freq += vibrato;
    }

    /* 叠加各次谐波 */
    float wave = 0.0f;

    if (timbre) {
        for (int i = 0; i < timbre->harmonic_count; i++) {
            float harmonic_freq = freq * timbre->harmonics[i].multiplier;
            float strength = timbre->harmonics[i].strength;

            /* 谐波衰减: 高频谐波衰减更快 */
            float decay_rate = timbre->harmonic_decay_base +
                              timbre->harmonics[i].multiplier * 2.0f;
            float decay = expf(-t * decay_rate);

            /* 累加谐波 */
            float harmonic_wave = sinf(TWO_PI * harmonic_freq * t) * decay * strength;
            wave += harmonic_wave;
        }

        /* 归一化: 防止多谐波叠加过载 */
        float max_harmonic = 0.0f;
        for (int i = 0; i < timbre->harmonic_count; i++) {
            max_harmonic += timbre->harmonics[i].strength;
        }
        if (max_harmonic > 0.0f) {
            wave /= max_harmonic;
        }
    } else {
        /* 无音色配置，使用纯正弦波 */
        wave = sinf(TWO_PI * freq * t);
    }

    /* 更新相位和索引 */
    float phase_increment = gen->frequency * TWO_PI / gen->sample_rate;
    gen->phase += phase_increment;
    if (gen->phase >= TWO_PI) {
        gen->phase -= TWO_PI;
    }

    gen->sample_index++;

    return wave;
}

/**
 * @brief 处理音调生成并输出到缓冲区
 * @param gen 音调生成器结构体指针
 * @param output 输出缓冲区
 * @param num_samples 要生成的采样数
 * @return 实际生成的采样数
 */
uint32_t tone_process_buffer(ToneGenerator* gen, float* output, uint32_t num_samples)
{
    uint32_t generated = 0;

    for (uint32_t i = 0; i < num_samples && !tone_is_finished(gen); i++) {
        output[i] = tone_process(gen);
        generated++;
    }

    return generated;
}

/**
 * @brief 检查音调生成是否已完成
 * @param gen 音调生成器结构体指针
 * @return 生成已完成返回1，否则返回0
 */
int tone_is_finished(const ToneGenerator* gen)
{
    return (gen->sample_index >= gen->num_samples) ? 1 : 0;
}

/**
 * @brief 重置音调生成器状态
 * @param gen 音调生成器结构体指针
 * @note 将采样索引和相位重置为初始状态
 */
void tone_reset(ToneGenerator* gen)
{
    gen->sample_index = 0;
    gen->phase = 0.0f;
    gen->vibrato_phase = 0.0f;
}

/**
 * @brief 生成单音并输出到缓冲区 (简化接口)
 * @param freq 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 - 1.0)
 * @param timbre 音色配置指针，为NULL时使用默认音色
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 * @note 包含ADSR包络处理，生成完整的音符波形
 */
uint32_t tone_generate(float freq, float duration, float volume,
                      const TimbreConfig* timbre,
                      float* output, uint32_t max_samples)
{
    if (!output || max_samples == 0) {
        return 0;
    }

    ToneGenerator gen;
    tone_init(&gen, freq, duration, volume, timbre);

    uint32_t num_samples = (uint32_t)(duration * SAMPLE_RATE);
    if (num_samples > max_samples) {
        num_samples = max_samples;
    }

    /* 生成原始波形 */
    float* raw_wave = (float*)malloc(num_samples * sizeof(float));
    if (!raw_wave) {
        return 0;
    }

    uint32_t generated = tone_process_buffer(&gen, raw_wave, num_samples);

    /* 应用 ADSR 包络 */
    ADSR_Envelope adsr;
    ADSR_Config adsr_cfg = {
        timbre ? timbre->attack : ADSR_ATTACK,
        timbre ? timbre->decay : ADSR_DECAY,
        timbre ? timbre->sustain : ADSR_SUSTAIN,
        timbre ? timbre->release : ADSR_RELEASE
    };
    adsr_init(&adsr, &adsr_cfg, generated);

    for (uint32_t i = 0; i < generated; i++) {
        float envelope = adsr_process(&adsr);
        output[i] = raw_wave[i] * envelope * volume;
    }

    free(raw_wave);
    return generated;
}