/**
 * @file main.c
 * @brief 键盘发音 - 主程序
 *
 * 功能:
 * - 全局键盘监听
 * - 多种音色分区
 * - 音效提示
 * - Ctrl+1/2/3/4 切换音色
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "input/keyboard.h"
#include "audio/player.h"
#include "audio/audio_config.h"
#include "input/keymapping.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/

/** 全局状态 */
typedef struct {
    AudioPlayer* player;
    KeyboardListener* keyboard;
    AudioConfigManager* config_manager;
    KeyMapping* key_mapping;

    float last_key_time;       /**< 上次按键时间 */
    float min_interval;        /**< 最小按键间隔 */

    TimbreType current_timbre; /**< 当前音色 */
    KeyCode active_modifiers[8];   /**< 当前按下的修饰键 */
    int modifier_count;

    bool use_special_effects;  /**< 是否使用特殊音效（铃声、点击声等） */
    bool sound_enabled;        /**< 是否启用声音输出 */

} AppState;

/*==============================================================================
 * 全局变量
 *============================================================================*/

static AppState g_app;

/*==============================================================================
 * 辅助函数
 *============================================================================*/

/**
 * @brief 获取当前时间 (秒)
 */
static float get_time_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (float)ts.tv_sec + (float)ts.tv_nsec / 1000000000.0f;
}

/**
 * @brief 查找按键对应的音符
 */
static const KeyNoteMap* find_key_note(KeyCode code)
{
    if (!g_app.key_mapping) {
        return NULL;
    }
    return key_mapping_find(code, g_app.key_mapping);
}

/**
 * @brief 播放音符
 */
static void play_note(KeyCode code, float frequency, const char* note, const char* desc)
{
    TimbreType timbre = key_mapping_get_timbre_for_key(code, g_app.key_mapping);
    const TimbreConfig* timbre_cfg = audio_config_manager_get_timbre(g_app.config_manager, timbre);

    printf("[%s] %s (%s) - %.1fHz\n",
           timbre_cfg ? timbre_cfg->name : "钢琴",
           note, desc, frequency);

    /* 检查声音是否启用 */
    if (!g_app.sound_enabled) {
        return;
    }

    /* 使用加载的音色配置播放音符 */
    if (timbre_cfg) {
        player_play_note_with_config(g_app.player, frequency, 0.15f, timbre_cfg);
    } else {
        player_play_note(g_app.player, frequency, 0.15f, timbre);
    }
}

/**
 * @brief 处理音色切换和模式切换
 */
static int handle_timbre_switch(KeyCode code)
{
    /* Ctrl + 1 -> 开关声音 */
    if (code == KEY_1) {
        g_app.sound_enabled = !g_app.sound_enabled;
        if (g_app.sound_enabled) {
            printf("声音: 已开启\n");
        } else {
            printf("声音: 已关闭\n");
        }
        return 1;
    }
    /* Ctrl + 2 -> 电子琴 */
    if (code == KEY_2) {
        g_app.current_timbre = TIMBRE_ELECTRIC;
        printf("音色切换: 电子琴\n");
        return 1;
    }
    /* Ctrl + 3 -> 钟琴 */
    if (code == KEY_3) {
        g_app.current_timbre = TIMBRE_BELL;
        printf("音色切换: 钟琴\n");
        return 1;
    }
    /* Ctrl + 4 -> 弦乐 */
    if (code == KEY_4) {
        g_app.current_timbre = TIMBRE_STRING;
        printf("音色切换: 弦乐\n");
        return 1;
    }
    /* Ctrl + 5 -> 切换特殊音效/基本音符模式 */
    if (code == KEY_5) {
        g_app.use_special_effects = !g_app.use_special_effects;
        if (g_app.use_special_effects) {
            printf("模式切换: 特殊音效模式 (铃声、点击声等)\n");
        } else {
            printf("模式切换: 基本音符模式 (钢琴音符)\n");
        }
        return 1;
    }
    return 0;
}

/**
 * @brief 处理特殊按键
 */
