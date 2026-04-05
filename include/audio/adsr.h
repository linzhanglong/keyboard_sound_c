/**
 * @file adsr.h
 * @brief ADSR 包络生成器
 *
 * ADSR (Attack-Decay-Sustain-Release) 是合成器中模拟声音起伏的核心算法
 *
 * 波形示意:
 *         /\
 *        /  \________
 *       /            \
 *      /              \________
 *     A D    S         R
 *
 * A - Attack:  起音阶段，从无声升到峰值
 * D - Decay:  衰减阶段，从峰值跌到持续音量
 * S - Sustain:持续阶段，保持的音量水平
 * R - Release: 释音阶段，松键后衰减到无声
 */

#ifndef ADSR_H
#define ADSR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * ADSR 状态机
 *============================================================================*/

/** ADSR 状态枚举 */
typedef enum {
    ADSR_IDLE = 0,    /**< 空闲状态 */
    ADSR_ATTACK,      /**< 起音阶段 */
    ADSR_DECAY,       /**< 衰减阶段 */
    ADSR_SUSTAIN,     /**< 持续阶段 */
    ADSR_RELEASE,     /**< 释音阶段 */
} ADSR_State;

/** ADSR 包络配置 */
typedef struct {
    float attack;     /**< 起音时间 (秒) */
    float decay;      /**< 衰减时间 (秒) */
    float sustain;    /**< 持续音量 (0.0 ~ 1.0) */
    float release;    /**< 释音时间 (秒) */
} ADSR_Config;

/** ADSR 包络发生器 */
typedef struct {
    ADSR_Config config;       /**< 包络配置 */
    ADSR_State state;         /**< 当前状态 */

    /** 阶段计数器和限制 */
    uint32_t attack_samples;  /**< 起音采样数 */
    uint32_t decay_samples;   /**< 衰减采样数 */
    uint32_t sustain_samples; /**< 持续采样数 */
    uint32_t release_samples; /**< 释音采样数 */

    uint32_t sample_pos;      /**< 当前采样位置 */
    uint32_t release_start;   /**< 释音开始位置 */
    float prev_level;         /**< 上一级音量 (用于释音平滑) */
} ADSR_Envelope;

/*==============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化 ADSR 包络发生器
 * @param adsr ADSR 指针
 * @param config 包络配置
 * @param total_samples 总采样数 (duration * sample_rate)
 */
void adsr_init(ADSR_Envelope* adsr, const ADSR_Config* config, uint32_t total_samples);

/**
 * @brief 重置 ADSR 状态
 * @param adsr ADSR 指针
 */
void adsr_reset(ADSR_Envelope* adsr);

/**
 * @brief 触发 ADSR (开始新音符)
 * @param adsr ADSR 指针
 * @param total_samples 总采样数
 */
void adsr_trigger(ADSR_Envelope* adsr, uint32_t total_samples);

/**
 * @brief 释放 ADSR (开始释音)
 * @param adsr ADSR 指针
 */
void adsr_release(ADSR_Envelope* adsr);

/**
 * @brief 获取当前采样点的包络值
 * @param adsr ADSR 指针
 * @return 包络值 (0.0 ~ 1.0)
 */
float adsr_process(ADSR_Envelope* adsr);

/**
 * @brief 检查 ADSR 是否完成
 * @param adsr ADSR 指针
 * @return 1 if 完成, 0 if 未完成
 */
int adsr_is_finished(const ADSR_Envelope* adsr);

/**
 * @brief 获取当前状态名称 (调试用)
 * @param adsr ADSR 指针
 * @return 状态名称字符串
 */
const char* adsr_state_name(const ADSR_Envelope* adsr);

#ifdef __cplusplus
}
#endif

#endif /* ADSR_H */