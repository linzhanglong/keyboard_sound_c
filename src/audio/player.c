/**
 * @file player.c
 * @brief 音频播放器实现
 */

/* 需要GNU扩展用于mkstemps - 必须在任何系统头文件之前定义 */
#define _GNU_SOURCE

#include "audio/player.h"
#include "audio/tone.h"
#include "audio/reverb.h"
#include "audio/adsr.h"
#include "audio/audio_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <alsa/asoundlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 常量定义
 *============================================================================*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0 * M_PI)

/** PCM 位深度 */
#define PCM_BITS      16

/** 最大缓存采样数 (约1秒) */
#define MAX_SAMPLES   44100

/*==============================================================================
 * 静态函数声明
 *============================================================================*/

static int play_pcm_directly(const char* device, const int16_t* pcm_data, uint32_t num_samples, int num_channels);
static float rand_float(void);
static void* playback_thread(void* arg);
static PlayTask* create_task(TaskType type, float frequency, float duration, TimbreType timbre);
static void enqueue_task(AudioPlayer* player, PlayTask* task);
static void process_task(AudioPlayer* player, PlayTask* task);

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 初始化音频播放器
 * @param player 音频播放器结构体指针
 * @param device 音频设备名称，为NULL时使用默认设备
 * @param volume 音量 (0.0 - 1.0)
 * @return 成功返回0，失败返回-1
 * @note 启动播放线程，初始化音频输出
 */
int player_init(AudioPlayer* player, const char* device, float volume)
{
    if (!player) return -1;

    memset(player, 0, sizeof(AudioPlayer));

    strncpy(player->device, device ? device : "default", sizeof(player->device) - 1);
    player->volume = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.5f;
    player->state = PLAYER_IDLE;

    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    /* 启动播放线程 */
    if (pthread_create(&player->play_thread, NULL, playback_thread, player) != 0) {
        pthread_mutex_destroy(&player->mutex);
        pthread_cond_destroy(&player->cond);
        return -1;
    }

    return 0;
}

/**
 * @brief 销毁音频播放器
 * @param player 音频播放器结构体指针
 * @note 停止播放线程，清理资源
 */
void player_destroy(AudioPlayer* player)
{
    if (!player) return;

    /* 停止播放线程 */
    pthread_mutex_lock(&player->mutex);
    player->stop_flag = 1;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);

    /* 等待线程结束 */
    pthread_join(player->play_thread, NULL);

    /* 清空任务队列 */
    PlayTask* task = player->task_queue;
    while (task) {
        PlayTask* next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&player->mutex);
    pthread_cond_destroy(&player->cond);
}

/**
 * @brief 播放音符
 * @param player 音频播放器结构体指针
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param timbre 音色类型
 * @return 成功返回0，失败返回-1
 * @note 将音符任务加入播放队列
 */
int player_play_note(AudioPlayer* player, float frequency,
                    float duration, TimbreType timbre)
{
    if (!player || frequency <= 0 || duration <= 0) {
        return -1;
    }

    /* 创建任务并加入队列 */
    PlayTask* task = create_task(TASK_NOTE, frequency, duration, timbre);
    if (!task) {
        return -1;
    }

    enqueue_task(player, task);
    return 0;
}

/**
 * @brief 使用自定义音色配置播放音符
 * @param player 音频播放器结构体指针
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param timbre_cfg 音色配置指针
 * @return 成功返回0，失败返回-1
 * @note 直接生成并播放，不通过任务队列
 */
int player_play_note_with_config(AudioPlayer* player, float frequency,
                                 float duration, const TimbreConfig* timbre_cfg)
{
    if (!player || frequency <= 0 || duration <= 0) {
        return -1;
    }

    /* 创建任务并加入队列 - 使用默认音色类型，但会在处理时使用提供的配置 */
    PlayTask* task = create_task(TASK_NOTE, frequency, duration, TIMBRE_PIANO);
    if (!task) {
        return -1;
    }

    /* 存储音色配置指针到任务的额外字段（需要修改 PlayTask 结构） */
    /* 暂时使用 timbre 字段存储配置指针的技巧：通过全局变量或修改结构 */
    /* 简化方案：直接生成音频并播放，不通过任务队列 */

    /* 生成 PCM 数据 */
    uint32_t num_samples = (uint32_t)(duration * 44100);
    int16_t* pcm = (int16_t*)malloc(num_samples * sizeof(int16_t));
    if (!pcm) {
        free(task);
        return -1;
    }

    uint32_t generated = player_generate_piano(frequency, duration,
                                               player->volume, timbre_cfg,
                                               pcm, num_samples);
    if (generated == 0) {
        free(pcm);
        free(task);
        return -1;
    }

    /* 播放 PCM 数据 */
    play_pcm_directly(player->device, pcm, generated, 1);
    free(pcm);
    free(task);

    return 0;
}

