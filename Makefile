# Makefile for 键盘发音 (Keyboard Sound)
# 商用级 C 代码

# 编译器
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS = -lm -lpthread -lpulse -lpulse-simple -lasound

# 警告选项
WARNINGS = -Werror -Wno-unused-parameter

# 目标文件
TARGET = keyboard_sound

# 源文件
SRCS = main.c \
       src/audio/audio_config.c \
       src/audio/adsr.c \
       src/audio/tone.c \
       src/audio/reverb.c \
       src/audio/player.c \
       src/input/keyboard.c \
       src/input/keymapping.c \
       src/config/config_loader_ini.c \
       src/config/config_manager.c \
       src/common/error.c

# 头文件
HDRS = include/audio/audio_config.h \
       include/audio/audio_config_types.h \
       include/audio/adsr.h \
       include/audio/tone.h \
       include/audio/reverb.h \
       include/audio/player.h \
       include/input/keyboard.h \
       include/input/keymapping.h \
       include/config/config_loader.h \
       include/config/config_manager.h \
       include/common/types.h \
       include/common/error.h

# 目标文件列表
OBJS = $(SRCS:.c=.o)

# 目录
SRC_DIR = .
OBJ_DIR = .

#==============================================================================
# 规则
#==============================================================================

.PHONY: all clean install run debug help test_simulate

all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

# 编译
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(WARNINGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "清理完成"

# 安装 (需要 root 权限)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	chmod 755 /usr/local/bin/$(TARGET)
	@echo "安装完成: /usr/local/bin/$(TARGET)"

# 卸载
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "卸载完成"

# 调试版本
debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)
	@echo "调试版本编译完成"

# 运行 (需要 root 权限访问 /dev/input)
run: $(TARGET)
	@echo "启动键盘发音... (按 Ctrl+C 退出)"
	sudo ./$(TARGET)

# 测试模拟 - 排除 main.o
TEST_OBJS = $(filter-out main.o, $(OBJS))
test_simulate: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o test_simulate test_simulate.c $(TEST_OBJS) $(LDFLAGS)
	@echo "编译完成: test_simulate"

# 帮助
help:
	@echo "键盘发音 - Makefile 帮助"
	@echo ""
	@echo "目标:"
	@echo "  all      - 编译程序 (默认)"
	@echo "  clean    - 清理编译文件"
	@echo "  install  - 安装到 /usr/local/bin"
	@echo "  uninstall- 卸载程序"
	@echo "  debug    - 编译调试版本"
	@echo "  run      - 编译并运行 (需要 root)"
	@echo "  test_simulate - 编译测试程序"
	@echo "  help     - 显示此帮助"
	@echo ""
	@echo "依赖:"
	@echo "  - Linux (evdev)"
	@echo "  - ALSA (aplay)"
	@echo "  - gcc, make"

#==============================================================================
# 依赖分析
#============================================================================#

src/audio/audio_config.o: src/audio/audio_config.c include/audio/audio_config.h include/config/config_manager.h include/common/error.h
src/audio/adsr.o: src/audio/adsr.c include/audio/adsr.h include/audio/audio_config_types.h
src/audio/tone.o: src/audio/tone.c include/audio/tone.h include/audio/adsr.h include/audio/audio_config_types.h
src/audio/reverb.o: src/audio/reverb.c include/audio/reverb.h include/audio/audio_config_types.h
src/audio/player.o: src/audio/player.c include/audio/player.h include/audio/tone.h include/audio/reverb.h include/audio/adsr.h include/audio/audio_config.h
src/input/keyboard.o: src/input/keyboard.c include/input/keyboard.h
src/input/keymapping.o: src/input/keymapping.c include/input/keymapping.h include/common/error.h
src/config/config_loader_ini.o: src/config/config_loader_ini.c include/config/config_loader.h include/common/error.h include/audio/audio_config_types.h include/input/keymapping.h
src/config/config_manager.o: src/config/config_manager.c include/config/config_manager.h include/config/config_loader.h include/common/error.h include/audio/audio_config_types.h include/input/keymapping.h
src/common/error.o: src/common/error.c include/common/error.h
main.o: main.c include/input/keyboard.h include/audio/player.h include/audio/audio_config.h include/input/keymapping.h