static void handle_special_key(KeyCode code)
{
    if (!g_app.sound_enabled) {
        return;  /* 声音已关闭，不播放 */
    }

    if (g_app.use_special_effects) {
        // 使用特殊音效模式
        switch (code) {
            case KEY_SPACE:
                printf("[铃声] SPACE\n");
                player_play_chime(g_app.player, 0.20f);
                break;
            case KEY_ENTER:
                printf("[铃声] ENTER\n");
                player_play_chime(g_app.player, 0.25f);
                break;
            case KEY_BACKSPACE:
                printf("[Tick] BACKSPACE\n");
                player_play_tick(g_app.player, 0.05f);
                break;
            case KEY_TAB:
            case KEY_ESC:
                printf("[点击] %s\n", keyboard_get_key_name(code));
                player_play_click(g_app.player, 0.08f);
                break;
            case KEY_UP:
            case KEY_DOWN:
            case KEY_LEFT:
            case KEY_RIGHT:
                printf("[提示] %s\n", keyboard_get_key_name(code));
                player_play_chime(g_app.player, 0.10f);
                break;
            default:
                break;
        }
    } else {
        // 使用基本音符模式
        switch (code) {
            case KEY_SPACE:
                printf("[音符] SPACE\n");
                player_play_note(g_app.player, 261.63f, 0.20f, TIMBRE_PIANO);
                break;
            case KEY_ENTER:
                printf("[音符] ENTER\n");
                player_play_note(g_app.player, 293.66f, 0.25f, TIMBRE_PIANO);
                break;
            case KEY_BACKSPACE:
                printf("[音符] BACKSPACE\n");
                player_play_note(g_app.player, 329.63f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_TAB:
                printf("[音符] TAB\n");
                player_play_note(g_app.player, 349.23f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_ESC:
                printf("[音符] ESC\n");
                player_play_note(g_app.player, 392.00f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_UP:
                printf("[音符] UP\n");
                player_play_note(g_app.player, 440.00f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_DOWN:
                printf("[音符] DOWN\n");
                player_play_note(g_app.player, 493.88f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_LEFT:
                printf("[音符] LEFT\n");
                player_play_note(g_app.player, 523.25f, 0.15f, TIMBRE_PIANO);
                break;
            case KEY_RIGHT:
                printf("[音符] RIGHT\n");
                player_play_note(g_app.player, 587.33f, 0.15f, TIMBRE_PIANO);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 键盘事件回调
 */
static void on_key_event(const KeyEvent* event, void* user_data)
{
    (void)user_data;

    if (event->type != KEY_EVENT_DOWN) {
        /* 处理按键释放 - 清除修饰键状态 */
        if (event->type == KEY_EVENT_UP) {
            for (int i = 0; i < g_app.modifier_count; i++) {
                if (g_app.active_modifiers[i] == event->code) {
                    g_app.active_modifiers[i] = g_app.active_modifiers[g_app.modifier_count - 1];
                    g_app.modifier_count--;
                    break;
                }
            }
        }
        return;
    }

    /* 检查是否是修饰键 */
    if (keyboard_is_modifier(event->code)) {
        /* 检查是否已存在 */
        int found = 0;
        for (int i = 0; i < g_app.modifier_count; i++) {
            if (g_app.active_modifiers[i] == event->code) {
                found = 1;
                break;
            }
        }
        if (!found && g_app.modifier_count < 8) {
            g_app.active_modifiers[g_app.modifier_count++] = event->code;
        }
        return;
    }

    /* 检查是否有 Ctrl 按下 */
    int has_ctrl = 0;
    for (int i = 0; i < g_app.modifier_count; i++) {
        if (g_app.active_modifiers[i] == KEY_LEFTCTRL ||
            g_app.active_modifiers[i] == KEY_RIGHTCTRL) {
            has_ctrl = 1;
            break;
        }
    }

    /* Ctrl 按下时处理音色切换 */
    if (has_ctrl) {
        if (handle_timbre_switch(event->code)) {
            return;
        }
    }

    /* 如果 Ctrl 按下，跳过数字键的音符播放 (避免 Ctrl+1, Ctrl+2 等冲突) */
    if (has_ctrl) {
        switch (event->code) {
            case KEY_1:
            case KEY_2:
            case KEY_3:
            case KEY_4:
            case KEY_5:
                return;  /* Ctrl+数字键用于功能切换，不播放音符 */
            default:
                break;
        }
    }

    /* 输入限速 - 字母按键使用更短的间隔 */
    float current_time = get_time_seconds();
    float interval = g_app.min_interval;

    /* 检查是否是字母按键 */
    const KeyNoteMap* note = find_key_note(event->code);
    if (note) {
        /* 字母按键使用更短的间隔，提高响应性 */
        interval = 0.02f;  /* 20毫秒 */
    }

    if (current_time - g_app.last_key_time < interval) {
        return;
    }
    g_app.last_key_time = current_time;

    /* 查找音符 */
    if (note) {
        play_note(note->code, note->frequency, note->note, note->desc);
        return;
    }

    /* 处理特殊按键 */
    handle_special_key(event->code);
}

/**
 * @brief 信号处理
 */
static void signal_handler(int sig)
{
    (void)sig;
    printf("\n收到信号，退出...\n");

    /* 设置停止标志 */
    if (g_app.keyboard) {
        keyboard_stop(g_app.keyboard);
    }

    /* 立即退出，让操作系统清理资源 */
    _exit(0);
}

/**
 * @brief 打印帮助信息
 */
static void print_help(void)
{
    const AudioConfig* audio_cfg = audio_config_manager_get_audio_config(g_app.config_manager);

    printf("==================================================\n");
    printf("键盘发音 - Keyboard Sound (C语言版)\n");
    printf("==================================================\n");
    printf("设备: %s\n", g_app.keyboard->device_name);
    printf("音频: %s\n", audio_cfg ? audio_cfg->audio_device : "default");
    printf("音量: %.1f%%\n", (audio_cfg ? audio_cfg->default_volume : 0.4f) * 100);
    printf("\n");
    printf("【按键说明】\n");
    printf("  QP行 / ASD行 / ZXC行 -> 钢琴/弦乐\n");
    printf("  数字行 -> 电子琴\n");
    printf("  Ctrl+1 -> 开关声音\n");
    printf("  Ctrl+2/3/4 -> 切换音色\n");
    printf("  Ctrl+5 -> 切换音效模式 (基本音符/特殊音效)\n");
    printf("  空格/回车 -> 音符/铃声\n");
    printf("  退格/Tab/Esc -> 音符/点击音\n");
    printf("\n");
    printf("按 Ctrl+C 退出\n");
    printf("==================================================\n");
}

/*==============================================================================
 * 主函数
 *============================================================================*/

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    /* 禁用stdout缓冲，确保调试输出立即显示 */
    setbuf(stdout, NULL);

    char keyboard_path[64];
    const char* device_path = NULL;
    const char* config_dir = NULL;

    /* 解析参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("用法: %s [设备路径] [配置目录]\n", argv[0]);
            printf("例如: %s /dev/input/event1 configs\n", argv[0]);
            return 0;
        }
        if (i == 1) {
            device_path = argv[i];
        } else if (i == 2) {
            config_dir = argv[i];
        }
    }

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化全局状态 */
    memset(&g_app, 0, sizeof(g_app));

    /* 初始化配置管理器 */
    g_app.config_manager = audio_config_manager_create(config_dir);
    if (!g_app.config_manager) {
        fprintf(stderr, "错误: 无法初始化配置管理器\n");
        return 1;
    }

    /* 获取配置 */
    const AudioConfig* audio_cfg = audio_config_manager_get_audio_config(g_app.config_manager);
    if (audio_cfg) {
        g_app.min_interval = audio_cfg->min_key_interval_ms / 1000.0f;
    } else {
        g_app.min_interval = 0.05f;  /* 默认 50ms */
    }
    g_app.current_timbre = TIMBRE_PIANO;
    g_app.use_special_effects = false;  /* 默认使用基本音符模式 */
    g_app.sound_enabled = true;         /* 默认启用声音 */

    /* 从配置管理器获取按键映射 */
    g_app.key_mapping = (KeyMapping*)audio_config_manager_get_key_mapping(g_app.config_manager);
    if (!g_app.key_mapping) {
        fprintf(stderr, "错误: 无法获取按键映射\n");
        audio_config_manager_destroy(g_app.config_manager);
        return 1;
    }

    /* 初始化键盘 */
    KeyboardListener keyboard;
    if (device_path) {
        strncpy(keyboard_path, device_path, sizeof(keyboard_path) - 1);
    } else {
        if (keyboard_find_keyboard(keyboard_path, sizeof(keyboard_path)) != 0) {
            fprintf(stderr, "错误: 未找到键盘设备\n");
            audio_config_manager_destroy(g_app.config_manager);
            return 1;
        }
    }

    if (keyboard_init(&keyboard, keyboard_path) != 0) {
        fprintf(stderr, "错误: 无法打开键盘设备 %s: %s\n",
                keyboard_path, strerror(errno));
        fprintf(stderr, "\n解决方法:\n");
        fprintf(stderr, "1. 运行: sudo usermod -a -G input $USER\n");
        fprintf(stderr, "2. 重新登录或重启系统\n");
        fprintf(stderr, "3. 或者使用: sudo ./keyboard_sound\n");
        fprintf(stderr, "\n现在进入测试模式，按Ctrl+C退出...\n");

        /* 测试模式：直接测试音频播放 */
        printf("\n测试音频播放...\n");
        AudioPlayer test_player;
        if (player_init(&test_player, audio_cfg ? audio_cfg->audio_device : "default",
                       audio_cfg ? audio_cfg->default_volume : 0.4f) == 0) {
            player_play_note(&test_player, 261.63f, 0.5f, TIMBRE_PIANO);
            player_play_note(&test_player, 329.63f, 0.5f, TIMBRE_PIANO);
            player_play_note(&test_player, 392.00f, 0.5f, TIMBRE_PIANO);
            player_wait(&test_player);
            player_destroy(&test_player);
            printf("测试完成！\n");
        } else {
            fprintf(stderr, "无法初始化音频播放器\n");
        }

        audio_config_manager_destroy(g_app.config_manager);
        return 0;
    }
    g_app.keyboard = &keyboard;

    /* 初始化播放器 */
    AudioPlayer player;
    if (player_init(&player, audio_cfg ? audio_cfg->audio_device : "default",
                   audio_cfg ? audio_cfg->default_volume : 0.4f) != 0) {
        fprintf(stderr, "错误: 无法初始化音频播放器\n");
        keyboard_destroy(&keyboard);
        audio_config_manager_destroy(g_app.config_manager);
        return 1;
    }
    g_app.player = &player;

    /* 打印帮助 */
    print_help();

    /* 设置回调 */
    keyboard_set_callback(&keyboard, on_key_event, NULL);

    /* 开始监听 */
    printf("开始监听键盘输入...\n");
    keyboard_listen(&keyboard);

    /* 清理 */
    keyboard_destroy(&keyboard);
    player_destroy(&player);
    audio_config_manager_destroy(g_app.config_manager);

    printf("再见!\n");
    return 0;
}