/**
 * @brief 播放铃声效果
 * @param player 音频播放器结构体指针
 * @param duration 时长 (秒)
 * @return 成功返回0，失败返回-1
 */
int player_play_chime(AudioPlayer* player, float duration)
{
    if (!player || duration <= 0) return -1;

    /* 创建任务并加入队列 */
    PlayTask* task = create_task(TASK_CHIME, 0.0f, duration, TIMBRE_PIANO);
    if (!task) {
        return -1;
    }

    enqueue_task(player, task);
    return 0;
}

/**
 * @brief 播放钟声效果
 * @param player 音频播放器结构体指针
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @return 成功返回0，失败返回-1
 */
int player_play_bell(AudioPlayer* player, float frequency, float duration)
{
    return player_play_note(player, frequency, duration, TIMBRE_PIANO);
}

/**
 * @brief 播放点击音效
 * @param player 音频播放器结构体指针
 * @param duration 时长 (秒)
 * @return 成功返回0，失败返回-1
 */
int player_play_click(AudioPlayer* player, float duration)
{
    if (!player || duration <= 0) return -1;

    /* 创建任务并加入队列 */
    PlayTask* task = create_task(TASK_CLICK, 0.0f, duration, TIMBRE_PIANO);
    if (!task) {
        return -1;
    }

    enqueue_task(player, task);
    return 0;
}

/**
 * @brief 播放滴答音效
 * @param player 音频播放器结构体指针
 * @param duration 时长 (秒)
 * @return 成功返回0，失败返回-1
 */
int player_play_tick(AudioPlayer* player, float duration)
{
    if (!player || duration <= 0) return -1;

    /* 创建任务并加入队列 */
    PlayTask* task = create_task(TASK_TICK, 0.0f, duration, TIMBRE_PIANO);
    if (!task) {
        return -1;
    }

    enqueue_task(player, task);
    return 0;
}

/**
 * @brief 等待播放完成
 * @param player 音频播放器结构体指针
 * @note 当前实现为同步播放，直接返回
 */
void player_wait(AudioPlayer* player)
{
    /* aplay 是同步执行的，所以这里什么都不做 */
    (void)player;
}

/**
 * @brief 获取播放器错误信息
 * @param player 音频播放器结构体指针
 * @return 错误信息字符串
 */
const char* player_get_error(const AudioPlayer* player)
{
    return player ? player->error_msg : "";
}

/*==============================================================================
 * 底层波形生成
 *============================================================================*/

/**
 * @brief 生成正弦波音频数据
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 - 1.0)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 */
uint32_t player_generate_sine(float frequency, float duration,
                              float volume, int16_t* output,
                              uint32_t max_samples)
{
    if (!output || frequency <= 0 || duration <= 0) {
        return 0;
    }

    uint32_t num_samples = (uint32_t)(duration * 44100);
    if (num_samples > max_samples) {
        num_samples = max_samples;
    }

    for (uint32_t i = 0; i < num_samples; i++) {
        float t = (float)i / 44100;
        float wave = sinf(TWO_PI * frequency * t) * volume;
        int16_t sample = (int16_t)(wave * 32767.0f);
        output[i] = sample;
    }

    return num_samples;
}

/**
 * @brief 生成钢琴音色音频数据
 * @param frequency 频率 (Hz)
 * @param duration 时长 (秒)
 * @param volume 音量 (0.0 - 1.0)
 * @param timbre 音色配置指针
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 * @note 包含谐波叠加、ADSR包络和低通滤波
 */
