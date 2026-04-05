# 键盘发音程序 (Keyboard Sound)

> **AI 编程声明**
> 本项目由 AI 编程开发。
> - **编程工具**: [Claude Code](https://claude.ai/code)
> - **大模型**: MiniMax (MiniMax-M2)


### 层次说明

#### 1. 通用层 (Common Layer)
提供跨层共享的基础类型和工具：
- **types.h** - 定义共享数据类型
  - `KeyCode` - 键盘按键码枚举
  - `TimbreType` - 音色类型枚举 (钢琴、电子琴、钟琴、弦乐)
  - `ConfigResult` - 配置加载结果枚举
  - `AudioBackend` - 音频后端类型
- **error.h** - 错误处理宏和工具函数

#### 2. 输入层 (Input Layer)
处理键盘输入和按键映射：
- **keyboard.h/c** - 键盘设备输入处理
  - 打开/关闭键盘设备
  - 读取按键事件
  - 支持测试模式（无键盘设备时）
- **keymapping.h/c** - 按键到音符的映射管理
  - 维护按键-频率-音符-音色的映射表
  - 支持从配置文件加载映射
  - 提供按键查找接口

#### 3. 音频层 (Audio Layer)
负责音频生成和处理：
- **audio_config_types.h** - 音频配置数据结构
  - `AudioConfig` - 采样率、音量、设备等
  - `ADSRConfig` - ADSR 包络参数
  - `ReverbConfig` - 混响效果参数
  - `TimbreConfig` - 音色配置（谐波、包络、颤音）
- **audio_config.h/c** - 音频配置管理
  - 从 ConfigManager 加载配置
  - 提供配置访问接口
- **tone.h/c** - 音调生成
  - 基于谐波合成的音调生成
  - 支持多种音色
- **adsr.h/c** - ADSR 包络
  - 起音 (Attack) - 音量从 0 上升到峰值的时间
  - 衰减 (Decay) - 音量从峰值下降到持续音的时间
  - 持续 (Sustain) - 持续音量水平
  - 释音 (Release) - 音量从持续水平下降到 0 的时间
- **reverb.h/c** - 混响效果
  - 基于延迟线的混响算法
  - 支持房间大小、高频衰减等参数
- **player.h/c** - 音频播放器
  - 管理音频播放线程
  - 处理按键事件到音频生成的转换

#### 4. 配置层 (Configuration Layer)
负责配置文件的加载和管理：
- **config_loader.h/c** - 配置加载器接口
  - 支持 INI 格式配置文件
  - 提供音频、音色、按键映射的加载接口
- **config_manager.h/c** - 配置管理器
  - 中央配置存储
  - 配置加载失败时的默认值回退
  - 运行时配置修改支持

## 配置文件机制

### 配置文件格式 (INI)

程序使用 INI 格式的配置文件，分为三个文件：

#### 1. configs/config.ini - 主配置
```ini
[audio]
sample_rate=44100          # 采样率 (Hz)
default_volume=0.4         # 默认音量 (0.0 ~ 1.0)
audio_device=default       # 音频设备名称
min_key_interval_ms=50     # 最小按键间隔 (毫秒)

[adsr]
attack_ms=8                # 起音时间 (毫秒)
decay_ms=120               # 衰减时间 (毫秒)
sustain_level=0.65         # 持续音量 (0.0 ~ 1.0)
release_ms=180             # 释音时间 (毫秒)

[reverb]
enabled=true               # 是否启用混响
room_size=0.25             # 房间大小 (0.0 ~ 1.0)
damp=0.5                   # 高频衰减 (0.0 ~ 1.0)
wet=0.25                   # 混响音量比例
```

#### 2. configs/timbres.ini - 音色配置

**重要更新**: 所有音色现在都使用**钢琴基本音符**配置，确保声音轻柔统一。

所有音色（钢琴、电子琴、钟琴、弦乐）都采用相同的谐波配置和包络参数：

```ini
[piano]
attack_ms=20               # 起音时间（毫秒）
decay_ms=200               # 衰减时间（毫秒）
sustain=0.6                # 持续音量 (0.0 ~ 1.0)
release_ms=300             # 释音时间（毫秒）
harmonic_count=6           # 谐波数量
harmonic_1=1.0,0.80        # 基频（最强）
harmonic_2=2.0,0.15        # 第2谐波（低强度，避免刺耳）
harmonic_3=3.0,0.08        # 第3谐波
harmonic_4=4.0,0.04        # 第4谐波
harmonic_5=5.0,0.02        # 第5谐波
harmonic_6=6.0,0.01        # 第6谐波
harmonic_decay_base=2.0    # 谐波衰减基数
vibrato_depth=0.0          # 颤音深度
vibrato_rate=0.0           # 颤音速度

[electric]
# 使用与钢琴相同的配置
attack_ms=20
decay_ms=200
sustain=0.6
release_ms=300
harmonic_count=6
harmonic_1=1.0,0.80
harmonic_2=2.0,0.15
harmonic_3=3.0,0.08
harmonic_4=4.0,0.04
harmonic_5=5.0,0.02
harmonic_6=6.0,0.01
harmonic_decay_base=2.0
vibrato_depth=0.0
vibrato_rate=0.0

[bell]
# 使用与钢琴相同的配置
attack_ms=20
decay_ms=200
sustain=0.6
release_ms=300
harmonic_count=6
harmonic_1=1.0,0.80
harmonic_2=2.0,0.15
harmonic_3=3.0,0.08
harmonic_4=4.0,0.04
harmonic_5=5.0,0.02
harmonic_6=6.0,0.01
harmonic_decay_base=2.0
vibrato_depth=0.0
vibrato_rate=0.0

[string]
# 使用与钢琴相同的配置
attack_ms=20
decay_ms=200
sustain=0.6
release_ms=300
harmonic_count=6
harmonic_1=1.0,0.80
harmonic_2=2.0,0.15
harmonic_3=3.0,0.08
harmonic_4=4.0,0.04
harmonic_5=5.0,0.02
harmonic_6=6.0,0.01
harmonic_decay_base=2.0
vibrato_depth=0.0
vibrato_rate=0.0
```

> **注意**: 如果 INI 文件中未定义 `harmonic_N` 参数，程序将使用内置的默认谐波值。
> `harmonic_count` 仅用于声明谐波数量，实际谐波值需要通过 `harmonic_N=倍数,强度` 格式定义。

#### 3. configs/keymap.ini - 按键映射
```ini
[piano]
A=261.63
S=293.66
D=329.63
F=349.23
G=392.00
H=440.00
J=493.88
K=523.25
L=587.33
SEMICOLON=659.25
APOSTROPHE=698.46
Z=130.81
X=146.83
C=164.81
V=174.61
B=196.00
N=220.00
M=246.94

[electric]
Q=523.25
W=587.33
E=659.25
R=698.46
T=783.99
Y=880.00
U=987.77
I=1046.50
O=1174.66
P=1318.51

[bell]
1=523.25
2=587.33
3=659.25
4=698.46
5=783.99
6=880.00
7=987.77
8=1046.50
9=1174.66
0=1318.51
```

### 配置加载流程

1. **初始化阶段**
   - 创建 ConfigManager 实例
   - 创建默认按键映射

2. **配置加载**
   - 尝试从 `configs/config.ini` 加载音频配置
   - 尝试从 `configs/timbres.ini` 加载音色配置
   - 尝试从 `configs/keymap.ini` 加载按键映射

3. **回退机制**
   - 如果配置文件不存在或解析失败，使用内置默认值
   - 打印警告信息，但不影响程序运行

4. **运行时配置**
   - 音量可以通过程序接口动态调整
   - 音色切换通过快捷键 (Ctrl+1/2/3/4)

## 键盘布局

### 钢琴音区 (A-M 键)
```
A  S  D  F  G  H  J  K  L  ;  '
C4 D4 E4 F4 G4 A4 B4 C5 D5 E5 F5
```

### 低音区 (Z-M 键)
```
Z  X  C  V  B  N  M
C3 D3 E3 F3 G3 A3 B3
```

### 电子琴音区 (Q-P 键)
```
Q  W  E  R  T  Y  U  I  O  P
C5 D5 E5 F5 G5 A5 B5 C6 D6 E6
```

### 钟琴音区 (1-0 键)
```
1  2  3  4  5  6  7  8  9  0
C5 D5 E5 F5 G5 A5 B5 C6 D6 E6
```

## 快捷键功能

**注意**: 所有音色现在都使用相同的钢琴基本音符配置，音色切换功能保持兼容但效果相同。

| 快捷键 | 功能 | 说明 |
|--------|------|------|
| Ctrl+1 | 开关声音 | 开启/关闭所有音符输出 |
| Ctrl+2 | 电子琴 (Electric) | 使用钢琴配置 |
| Ctrl+3 | 钟琴 (Bell) | 使用钢琴配置 |
| Ctrl+4 | 弦乐 (String) | 使用钢琴配置 |
| Ctrl+5 | 切换音效模式 | 基本音符/特殊音效模式切换 |

## 音效模式

程序提供两种音效模式，可通过 Ctrl+5 切换：

### 1. 基本音符模式（默认）
- 所有按键都播放钢琴基本音符
- 声音轻柔统一，适合演奏音乐

### 2. 特殊音效模式
- Enter/Space → 铃声（和弦）
- Backspace → Tick 音
- Tab/Esc → 点击音
- 方向键 → 提示音
- 字母按键 → 正常音符

## 退出程序

- **Ctrl+C** - 退出程序

---

## 安装依赖

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install libpulse-dev libasound2-dev
```

### Fedora
```bash
sudo dnf install pulseaudio-libs-devel alsa-lib-devel
```

### Arch Linux
```bash
sudo pacman -S pulseaudio alsa-lib
```

## 编译

```bash
make clean
make
```

编译成功后会生成可执行文件 `keyboard_sound`。

## 运行

### 方法1：需要root权限（推荐）
```bash
sudo ./keyboard_sound
```

### 方法2：不需要root权限
```bash
# 1. 将用户添加到input组
sudo usermod -a -G input $USER

# 2. 重新登录或重启系统使权限生效

# 3. 运行程序
./keyboard_sound
```

### 方法3：测试模式
如果无法访问键盘设备，程序会自动进入测试模式：
```bash
./keyboard_sound
```
程序会播放3个音符来验证音频功能。

## 按键说明

**注意**: 所有按键现在都使用相同的钢琴基本音符配置，确保声音轻柔统一。

| 按键 | 功能（基本音符模式） | 音色 |
|------|----------------------|------|
| A-L | 音符 C4-B4 | 钢琴 |
| Z-M | 音符 C3-B3 | 钢琴 |
| Q-P | 音符 C5-A5 | 钢琴 |
| 0-9 | 高音符 | 钢琴 |
| 空格 | 音符 C4 | 钢琴 |
| 回车 | 音符 D4 | 钢琴 |
| 退格 | 音符 E4 | 钢琴 |
| Tab | 音符 F4 | 钢琴 |
| Esc | 音符 G4 | 钢琴 |
| 方向键 | 音符 A4/B4/C5/D5 | 钢琴 |

**特殊音效模式**（Ctrl+5 切换）：
- 空格/回车 → 铃声（和弦）
- 退格 → Tick 音
- Tab/Esc → 点击音
- 方向键 → 提示音

**快捷键功能**：
- Ctrl+1: 开关声音（开启/关闭所有音符输出）
- Ctrl+2/3/4: 切换音色（所有使用钢琴配置）
- Ctrl+5: 切换音效模式（基本音符/特殊音效）

## 故障排除

### 1. 无法打开键盘设备
```
错误: 无法打开键盘设备 /dev/input/event1: Permission denied
```

**解决方法：**
```bash
sudo usermod -a -G input $USER
# 然后重新登录或重启系统
```

### 2. PulseAudio连接失败
```
PulseAudio 连接失败: Connection refused
```

**解决方法：**
```bash
pulseaudio --start
# 或者
systemctl --user start pulseaudio
```

### 3. 没有声音
- 检查音量设置
- 检查音频设备是否正常工作
- 运行测试模式验证音频功能

## 技术细节

- **音频后端**: PulseAudio (首选) / ALSA (备选)
- **采样率**: 44.1kHz (CD品质)
- **音色合成**: 谐波叠加 + ADSR包络
- **线程模型**: 生产者-消费者模式
- **配置格式**: INI 格式，支持运行时配置

## 许可证

MIT License
