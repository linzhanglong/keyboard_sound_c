/**
 * @file keyboard.c
 * @brief 键盘监听器实现 - 使用 evdev
 */

/* 特性测试宏 - 启用usleep */
#define _DEFAULT_SOURCE

#include "input/keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/select.h>

/*==============================================================================
 * evdev 事件结构 (从 linux/input.h)
 *============================================================================*/

#ifndef EV_KEY
#define EV_KEY 0x01
#endif

#ifndef EV_SYN
#define EV_SYN 0x00
#endif

/*==============================================================================
 * 按键码到名称的映射表
 *============================================================================*/

typedef struct {
    KeyCode code;
    const char* name;
} KeyNameMap;

static const KeyNameMap KEY_NAMES[] = {
    {KEY_ESC, "ESC"},
    {KEY_1, "1"},
    {KEY_2, "2"},
    {KEY_3, "3"},
    {KEY_4, "4"},
    {KEY_5, "5"},
    {KEY_6, "6"},
    {KEY_7, "7"},
    {KEY_8, "8"},
    {KEY_9, "9"},
    {KEY_0, "0"},
    {KEY_MINUS, "MINUS"},
    {KEY_EQUAL, "EQUAL"},
    {KEY_BACKSPACE, "BACKSPACE"},
    {KEY_TAB, "TAB"},
    {KEY_Q, "Q"},
    {KEY_W, "W"},
    {KEY_E, "E"},
    {KEY_R, "R"},
    {KEY_T, "T"},
    {KEY_Y, "Y"},
    {KEY_U, "U"},
    {KEY_I, "I"},
    {KEY_O, "O"},
    {KEY_P, "P"},
    {KEY_LEFTBRACE, "LEFTBRACE"},
    {KEY_RIGHTBRACE, "RIGHTBRACE"},
    {KEY_ENTER, "ENTER"},
    {KEY_LEFTCTRL, "LEFTCTRL"},
    {KEY_A, "A"},
    {KEY_S, "S"},
    {KEY_D, "D"},
    {KEY_F, "F"},
    {KEY_G, "G"},
    {KEY_H, "H"},
    {KEY_J, "J"},
    {KEY_K, "K"},
    {KEY_L, "L"},
    {KEY_SEMICOLON, "SEMICOLON"},
    {KEY_APOSTROPHE, "APOSTROPHE"},
    {KEY_GRAVE, "GRAVE"},
    {KEY_LEFTSHIFT, "LEFTSHIFT"},
    {KEY_BACKSLASH, "BACKSLASH"},
    {KEY_Z, "Z"},
    {KEY_X, "X"},
    {KEY_C, "C"},
    {KEY_V, "V"},
    {KEY_B, "B"},
    {KEY_N, "N"},
    {KEY_M, "M"},
    {KEY_COMMA, "COMMA"},
    {KEY_DOT, "DOT"},
    {KEY_SLASH, "SLASH"},
    {KEY_RIGHTSHIFT, "RIGHTSHIFT"},
    {KEY_KPASTERISK, "KPASTERISK"},
    {KEY_LEFTALT, "LEFTALT"},
    {KEY_SPACE, "SPACE"},
    {KEY_CAPSLOCK, "CAPSLOCK"},
    {KEY_F1, "F1"},
    {KEY_F2, "F2"},
    {KEY_F3, "F3"},
    {KEY_F4, "F4"},
    {KEY_F5, "F5"},
    {KEY_F6, "F6"},
    {KEY_F7, "F7"},
    {KEY_F8, "F8"},
    {KEY_F9, "F9"},
    {KEY_F10, "F10"},
    {KEY_NUMLOCK, "NUMLOCK"},
    {KEY_SCROLLLOCK, "SCROLLLOCK"},
    {KEY_KP7, "KP7"},
    {KEY_KP8, "KP8"},
    {KEY_KP9, "KP9"},
    {KEY_KPMINUS, "KPMINUS"},
    {KEY_KP4, "KP4"},
    {KEY_KP5, "KP5"},
    {KEY_KP6, "KP6"},
    {KEY_KPPLUS, "KPPLUS"},
    {KEY_KP1, "KP1"},
    {KEY_KP2, "KP2"},
    {KEY_KP3, "KP3"},
    {KEY_KP0, "KP0"},
    {KEY_KPDOT, "KPDOT"},
    {KEY_F11, "F11"},
    {KEY_F12, "F12"},
    {KEY_KPENTER, "KPENTER"},
    {KEY_RIGHTCTRL, "RIGHTCTRL"},
    {KEY_KPSLASH, "KPSLASH"},
    {KEY_RIGHTALT, "RIGHTALT"},
    {KEY_HOME, "HOME"},
    {KEY_UP, "UP"},
    {KEY_PAGEUP, "PAGEUP"},
    {KEY_LEFT, "LEFT"},
    {KEY_RIGHT, "RIGHT"},
    {KEY_END, "END"},
    {KEY_DOWN, "DOWN"},
    {KEY_PAGEDOWN, "PAGEDOWN"},
    {KEY_INSERT, "INSERT"},
    {KEY_DELETE, "DELETE"},
    {KEY_KPEQUAL, "KPEQUAL"},
    {KEY_KPPLUSMINUS, "KPPLUSMINUS"},
    {KEY_PAUSE, "PAUSE"},
    {KEY_SCALE, "SCALE"},
    {KEY_KPCOMMA, "KPCOMMA"},
    {KEY_LEFTMETA, "LEFTMETA"},
    {KEY_RIGHTMETA, "RIGHTMETA"},
    {KEY_COMPOSE, "COMPOSE"},
    {KEY_STOP, "STOP"},
    {KEY_AGAIN, "AGAIN"},
    {KEY_PROPS, "PROPS"},
    {KEY_UNDO, "UNDO"},
    {KEY_FRONT, "FRONT"},
    {KEY_COPY, "COPY"},
    {KEY_OPEN, "OPEN"},
    {KEY_PASTE, "PASTE"},
    {KEY_FIND, "FIND"},
    {KEY_CUT, "CUT"},
    {KEY_HELP, "HELP"},
    {KEY_MENU, "MENU"},
    {KEY_CALC, "CALC"},
    {KEY_SLEEP, "SLEEP"},
    {KEY_WAKEUP, "WAKEUP"},
    {KEY_FILE, "FILE"},
    {KEY_SEND, "SEND"},
    {KEY_REPLY, "REPLY"},
    {KEY_FORWARDMAIL, "FORWARDMAIL"},
    {KEY_SAVE, "SAVE"},
    {KEY_DOCUMENTS, "DOCUMENTS"},
    {KEY_UNKNOWN, "UNKNOWN"},
};

#define NUM_KEY_NAMES (sizeof(KEY_NAMES) / sizeof(KEY_NAMES[0]))

/*==============================================================================
 * 修饰键列表
 *============================================================================*/

static const KeyCode MODIFIER_KEYS[] = {
    KEY_LEFTSHIFT,
    KEY_RIGHTSHIFT,
    KEY_LEFTCTRL,
    KEY_RIGHTCTRL,
    KEY_LEFTALT,
    KEY_RIGHTALT,
    KEY_LEFTMETA,
    KEY_RIGHTMETA,
};

#define NUM_MODIFIERS (sizeof(MODIFIER_KEYS) / sizeof(MODIFIER_KEYS[0]))

/*==============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 查找键盘设备路径
 * @param path_buffer 输出缓冲区，用于存储找到的设备路径
 * @param buffer_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 * @note 如果未找到具体键盘设备，使用默认路径 /dev/input/event1
 */
int keyboard_find_keyboard(char* path_buffer, size_t buffer_size)
{
    if (!path_buffer || buffer_size < 16) {
        return -1;
    }

    DIR* dir;
    struct dirent* ent;
    char dev_path[280];
    char name[256];

    /* 遍历 /dev/input 目录 */
    dir = opendir("/dev/input");
    if (!dir) {
        /* 尝试 /dev 目录 */
        dir = opendir("/dev");
    }
    if (!dir) {
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        /* 跳过 . 和 .. */
        if (ent->d_name[0] == '.') {
            continue;
        }

        /* 查找 event 设备 */
        if (strncmp(ent->d_name, "event", 5) != 0) {
            continue;
        }

        snprintf(dev_path, sizeof(dev_path), "/dev/input/%s", ent->d_name);

        /* 尝试打开并读取设备名称 */
        int fd = open(dev_path, O_RDONLY);
        if (fd < 0) {
            continue;
        }

        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
            /* 查找键盘设备 */
            if (strstr(name, "AT Translated") != NULL ||
                strstr(name, "keyboard") != NULL ||
                strstr(name, "Keyboard") != NULL) {
                /* 找到键盘 */
                strncpy(path_buffer, dev_path, buffer_size - 1);
                path_buffer[buffer_size - 1] = '\0';
                close(fd);
                closedir(dir);
                return 0;
            }
        }

        close(fd);
    }

    closedir(dir);

    /* 如果没找到具体的键盘，使用默认路径 */
    strncpy(path_buffer, "/dev/input/event1", buffer_size - 1);
    path_buffer[buffer_size - 1] = '\0';
    return 0;
}