uint32_t player_generate_piano(float frequency, float duration,
                               float volume, const TimbreConfig* timbre,
                               int16_t* output, uint32_t max_samples)
{
    if (!output || frequency <= 0 || duration <= 0) {
        return 0;
    }

    uint32_t num_samples = (uint32_t)(duration * 44100);
    if (num_samples > max_samples) {
        num_samples = max_samples;
    }

    /* 分配临时缓冲区 */
    float* float_buf = (float*)malloc(num_samples * sizeof(float));
    if (!float_buf) {
        return 0;
    }

    /* 生成泛音叠加波形 */
    float max_harmonic = 0.0f;
    float prev_sample = 0.0f;  /* 用于低通滤波 */

    for (uint32_t i = 0; i < num_samples; i++) {
        float t = (float)i / 44100;
        float wave = 0.0f;

        if (timbre) {
            for (int h = 0; h < timbre->harmonic_count; h++) {
                float harmonic_freq = frequency * timbre->harmonics[h].multiplier;
                float decay_rate = timbre->harmonic_decay_base +
                                  timbre->harmonics[h].multiplier * 2.0f;
                float decay = expf(-t * decay_rate);

                /* 添加相位随机化，使声音更自然 */
                float phase = (float)h * 0.1f;  /* 简单的相位偏移 */
                float harmonic_wave = sinf(TWO_PI * harmonic_freq * t + phase) *
                                       decay * timbre->harmonics[h].strength;
                wave += harmonic_wave;
            }
            max_harmonic = 1.0f; /* 已在 timbre 中归一化 */
        } else {
            wave = sinf(TWO_PI * frequency * t);
            max_harmonic = 1.0f;
        }

        /* 应用简单的低通滤波器减少刺耳的高频 */
        float cutoff = 0.7f;  /* 滤波器截止频率比例 */
        wave = prev_sample + cutoff * (wave - prev_sample);
        prev_sample = wave;

        float_buf[i] = wave / max_harmonic;
    }

    /* 生成并应用 ADSR 包络 */
    ADSR_Envelope adsr;
    ADSR_Config adsr_cfg;

    /* 确保使用有效的 ADSR 配置 */
    adsr_cfg.attack = timbre ? timbre->attack : ADSR_ATTACK_TIME;
    adsr_cfg.decay = timbre ? timbre->decay : ADSR_DECAY_TIME;
    adsr_cfg.sustain = timbre ? timbre->sustain : ADSR_SUSTAIN_LEVEL;
    adsr_cfg.release = timbre ? timbre->release : ADSR_RELEASE_TIME;

    adsr_init(&adsr, &adsr_cfg, num_samples);
    adsr_trigger(&adsr, num_samples);

    for (uint32_t i = 0; i < num_samples; i++) {
        float envelope = adsr_process(&adsr);
        float sample = float_buf[i] * envelope * volume;

        /* 添加轻微的压缩，避免削波 */
        if (sample > 0.8f) {
            sample = 0.8f + (sample - 0.8f) * 0.3f;  /* 轻微压缩 */
        } else if (sample < -0.8f) {
            sample = -0.8f + (sample + 0.8f) * 0.3f;
        }

        /* 立体声 - 添加轻微的立体声宽度 */
        output[i] = (int16_t)(sample * 32767.0f);
    }

    free(float_buf);
    return num_samples;
}

/**
 * @brief 生成铃声效果音频数据
 * @param duration 时长 (秒)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 * @note 使用极柔和的纯正弦波，无谐波
 */
