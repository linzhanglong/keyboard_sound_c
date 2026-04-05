/**
 * @file adsr.c
 * @brief ADSR 包络生成器实现
 */

#include "audio/adsr.h"
#include "audio/audio_config.h"
#include <string.h>
#include <math.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/

#define ADSR_MIN_SAMPLES  1   /**< 最小采样数 */

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 初始化ADSR包络
 * @param adsr ADSR包络结构体指针
 * @param config ADSR配置参数，为NULL时使用默认值
 * @param total_samples 总采样数，用于计算持续阶段时长
 */
void adsr_init(ADSR_Envelope* adsr, const ADSR_Config* config, uint32_t total_samples)
{
    memset(adsr, 0, sizeof(ADSR_Envelope));

    if (config) {
        adsr->config = *config;
    }

    adsr->state = ADSR_IDLE;

    /* 计算各阶段采样数 */
    adsr->attack_samples = (uint32_t)(adsr->config.attack * SAMPLE_RATE);
    adsr->decay_samples = (uint32_t)(adsr->config.decay * SAMPLE_RATE);
    adsr->release_samples = (uint32_t)(adsr->config.release * SAMPLE_RATE);

    /* 确保最小采样数 */
    if (adsr->attack_samples < ADSR_MIN_SAMPLES) adsr->attack_samples = ADSR_MIN_SAMPLES;
    if (adsr->decay_samples < ADSR_MIN_SAMPLES) adsr->decay_samples = ADSR_MIN_SAMPLES;
    if (adsr->release_samples < ADSR_MIN_SAMPLES) adsr->release_samples = ADSR_MIN_SAMPLES;

    /* 持续阶段采样数 = 总采样数 - 其他阶段 */
    uint32_t other_samples = adsr->attack_samples + adsr->decay_samples + adsr->release_samples;
    if (total_samples > other_samples) {
        adsr->sustain_samples = total_samples - other_samples;
    } else {
        adsr->sustain_samples = 0;
    }
}

/**
 * @brief 重置ADSR包络状态
 * @param adsr ADSR包络结构体指针
 * @note 将包络状态重置为IDLE，位置归零
 */
void adsr_reset(ADSR_Envelope* adsr)
{
    adsr->state = ADSR_IDLE;
    adsr->sample_pos = 0;
    adsr->prev_level = 0.0f;
}

/**
 * @brief 触发ADSR包络开始
 * @param adsr ADSR包络结构体指针
 * @param total_samples 总采样数，用于重新计算持续阶段
 * @note 将包络状态设置为ATTACK阶段，开始音符起音
 */
void adsr_trigger(ADSR_Envelope* adsr, uint32_t total_samples)
{
    adsr->state = ADSR_ATTACK;
    adsr->sample_pos = 0;
    adsr->prev_level = 0.0f;

    /* 重新计算持续阶段采样数 */
    uint32_t other_samples = adsr->attack_samples + adsr->decay_samples + adsr->release_samples;
    if (total_samples > other_samples) {
        adsr->sustain_samples = total_samples - other_samples;
    } else {
        adsr->sustain_samples = 0;
    }
}

/**
 * @brief 释放ADSR包络
 * @param adsr ADSR包络结构体指针
 * @note 将包络状态切换到RELEASE阶段，开始音符释音
 */
void adsr_release(ADSR_Envelope* adsr)
{
    if (adsr->state != ADSR_IDLE) {
        adsr->release_start = adsr->sample_pos;
        adsr->state = ADSR_RELEASE;
    }
}

/**
 * @brief 处理ADSR包络，计算当前采样点的音量级别
 * @param adsr ADSR包络结构体指针
 * @return 当前采样点的音量级别 (0.0 - 1.0)
 * @note 根据当前状态（ATTACK/DECAY/SUSTAIN/RELEASE/IDLE）计算音量
 */
float adsr_process(ADSR_Envelope* adsr)
{
    float level = 0.0f;

    switch (adsr->state) {
        case ADSR_ATTACK: {
            /* 起音阶段: 0 -> 1 */
            if (adsr->attack_samples > 0) {
                float progress = (float)adsr->sample_pos / adsr->attack_samples;
                level = progress;
            } else {
                level = 1.0f;
            }
            adsr->prev_level = level;

            adsr->sample_pos++;
            if (adsr->sample_pos >= adsr->attack_samples) {
                adsr->state = ADSR_DECAY;
                adsr->sample_pos = 0;
            }
            break;
        }

        case ADSR_DECAY: {
            /* 衰减阶段: 1 -> sustain */
            if (adsr->decay_samples > 0) {
                float progress = (float)adsr->sample_pos / adsr->decay_samples;
                level = 1.0f - (1.0f - adsr->config.sustain) * progress;
            } else {
                level = adsr->config.sustain;
            }
            adsr->prev_level = level;

            adsr->sample_pos++;
            if (adsr->sample_pos >= adsr->decay_samples) {
                adsr->state = ADSR_SUSTAIN;
                adsr->sample_pos = 0;
            }
            break;
        }

        case ADSR_SUSTAIN: {
            /* 持续阶段: 保持 sustain 音量 */
            level = adsr->config.sustain;
            adsr->prev_level = level;

            adsr->sample_pos++;
            if (adsr->sample_pos >= adsr->sustain_samples) {
                adsr->state = ADSR_IDLE;
            }
            break;
        }

        case ADSR_RELEASE: {
            /* 释音阶段: prev_level -> 0 */
            if (adsr->release_samples > 0) {
                uint32_t release_pos = adsr->sample_pos - adsr->release_start;
                if (release_pos < adsr->release_samples) {
                    float progress = (float)release_pos / adsr->release_samples;
                    level = adsr->prev_level * (1.0f - progress);
                } else {
                    level = 0.0f;
                }
            }

            adsr->sample_pos++;
            if (adsr->sample_pos >= adsr->release_start + adsr->release_samples) {
                adsr->state = ADSR_IDLE;
                level = 0.0f;
            }
            break;
        }

        case ADSR_IDLE:
        default:
            level = 0.0f;
            break;
    }

    return level;
}

/**
 * @brief 检查ADSR包络是否已完成
 * @param adsr ADSR包络结构体指针
 * @return 包络已完成返回1，否则返回0
 */
int adsr_is_finished(const ADSR_Envelope* adsr)
{
    return (adsr->state == ADSR_IDLE) ? 1 : 0;
}

/**
 * @brief 获取ADSR包络状态名称
 * @param adsr ADSR包络结构体指针
 * @return 状态名称字符串
 */
const char* adsr_state_name(const ADSR_Envelope* adsr)
{
    switch (adsr->state) {
        case ADSR_IDLE:    return "IDLE";
        case ADSR_ATTACK:  return "ATTACK";
        case ADSR_DECAY:   return "DECAY";
        case ADSR_SUSTAIN: return "SUSTAIN";
        case ADSR_RELEASE: return "RELEASE";
        default:           return "UNKNOWN";
    }
}