/**
 * @brief 初始化键盘监听器
 * @param listener 键盘监听器结构体指针
 * @param device_path 设备路径，为NULL时自动查找键盘设备
 * @return 成功返回0，失败返回-1
 * @note 需要root权限或用户加入input组才能访问设备
 */
int keyboard_init(KeyboardListener* listener, const char* device_path)
{
    if (!listener) {
        return -1;
    }

    memset(listener, 0, sizeof(KeyboardListener));

    /* 如果没有指定设备路径，自动查找 */
    if (!device_path) {
        if (keyboard_find_keyboard(listener->device_path,
                                   sizeof(listener->device_path)) != 0) {
            return -1;
        }
    } else {
        strncpy(listener->device_path, device_path,
                sizeof(listener->device_path) - 1);
    }

    /* 打开设备 - 尝试只读模式，如果失败则尝试其他模式 */
    listener->fd = open(listener->device_path, O_RDONLY | O_NONBLOCK);
    if (listener->fd < 0) {
        /* 如果只读失败，尝试其他模式 */
        listener->fd = open(listener->device_path, O_RDWR | O_NONBLOCK);
    }
    if (listener->fd < 0) {
        fprintf(stderr, "无法打开设备 %s: %s\n", listener->device_path, strerror(errno));
        fprintf(stderr, "提示: 需要root权限或用户加入input组\n");
        fprintf(stderr, "解决方法: sudo usermod -a -G input $USER\n");
        return -1;
    }
    printf("成功打开键盘设备: %s\n", listener->device_path);

    /* 获取设备名称 */
    if (ioctl(listener->fd, EVIOCGNAME(sizeof(listener->device_name)),
              listener->device_name) < 0) {
        strncpy(listener->device_name, "Unknown Keyboard",
                sizeof(listener->device_name) - 1);
    }

    listener->running = false;
    listener->callback = NULL;
    listener->user_data = NULL;

    return 0;
}

/**
 * @brief 设置键盘事件回调函数
 * @param listener 键盘监听器结构体指针
 * @param callback 回调函数指针
 * @param user_data 用户自定义数据，将传递给回调函数
 * @note 回调函数将在按键事件发生时被调用
 */
void keyboard_set_callback(KeyboardListener* listener,
                          KeyboardCallback callback,
                          void* user_data)
{
    if (listener) {
        listener->callback = callback;
        listener->user_data = user_data;
    }
}

/**
 * @brief 开始监听键盘事件
 * @param listener 键盘监听器结构体指针
 * @return 成功返回0，失败返回-1
 * @note 使用select()系统调用实现事件监听，CPU占用率低
 * @note 监听循环会持续运行，直到调用keyboard_stop()停止
 */
int keyboard_listen(KeyboardListener* listener)
{
    if (!listener || listener->fd < 0) {
        return -1;
    }

    struct input_event ev;
    fd_set read_fds;
    struct timeval timeout;
    int ret;

    listener->running = true;

    /* 设置为阻塞模式（select会等待） */
    int flags = fcntl(listener->fd, F_GETFL, 0);
    fcntl(listener->fd, F_SETFL, flags & ~O_NONBLOCK);

    while (listener->running) {
        /* 清空文件描述符集合 */
        FD_ZERO(&read_fds);
        FD_SET(listener->fd, &read_fds);

        /* 设置超时时间 - 使用select等待事件 */
        timeout.tv_sec = 1;  /* 1秒超时 */
        timeout.tv_usec = 0;

        /* 等待文件描述符可读 */
        ret = select(listener->fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ret < 0) {
            if (errno == EINTR) {
                /* 被信号中断，继续等待 */
                continue;
            }
            break;
        } else if (ret == 0) {
            /* 超时，检查是否需要停止 */
            continue;
        }

        /* 有事件可读 */
        if (FD_ISSET(listener->fd, &read_fds)) {
            ssize_t n = read(listener->fd, &ev, sizeof(ev));

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                break;
            }

            if (n != sizeof(ev)) {
                continue;
            }

            /* 只处理按键事件 */
            if (ev.type != EV_KEY) {
                continue;
            }

            /* 构造事件结构 */
            KeyEvent key_ev;
            key_ev.code = (KeyCode)ev.code;
            key_ev.timestamp_us = ev.time.tv_sec * 1000000 + ev.time.tv_usec;

            if (ev.value == 0) {
                key_ev.type = KEY_EVENT_UP;
            } else if (ev.value == 2) {
                key_ev.type = KEY_EVENT_HOLD;
            } else {
                key_ev.type = KEY_EVENT_DOWN;
            }

            /* 调用回调 */
            if (listener->callback) {
                listener->callback(&key_ev, listener->user_data);
            }
        }
    }

    return 0;
}

/**
 * @brief 停止键盘监听
 * @param listener 键盘监听器结构体指针
 * @note 设置running标志为false，使监听循环退出
 */
void keyboard_stop(KeyboardListener* listener)
{
    if (listener) {
        listener->running = false;
    }
}

/**
 * @brief 销毁键盘监听器
 * @param listener 键盘监听器结构体指针
 * @note 关闭设备文件描述符并清理资源
 */
void keyboard_destroy(KeyboardListener* listener)
{
    if (listener) {
        if (listener->fd >= 0) {
            close(listener->fd);
            listener->fd = -1;
        }
        listener->running = false;
    }
}

/**
 * @brief 获取按键名称
 * @param code 按键码
 * @return 按键名称字符串，未知按键返回"UNKNOWN"
 */
const char* keyboard_get_key_name(KeyCode code)
{
    for (size_t i = 0; i < NUM_KEY_NAMES; i++) {
        if (KEY_NAMES[i].code == code) {
            return KEY_NAMES[i].name;
        }
    }
    return "UNKNOWN";
}

/**
 * @brief 判断按键是否为修饰键
 * @param code 按键码
 * @return 是修饰键返回true，否则返回false
 * @note 修饰键包括：Shift、Ctrl、Alt、Meta等
 */
bool keyboard_is_modifier(KeyCode code)
{
    for (size_t i = 0; i < NUM_MODIFIERS; i++) {
        if (MODIFIER_KEYS[i] == code) {
            return true;
        }
    }
    return false;
}