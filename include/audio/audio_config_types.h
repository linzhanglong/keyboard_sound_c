/**
 * @file audio_config_types.h
 * @brief 音频配置数据类型定义
 */

#ifndef AUDIO_CONFIG_TYPES_H
#define AUDIO_CONFIG_TYPES_H

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * ADSR 包络配置结构
 *============================================================================*/

typedef struct {
    float attack;               /**< 起音时间 (秒) */
    float decay;                /**< 衰减时间 (秒) */
    float sustain;              /**< 持续音量 (0.0 ~ 1.0) */
    float release;              /**< 释音时间 (秒) */
} ADSRConfig;

/*==============================================================================
 * 混响配置结构
 *============================================================================*/

/** 混响延迟线数量 */
#define REVERB_MAX_DELAYS 8

/** 单个延迟线 */
typedef struct {
    uint32_t delay_samples;  /**< 延迟采样数 */
    float gain;              /**< 该延迟线的增益 */
} ReverbDelay;

typedef struct {
    bool enabled;               /**< 是否启用 */
    float room_size;            /**< 房间大小 (0.0 ~ 1.0) */
    float damp;                 /**< 高频衰减 (0.0 ~ 1.0) */
    float wet;                  /**< 混响音量比例 */
    int num_delays;             /**< 延迟级数 */
    ReverbDelay delays[REVERB_MAX_DELAYS];  /**< 延迟线配置 */
} ReverbConfig;

/*==============================================================================
 * 音频配置结构
 *============================================================================*/

typedef struct {
    int sample_rate;            /**< 采样率 (Hz) */
    float default_volume;       /**< 默认音量 (0.0 ~ 1.0) */
    char audio_device[64];      /**< 音频设备名称 */
    int min_key_interval_ms;    /**< 最小按键间隔 (毫秒) */
    AudioBackend backend;       /**< 音频后端 */
    ADSRConfig adsr;            /**< ADSR 包络配置 */
    ReverbConfig reverb;        /**< 混响配置 */
} AudioConfig;

/*==============================================================================
 * 谐波定义
 *============================================================================*/

/** 最大谐波次数 */
#define MAX_HARMONICS 10

/** 单个谐波: (倍数, 强度) */
typedef struct {
    float multiplier;   /**< 相对基频的倍数 */
    float strength;     /**< 强度 (0.0 ~ 1.0) */
} Harmonic;

/*==============================================================================
 * 音色配置结构
 *============================================================================*/

typedef struct {
    TimbreType type;           /**< 音色类型 */
    char name[32];             /**< 音色名称 */

    /** 谐波列表 */
    Harmonic harmonics[MAX_HARMONICS];
    int harmonic_count;        /**< 谐波数量 */

    /** ADSR 包络参数 */
    float attack;              /**< 起音时间 (秒) */
    float decay;               /**< 衰减时间 (秒) */
    float sustain;             /**< 持续音量 (0.0 ~ 1.0) */
    float release;             /**< 释音时间 (秒) */

    /** 谐波衰减基数 (越高衰减越快) */
    float harmonic_decay_base;

    /** 颤音参数 */
    float vibrato_depth;       /**< 颤音深度 (Hz) */
    float vibrato_rate;        /**< 颤音速度 (Hz) */
} TimbreConfig;

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_CONFIG_TYPES_H */
