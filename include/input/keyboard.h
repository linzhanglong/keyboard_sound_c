/**
 * @file keyboard.h
 * @brief 键盘监听器 - 使用 evdev
 *
 * 通过 Linux evdev 接口监听键盘输入
 * 支持按键按下/释放事件
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 常量定义
 *============================================================================*/

/** 最大按键名称长度 */
#define KEY_NAME_MAX  32

/** evdev 键码定义 (简化的部分键码) */
typedef enum {
    KEY_RESERVED = 0,
    KEY_ESC = 1,
    KEY_1 = 2,
    KEY_2 = 3,
    KEY_3 = 4,
    KEY_4 = 5,
    KEY_5 = 6,
    KEY_6 = 7,
    KEY_7 = 8,
    KEY_8 = 9,
    KEY_9 = 10,
    KEY_0 = 11,
    KEY_MINUS = 12,
    KEY_EQUAL = 13,
    KEY_BACKSPACE = 14,
    KEY_TAB = 15,
    KEY_Q = 16,
    KEY_W = 17,
    KEY_E = 18,
    KEY_R = 19,
    KEY_T = 20,
    KEY_Y = 21,
    KEY_U = 22,
    KEY_I = 23,
    KEY_O = 24,
    KEY_P = 25,
    KEY_LEFTBRACE = 26,
    KEY_RIGHTBRACE = 27,
    KEY_ENTER = 28,
    KEY_LEFTCTRL = 29,
    KEY_A = 30,
    KEY_S = 31,
    KEY_D = 32,
    KEY_F = 33,
    KEY_G = 34,
    KEY_H = 35,
    KEY_J = 36,
    KEY_K = 37,
    KEY_L = 38,
    KEY_SEMICOLON = 39,
    KEY_APOSTROPHE = 40,
    KEY_GRAVE = 41,
    KEY_LEFTSHIFT = 42,
    KEY_BACKSLASH = 43,
    KEY_Z = 44,
    KEY_X = 45,
    KEY_C = 46,
    KEY_V = 47,
    KEY_B = 48,
    KEY_N = 49,
    KEY_M = 50,
    KEY_COMMA = 51,
    KEY_DOT = 52,
    KEY_SLASH = 53,
    KEY_RIGHTSHIFT = 54,
    KEY_KPASTERISK = 55,
    KEY_LEFTALT = 56,
    KEY_SPACE = 57,
    KEY_CAPSLOCK = 58,
    KEY_F1 = 59,
    KEY_F2 = 60,
    KEY_F3 = 61,
    KEY_F4 = 62,
    KEY_F5 = 63,
    KEY_F6 = 64,
    KEY_F7 = 65,
    KEY_F8 = 66,
    KEY_F9 = 67,
    KEY_F10 = 68,
    KEY_NUMLOCK = 69,
    KEY_SCROLLLOCK = 70,
    KEY_KP7 = 71,
    KEY_KP8 = 72,
    KEY_KP9 = 73,
    KEY_KPMINUS = 74,
    KEY_KP4 = 75,
    KEY_KP5 = 76,
    KEY_KP6 = 77,
    KEY_KPPLUS = 78,
    KEY_KP1 = 79,
    KEY_KP2 = 80,
    KEY_KP3 = 81,
    KEY_KP0 = 82,
    KEY_KPDOT = 83,
    KEY_ZENKAKUHANKAKU = 85,
    KEY_102ND = 86,
    KEY_F11 = 87,
    KEY_F12 = 88,
    KEY_RO = 89,
    KEY_KATAKANA = 90,
    KEY_HIRAGANA = 91,
    KEY_HENKAN = 92,
    KEY_KATAKANAHIRAGANA = 93,
    KEY_MUHENKAN = 94,
    KEY_KPJPCOMMA = 95,
    KEY_KPENTER = 96,
    KEY_RIGHTCTRL = 97,
    KEY_KPSLASH = 98,
    KEY_SYSRQ = 99,
    KEY_RIGHTALT = 100,
    KEY_LINEFEED = 101,
    KEY_HOME = 102,
    KEY_UP = 103,
    KEY_PAGEUP = 104,
    KEY_LEFT = 105,
    KEY_RIGHT = 106,
    KEY_END = 107,
    KEY_DOWN = 108,
    KEY_PAGEDOWN = 109,
    KEY_INSERT = 110,
    KEY_DELETE = 111,
    KEY_MACRO = 112,
    KEY_MUTE = 113,
    KEY_VOLUMEDOWN = 114,
    KEY_VOLUMEUP = 115,
    KEY_POWER = 116,
    KEY_KPEQUAL = 117,
    KEY_KPPLUSMINUS = 118,
    KEY_PAUSE = 119,
    KEY_SCALE = 120,
    KEY_KPCOMMA = 121,
    KEY_HANGEUL = 122,
    KEY_HANGUEL = KEY_HANGEUL,
    KEY_HANJA = 123,
    KEY_YEN = 124,
    KEY_LEFTMETA = 125,
    KEY_RIGHTMETA = 127,
    KEY_COMPOSE = 127,
    KEY_STOP = 128,
    KEY_AGAIN = 129,
    KEY_PROPS = 130,
    KEY_UNDO = 131,
    KEY_FRONT = 132,
    KEY_COPY = 133,
    KEY_OPEN = 134,
    KEY_PASTE = 135,
    KEY_FIND = 136,
    KEY_CUT = 137,
    KEY_HELP = 138,
    KEY_MENU = 139,
    KEY_CALC = 140,
    KEY_SLEEP = 142,
    KEY_WAKEUP = 143,
    KEY_FILE = 144,
    KEY_SEND = 145,
    KEY_REPLY = 146,
    KEY_FORWARDMAIL = 147,
    KEY_SAVE = 148,
    KEY_DOCUMENTS = 149,
    KEY_BATTERY = 150,
    KEY_BLUETOOTH = 151,
    KEY_WLAN = 152,
    KEY_UWB = 153,
    KEY_UNKNOWN = 240,

    /* 结束标记 */
    KEY_MAX = 255
} KeyCode;