uint32_t player_generate_chime(float duration, int16_t* output,
                               uint32_t max_samples)
{
    if (!output || duration <= 0) {
        return 0;
    }

    uint32_t num_samples = (uint32_t)(duration * 44100);
    if (num_samples > max_samples) {
        num_samples = max_samples;
    }

    /* 极柔和铃声 - 纯正弦波，无任何泛音 */
    float freqs[] = {65.41f, 82.41f, 98.00f};  /* C2, E2, G2 - 极低频纯音 */
    float gains[] = {0.20f, 0.12f, 0.08f};  /* 极低增益 */
    int num_chords = 3;

    float* float_buf = (float*)malloc(num_samples * sizeof(float));
    if (!float_buf) {
        return 0;
    }

    memset(float_buf, 0, num_samples * sizeof(float));

    /* 极柔和起音 - 80ms 渐入，避免冲击声 */
    uint32_t attack_samples = (uint32_t)(0.080f * 44100);
    if (attack_samples > num_samples) {
        attack_samples = num_samples;
    }

    /* 极缓衰减 - 余音悠长 */
    float decay_rate = 0.6f;

    for (int c = 0; c < num_chords; c++) {
        float freq = freqs[c];
        float gain = gains[c];

        for (uint32_t i = 0; i < num_samples; i++) {
            float t = (float)i / 44100.0f;

            /* 柔和起音包络 */
            float attack_env = 1.0f;
            if (i < attack_samples) {
                attack_env = (float)i / attack_samples;
                attack_env = attack_env * attack_env;  /* 二次曲线，更柔和 */
            }

            /* 指数衰减 */
            float decay = 1.0f - expf(-t * decay_rate);

            /* 纯正弦波，无谐波 */
            float wave = sinf(TWO_PI * freq * t) * gain * attack_env * decay;

            float_buf[i] += wave;
        }
    }

    /* 终点淡出 - 最后100ms极平滑淡出 */
    uint32_t fadeout_samples = (uint32_t)(0.100f * 44100);
    if (fadeout_samples > num_samples) {
        fadeout_samples = num_samples;
    }

    for (uint32_t i = 0; i < fadeout_samples; i++) {
        float fade = 1.0f - (float)i / fadeout_samples;
        fade = fade * fade * fade;  /* 三次曲线，极平滑 */
        float_buf[num_samples - fadeout_samples + i] *= fade;
    }

    /* 归一化并避免clipping */
    float max_amp = 0.0f;
    for (uint32_t i = 0; i < num_samples; i++) {
        float abs_val = fabsf(float_buf[i]);
        if (abs_val > max_amp) {
            max_amp = abs_val;
        }
    }

    float scale = (max_amp > 0.0f) ? (0.80f / max_amp) : 0.0f;

    for (uint32_t i = 0; i < num_samples; i++) {
        float sample = float_buf[i] * scale;
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        output[i] = (int16_t)sample;
    }

    free(float_buf);
    return num_samples;
}
/**
 * @brief 生成点击音效音频数据
 * @param duration 时长 (秒)
 * @param output 输出缓冲区
 * @param max_samples 缓冲区最大采样数
 * @return 实际生成的采样数
 * @note 使用粉红噪声，更自然的点击声
 */
uint32_t player_generate_click(float duration, int16_t* output,
                              uint32_t max_samples)
{
    if (!output || duration <= 0) {
        return 0;
    }

    uint32_t num_samples = (uint32_t)(duration * 44100);
    if (num_samples > max_samples) {
        num_samples = max_samples;
    }

    /* 优化的点击音 - 使用粉红噪声而非白噪声，更柔和 */
    float prev_noise = 0.0f;
    for (uint32_t i = 0; i < num_samples; i++) {
        float t = (float)i / 44100;
        float decay = expf(-t * 8.0f);  /* 减慢衰减 */

        /* 生成粉红噪声（更自然的点击声） */
        float white_noise = rand_float() * 0.3f;
        prev_noise = prev_noise * 0.9f + white_noise * 0.1f;

        /* 添加轻微的音调成分，使声音更丰富 */
        float tone = sinf(TWO_PI * 800.0f * t) * 0.1f;

        float sample = (prev_noise + tone) * decay * 0.2f * 32767.0f;
        output[i] = (int16_t)sample;
    }

    return num_samples;
}


/*==============================================================================
 * 静态函数实现
 *============================================================================*/

/**
 * @brief 通过 ALSA 播放 PCM 数据（备选方案）
 */
