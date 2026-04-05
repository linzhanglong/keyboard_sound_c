/**
 * @file player.h
 * @brief 音频播放器 - 直接使用 ALSA API 播放
 *
 * 播放流程:
 * 1. 生成 PCM 波形数据
 * 2. 直接通过 ALSA 播放
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <pthread.h>
#include "audio/audio_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 类型定义
 *============================================================================*/

/** 播放器状态 */
typedef enum {
    PLAYER_IDLE = 0,    /**< 空闲 */
    PLAYER_PLAYING,     /**< 正在播放 */
    PLAYER_ERROR        /**< 错误 */
} PlayerState;

/** 播放任务类型 */
typedef enum {
    TASK_NOTE = 0,     /**< 播放音符 */
    TASK_CHIME,        /**< 播放铃声 */
    TASK_CLICK,        /**< 播放点击音 */
    TASK_TICK,         /**< 播放 tick 音 */
    TASK_STOP          /**< 停止任务 */
} TaskType;

/** 单个播放任务 */
typedef struct PlayTask {
    TaskType type;              /**< 任务类型 */
    float frequency;            /**< 频率 (Hz) - 仅音符任务 */
    float duration;             /**< 时长 (秒) */
    TimbreType timbre;          /**< 音色类型 - 仅音符任务 */
    struct PlayTask* next;      /**< 队列中下一个任务 */
} PlayTask;

/** 音频播放器 */
typedef struct {
    char device[64];           /**< ALSA 设备名 */
    float volume;              /**< 主音量 */

    PlayerState state;         /**< 当前状态 */
    char error_msg[256];       /**< 错误消息 */

    pthread_mutex_t mutex;     /**< 线程互斥锁 */
    pthread_cond_t cond;       /**< 条件变量 */
    pthread_t play_thread;     /**< 播放线程 */
    int stop_flag;             /**< 停止标志 */

    PlayTask* task_queue;      /**< 任务队列头部 */
    PlayTask* task_queue_tail; /**< 任务队列尾部 */
} AudioPlayer;

/*==============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化音频播放器
 * @param player 播放器指针
 * @param device ALSA 设备名 (e.g., "plughw:0,1")
 * @param volume 音量 (0.0 ~ 1.0)
 * @return 0 if 成功, -1 if 失败
 */
int player_init(AudioPlayer* player, const char* device, float volume);

/**
 * @brief 销毁音频播放器
 * @param player 播放器指针
 */
void player_destroy(AudioPlayer* player);

/**
 * @brief 播放音符
 * @param player 播放器指针
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param timbre 音色类型
 * @return 0 if 成功, -1 if 失败
 */
int player_play_note(AudioPlayer* player, float frequency,
                     float duration, TimbreType timbre);

/**
 * @brief 播放音符（使用指定音色配置）
 * @param player 播放器指针
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param timbre_cfg 音色配置指针
 * @return 0 if 成功, -1 if 失败
 */
int player_play_note_with_config(AudioPlayer* player, float frequency,
                                 float duration, const TimbreConfig* timbre_cfg);

/**
 * @brief 播放铃声
 * @param player 播放器指针
 * @param duration 时长 (秒)
 * @return 0 if 成功, -1 if 失败
 */
int player_play_chime(AudioPlayer* player, float duration);

/**
 * @brief 播放钟琴音符
 * @param player 播放器指针
 * @param frequency 频率
 * @param duration 时长
 * @return 0 if 成功, -1 if 失败
 */
int player_play_bell(AudioPlayer* player, float frequency, float duration);

/**
 * @brief 播放点击音效
 * @param player 播放器指针
 * @param duration 时长
 * @return 0 if 成功, -1 if 失败
 */
int player_play_click(AudioPlayer* player, float duration);

/**
 * @brief 播放 tick 音效
 * @param player 播放器指针
 * @param duration 时长
 * @return 0 if 成功, -1 if 失败
 */
int player_play_tick(AudioPlayer* player, float duration);

/**
 * @brief 等待当前播放完成
 * @param player 播放器指针
 */
void player_wait(AudioPlayer* player);

/**
 * @brief 获取错误消息
 * @param player 播放器指针
 * @return 错误消息字符串
 */
const char* player_get_error(const AudioPlayer* player);

/*==============================================================================
 * 底层 PCM 生成函数
 *============================================================================*/

/**
 * @brief 生成正弦波 PCM 数据
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 ~ 1.0)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 */
uint32_t player_generate_sine(float frequency, float duration,
                               float volume, int16_t* output,
                               uint32_t max_samples);

/**
 * @brief 生成钢琴音色 PCM 数据
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量
 * @param timbre 音色配置
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 */
uint32_t player_generate_piano(float frequency, float duration,
                                float volume, const TimbreConfig* timbre,
                                int16_t* output, uint32_t max_samples);

/**
 * @brief 生成铃声 PCM 数据 (和弦)
 * @param duration 时长 (秒)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 */
uint32_t player_generate_chime(float duration, int16_t* output,
                                uint32_t max_samples);

/**
 * @brief 生成点击音效 PCM 数据
 * @param duration 时长 (秒)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 */
uint32_t player_generate_click(float duration, int16_t* output,
                               uint32_t max_samples);


#ifdef __cplusplus
}
#endif

#endif /* PLAYER_H */