/** 按键事件类型 */
typedef enum {
    KEY_EVENT_UP = 0,    /**< 按键释放 */
    KEY_EVENT_DOWN = 1, /**< 按键按下 */
    KEY_EVENT_HOLD = 2  /**< 按键长按 (重复) */
} KeyEventType;

/** 按键事件 */
typedef struct {
    KeyCode code;           /**< 按键码 */
    KeyEventType type;      /**< 事件类型 */
    uint32_t timestamp_us;  /**< 时间戳 (微秒) */
} KeyEvent;

/** 键盘事件回调 */
typedef void (*KeyboardCallback)(const KeyEvent* event, void* user_data);

/** 键盘监听器 */
typedef struct {
    int fd;                     /**< evdev 文件描述符 */
    char device_path[64];      /**< 设备路径 */
    char device_name[128];     /**< 设备名称 */
    bool running;              /**< 是否正在运行 */
    KeyboardCallback callback; /**< 事件回调 */
    void* user_data;           /**< 用户数据 */
} KeyboardListener;

/*==============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化键盘监听器
 * @param listener 监听器指针
 * @param device_path 设备路径 (e.g., "/dev/input/event1")
 * @return 0 if 成功, -1 if 失败
 */
int keyboard_init(KeyboardListener* listener, const char* device_path);

/**
 * @brief 查找键盘设备
 * @param path_buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 0 if 成功, -1 if 未找到
 */
int keyboard_find_keyboard(char* path_buffer, size_t buffer_size);

/**
 * @brief 设置事件回调
 * @param listener 监听器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void keyboard_set_callback(KeyboardListener* listener,
                          KeyboardCallback callback,
                          void* user_data);

/**
 * @brief 开始监听 (阻塞)
 * @param listener 监听器指针
 * @return 0 if 正常退出, -1 if 出错
 *
 * @note 此函数会阻塞，直到出错或被中断
 */
int keyboard_listen(KeyboardListener* listener);

/**
 * @brief 停止监听
 * @param listener 监听器指针
 */
void keyboard_stop(KeyboardListener* listener);

/**
 * @brief 销毁键盘监听器
 * @param listener 监听器指针
 */
void keyboard_destroy(KeyboardListener* listener);

/**
 * @brief 获取按键名称
 * @param code 按键码
 * @return 按键名称字符串
 */
const char* keyboard_get_key_name(KeyCode code);

/**
 * @brief 检查是否是修饰键
 * @param code 按键码
 * @return true if 是修饰键, false if 不是
 */
bool keyboard_is_modifier(KeyCode code);

#ifdef __cplusplus
}
#endif

#endif /* KEYBOARD_H */