static int play_pcm_alsa(const char* device, const int16_t* pcm_data, uint32_t num_samples, int num_channels)
{
    snd_pcm_t *pcm_handle = NULL;
    snd_pcm_hw_params_t *params = NULL;
    int err;

    /* 打开 PCM 设备 */
    err = snd_pcm_open(&pcm_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "ALSA 打开设备失败: %s\n", snd_strerror(err));
        return -1;
    }

    /* 分配硬件参数对象 */
    snd_pcm_hw_params_alloca(&params);

    /* 初始化硬件参数 */
    err = snd_pcm_hw_params_any(pcm_handle, params);
    if (err < 0) {
        fprintf(stderr, "ALSA 初始化参数失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    /* 设置参数 */
    err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "ALSA 设置访问模式失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        fprintf(stderr, "ALSA 设置格式失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    err = snd_pcm_hw_params_set_channels(pcm_handle, params, num_channels);
    if (err < 0) {
        fprintf(stderr, "ALSA 设置通道数失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    unsigned int sample_rate = 44100;
    err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0);
    if (err < 0) {
        fprintf(stderr, "ALSA 设置采样率失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    /* 设置缓冲区大小 */
    snd_pcm_uframes_t buffer_size = num_samples * 2;
    err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &buffer_size);
    if (err < 0) {
        fprintf(stderr, "ALSA 设置缓冲区大小失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    /* 应用参数 */
    err = snd_pcm_hw_params(pcm_handle, params);
    if (err < 0) {
        fprintf(stderr, "ALSA 应用参数失败: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    /* 写入 PCM 数据 */
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, pcm_data, num_samples);
    if (frames < 0) {
        frames = snd_pcm_recover(pcm_handle, frames, 0);
    }
    if (frames < 0) {
        fprintf(stderr, "ALSA 写入失败: %s\n", snd_strerror((int)frames));
        snd_pcm_close(pcm_handle);
        return -1;
    }

    /* 等待播放完成 */
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    return 0;
}

/**
 * @brief 通过 PulseAudio 播放 PCM 数据
 */
static int play_pcm_directly(const char* device, const int16_t* pcm_data, uint32_t num_samples, int num_channels)
{
    if (!device || !pcm_data || num_samples == 0) {
        return -1;
    }

    pa_simple *s = NULL;
    pa_sample_spec ss;
    pa_buffer_attr ba;
    int error;

    /* 设置音频格式 */
    ss.format = PA_SAMPLE_S16LE;  /* 16位有符号整数，小端 */
    ss.rate = 44100;
    ss.channels = num_channels;

    /* 设置缓冲区属性 */
    ba.maxlength = (uint32_t)(num_samples * num_channels * sizeof(int16_t) * 2);  /* 2倍缓冲 */
    ba.tlength = (uint32_t)(num_samples * num_channels * sizeof(int16_t));         /* 目标长度 */
    ba.prebuf = 0;      /* 不需要预缓冲 */
    ba.minreq = 0;      /* 最小请求 */
    ba.fragsize = 0;    /* 片段大小 */

    /* 打开 PulseAudio 连接 */
    s = pa_simple_new(NULL,               /* 默认服务器 */
                      "keyboard_sound",   /* 应用名称 */
                      PA_STREAM_PLAYBACK, /* 播放模式 */
                      NULL,               /* 默认设备 */
                      "Keyboard Sound",   /* 流名称 */
                      &ss,                /* 样本规格 */
                      NULL,               /* 默认通道映射 */
                      &ba,                /* 缓冲区属性 */
                      &error);            /* 错误码 */

    if (!s) {
        /* PulseAudio 不可用，尝试 ALSA */
        #ifdef DEBUG
        fprintf(stderr, "PulseAudio 连接失败: %s, 尝试 ALSA...\n", pa_strerror(error));
        #endif
        return play_pcm_alsa(device, pcm_data, num_samples, num_channels);
    }

    /* 写入 PCM 数据 */
    if (num_channels == 2) {
        /* 立体声：左右声道相同 */
        int16_t* stereo_data = (int16_t*)malloc(num_samples * num_channels * sizeof(int16_t));
        if (!stereo_data) {
            fprintf(stderr, "[DEBUG] 内存分配失败\n");
            pa_simple_free(s);
            return -1;
        }

        for (uint32_t i = 0; i < num_samples; i++) {
            stereo_data[i * 2] = pcm_data[i];     /* 左声道 */
            stereo_data[i * 2 + 1] = pcm_data[i]; /* 右声道 */
        }

        if (pa_simple_write(s, stereo_data, num_samples * num_channels * sizeof(int16_t), &error) < 0) {
            fprintf(stderr, "PulseAudio 写入失败: %s\n", pa_strerror(error));
            free(stereo_data);
            pa_simple_free(s);
            return -1;
        }
        free(stereo_data);
    } else {
        /* 单声道 */
        if (pa_simple_write(s, pcm_data, num_samples * sizeof(int16_t), &error) < 0) {
            fprintf(stderr, "PulseAudio 写入失败: %s\n", pa_strerror(error));
            pa_simple_free(s);
            return -1;
        }
    }

    /* 等待播放完成 */
    if (pa_simple_drain(s, &error) < 0) {
        fprintf(stderr, "PulseAudio 排空失败: %s\n", pa_strerror(error));
        pa_simple_free(s);
        return -1;
    }

    /* 关闭连接 */
    pa_simple_free(s);

    return 0;
}

static float rand_float(void)
{
    return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

/*==============================================================================
 * 播放队列实现
 *============================================================================*/

/**
 * @brief 创建播放任务
 */
static PlayTask* create_task(TaskType type, float frequency, float duration, TimbreType timbre)
{
    PlayTask* task = (PlayTask*)malloc(sizeof(PlayTask));
    if (!task) return NULL;

    task->type = type;
    task->frequency = frequency;
    task->duration = duration;
    task->timbre = timbre;
    task->next = NULL;

    return task;
}

/**
 * @brief 将任务加入队列
 */
static void enqueue_task(AudioPlayer* player, PlayTask* task)
{
    pthread_mutex_lock(&player->mutex);

    if (!player->task_queue) {
        player->task_queue = task;
        player->task_queue_tail = task;
    } else {
        player->task_queue_tail->next = task;
        player->task_queue_tail = task;
    }

    /* 唤醒播放线程 */
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
}

/**
 * @brief 处理单个播放任务
 */
static void process_task(AudioPlayer* player, PlayTask* task)
{
    int16_t* pcm = NULL;
    uint32_t num_samples = 0;
    uint32_t generated = 0;
    int num_channels = 1;  /* 默认单声道 */

    switch (task->type) {
        case TASK_NOTE: {
            const TimbreConfig* timbre_cfg = timbre_get_config(task->timbre);
            if (!timbre_cfg) {
                timbre_cfg = timbre_get_config(TIMBRE_PIANO);
            }

            num_samples = (uint32_t)(task->duration * 44100);
            pcm = (int16_t*)malloc(num_samples * sizeof(int16_t));
            if (!pcm) return;

            generated = player_generate_piano(task->frequency, task->duration,
                                              player->volume, timbre_cfg,
                                              pcm, num_samples);
            if (generated == 0) {
                free(pcm);
                return;
            }

            play_pcm_directly(player->device, pcm, generated, num_channels);
            free(pcm);
            break;
        }

        case TASK_CHIME: {
            num_samples = (uint32_t)(task->duration * 44100);
            pcm = (int16_t*)malloc(num_samples * sizeof(int16_t));
            if (!pcm) return;

            generated = player_generate_chime(task->duration, pcm, num_samples);
            if (generated == 0) {
                free(pcm);
                return;
            }

            num_channels = 2;  /* 铃声使用立体声 */
            play_pcm_directly(player->device, pcm, generated, num_channels);
            free(pcm);
            break;
        }

        case TASK_CLICK: {
            num_samples = (uint32_t)(task->duration * 44100);
            pcm = (int16_t*)malloc(num_samples * sizeof(int16_t));
            if (!pcm) return;

            generated = player_generate_click(task->duration, pcm, num_samples);
            if (generated == 0) {
                free(pcm);
                return;
            }

            num_channels = 2;  /* 点击音使用立体声 */
            play_pcm_directly(player->device, pcm, generated, num_channels);
            free(pcm);
            break;
        }

        case TASK_TICK: {
            num_samples = (uint32_t)(task->duration * 44100);
            pcm = (int16_t*)malloc(num_samples * sizeof(int16_t));
            if (!pcm) return;

            float* float_buf = (float*)malloc(num_samples * sizeof(float));
            if (!float_buf) {
                free(pcm);
                return;
            }

            for (uint32_t i = 0; i < num_samples; i++) {
                float t = (float)i / 44100;
                float decay = expf(-t * 100.0f);
                float wave = sinf(TWO_PI * 2000.0f * t) * decay;
                float_buf[i] = wave * player->volume * 0.2f;
            }

            for (uint32_t i = 0; i < num_samples; i++) {
                float sample = float_buf[i] * 32767.0f;
                if (sample > 32767.0f) sample = 32767.0f;
                if (sample < -32768.0f) sample = -32768.0f;
                pcm[i] = (int16_t)sample;
            }

            free(float_buf);

            num_channels = 2;  /* Tick音使用立体声 */
            play_pcm_directly(player->device, pcm, num_samples, num_channels);
            free(pcm);
            break;
        }

        default:
            break;
    }
}

/**
 * @brief 播放线程主函数
 */
static void* playback_thread(void* arg)
{
    AudioPlayer* player = (AudioPlayer*)arg;

    while (1) {
        pthread_mutex_lock(&player->mutex);

        /* 等待任务或停止信号 */
        while (!player->task_queue && !player->stop_flag) {
            pthread_cond_wait(&player->cond, &player->mutex);
        }

        /* 检查是否需要停止 */
        if (player->stop_flag) {
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        /* 取出任务 */
        PlayTask* task = player->task_queue;
        if (task) {
            player->task_queue = task->next;
            if (!player->task_queue) {
                player->task_queue_tail = NULL;
            }
        }

        pthread_mutex_unlock(&player->mutex);

        /* 处理任务 */
        if (task) {
            process_task(player, task);
            free(task);
        }
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif