
## AI Robot-M
#### 硬件架构配置

```

    电源：7.4V 2200mAh锂电池 ✅ -
    稳压板：ML2596电源稳压板 -

    主板：ESP32-S3-DEV-KIT-N32R16V-M -
    主板集线器：IIC HUB模块 分线器 I2C集线器 ✅ -

    舵机转换板：URT-2 ✅
    总线舵机集线器： 舵机集线器 TTL分线板 ✅ -
    12个飞特舵机：SC-0017-C001
    步态平衡：601N16轴BMI323 ✅ -
    障碍识别：TCRT5000红外反射传感器    ToF测距模块VL53L1X ✅ -

    屏幕：4.0寸SPI串口 TFT液晶屏电容触摸屏 驱动IC ST7796S ✅ -
    摄影头：ESP32串口转 带OV2640摄像头
    麦克风：INMP441 ✅ -
    音频：MAX98357 I2S 音频放大器 ✅ -
    

```

### Robot-M 开发与接线手册

本文档同时说明两套固件：

- **PlatformIO/Arduino 固件**：位于仓库根目录，主要用于硬件、屏幕、I2S 音频、I2C 传感器和 RobotRuntime 的单项调试。
- **小智 ESP-IDF 固件**：位于 `xiaozhi-esp32/`，用于语音交互、屏幕 UI、配网和联网运行。

两套固件使用同一块 ESP32-S3 和相同的主要引脚，但不能同时运行；烧录哪套固件，取决于当前要测试硬件还是使用小智功能。

#### 快速开始

##### 1. PlatformIO/Arduino 固件

在仓库根目录执行。需要先安装 PlatformIO，并确认 `pio` 已加入终端 `PATH`。

```sh
cd /Users/hemingming/worker/Robot-M

# 编译当前 Arduino 固件
pio run

# 连接 ESP32 后烧录；也可以追加 --upload-port 指定串口
pio run --target upload
# pio run --target upload --upload-port /dev/cu.usbmodem5B900929761

# 打开 115200 波特率串口监视器
pio device monitor --baud 115200
```

PlatformIO 固件没有独立的“启动命令”。烧录后 ESP32 复位，Arduino 会自动执行 `setup()`，随后循环执行 `loop()`。启动日志会输出供电、I2C、显示、音频和 RobotRuntime 状态。

##### 2. 小智 ESP-IDF 固件

小智工程要求 ESP-IDF 6.0.1 或更高版本，当前推荐 ESP-IDF 6.1。每次打开新终端都要先加载 ESP-IDF 环境：

```sh
cd /Users/hemingming/worker/Robot-M/xiaozhi-esp32
source /Users/hemingming/esp/esp-idf/export.sh
```

编译 Robot-M 小智固件：

```sh
python3 scripts/build.py robot-m --name robot-m
```

看到 `Project build complete` 表示编译成功。指定串口烧录完整固件：

```sh
idf.py -p /dev/cu.usbmodemXXXX flash
```

烧录后直接查看日志，可以合并成一条命令：
```sh
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

只烧录已经生成的应用镜像时，可以使用：

```sh
idf.py -p /dev/cu.usbmodemXXXX app-flash
```

注意：`app-flash` 在构建目录缓存不一致时仍可能触发增量编译。修改了代码或配置后，先重新执行 `python3 scripts/build.py robot-m --name robot-m`，再烧录。

查看小智串口日志：

```sh
idf.py -p /dev/cu.usbmodemXXXX monitor
```

退出串口监视器通常使用 `Ctrl+]`。如果自动下载失败，按住 **BOOT**，短按一次 **EN/RESET**，松开 BOOT 后重新执行烧录命令。

#### 测试命令与验证方式

```sh
# PlatformIO 编译检查
pio run

# PlatformIO 单元测试入口；当前仓库暂时没有 test/ 测试目录
pio test

# ESP-IDF 编译检查
cd xiaozhi-esp32
source /Users/hemingming/esp/esp-idf/export.sh
python3 scripts/build.py robot-m --name robot-m
```

当前项目的硬件验证以串口日志和实际外设表现为主：

- 显示屏：启动后出现三段红、绿、蓝测试区域，并显示麦克风电平条。
- I2C：PlatformIO 固件启动时输出 ToF `0x29` 和 BMI323 `0x68` 是否响应。
- 麦克风：对着 INMP441 说话，串口的 `Microphone level` 应明显变化。
- 小智：烧录后屏幕显示 UI，按提示配网，再说“你好，小智”测试唤醒和语音链路。
- 舵机：当前代码只建立了 `actuators/` 和 `motion/` 接口，尚未自动发送动作指令。

##### TCRT5000 红外传感器接线与测试

当前 Arduino/PlatformIO 固件读取两个 TCRT5000 模块的数字输出 `DO`：

| 模块引脚 | 左侧模块 | 右侧模块 | 说明 |
| --- | --- | --- | --- |
| `VCC` | 3.3V | 3.3V | 两个模块都使用 ESP32 的 3.3V |
| `GND` | GND | GND | 必须与 ESP32 共地 |
| `DO` | GPIO14 | GPIO21 | 数字比较器输出，固件周期性打印状态 |
| `AO` | 暂不接 | 暂不接 | 当前固件不读取模拟输出 |

接线示意：

```text
3.3V  ──┬── 左侧 TCRT5000 VCC
    └── 右侧 TCRT5000 VCC

GND   ──┬── 左侧 TCRT5000 GND
    └── 右侧 TCRT5000 GND

GPIO14 ───── 左侧 TCRT5000 DO
GPIO21 ───── 右侧 TCRT5000 DO
```

烧录 Arduino 固件后，打开串口监视器：

```sh
export PATH="$HOME/Library/Python/3.14/bin:$PATH"
cd /Users/hemingming/worker/Robot-M
pio device monitor --port /dev/cu.usbmodem5B900929761 --baud 115200
```

串口会周期性显示：

```text
TCRT5000: left=1 right=1
```

测试步骤：

1. 用手或黑色物体靠近左侧传感器，观察 `left` 是否变化。
2. 用手或黑色物体靠近右侧传感器，观察 `right` 是否变化。
3. 分别调节两个模块上的电位器，使靠近和移开时 `DO` 能稳定切换。
4. 部分模块是低电平触发，`1 -> 0` 可能表示检测到物体；以实际变化为准。

如果状态不变化，依次检查 `VCC`、`GND`、对应 `DO` GPIO、模块指示灯和电位器。USB 重插后串口名称可能变化，可先执行 `pio device list` 查看当前端口。

#### ESP32-S3 GPIO 完整映射

以下表格以当前 `src/config/pin_def.h` 和 `xiaozhi-esp32/main/boards/robot-m/config.h` 为准。GPIO 数字是 ESP32-S3 管脚编号，不是排针位置编号。

##### 电源、采样和总线

| 功能 | ESP32-S3 GPIO/电源 | 外设端 | 说明 |
| --- | --- | --- | --- |
| 电池电压采样 | GPIO4 | 电池分压输出 | 必须经过电阻分压，不能把 7.4V 电池直接接入 ADC |
| 外设供电使能 | GPIO5 | 稳压板使能输入 | 固件启动时拉高 |
| I2C SDA | GPIO38 | HUB SDA、触摸 SDA | 400 kHz，多个 I2C 设备共用 |
| I2C SCL | GPIO39 | HUB SCL、触摸 SCL | 400 kHz，多个 I2C 设备共用 |
| 逻辑电源 | 3.3V | ESP32、INMP441、传感器 | 不要用 5V 给 3.3V 设备供电 |
| 功率电源 | 稳定 5V | TFT VCC、MAX98357A VIN | 按模块规格供电 |
| 公共地 | GND | 所有模块 | ESP32、音频、屏幕、传感器必须共地 |

##### ST7796S 4.0 英寸 SPI 屏幕

| 屏幕引脚 | ESP32-S3 | 固件宏/说明 |
| --- | --- | --- |
| `SDO/MISO` | GPIO13 | `DISPLAY_SPI_MISO_PIN` |
| `SCK` | GPIO12 | `DISPLAY_SPI_SCLK_PIN` |
| `SDI/MOSI` | GPIO11 | `DISPLAY_SPI_MOSI_PIN` |
| `LCD_RS/DC` | GPIO9 | `DISPLAY_DC_PIN` |
| `LCD_RST` | GPIO8 | `DISPLAY_RST_PIN` |
| `LCD_CS` | GPIO10 | `DISPLAY_CS_PIN` |
| `CTP_SCL` | GPIO39 | 与 I2C SCL 共用 |
| `CTP_SDA` | GPIO38 | 与 I2C SDA 共用 |
| `CTP_INT` | GPIO7 | 触摸中断，当前小智只预留 |
| `CTP_RST` | GPIO6 | 触摸复位，当前小智只预留 |
| `VCC` | 稳定 5V | 按屏幕模块规格供电 |
| `GND` | GND | 与 ESP32 共地 |
| `LED` | 按屏幕背光要求接电 | 不要让 ESP32 GPIO 直接承担约 103mA 背光电流 |
| `SD_CS` | 暂不接 | 屏幕板载 SD 卡片选未使用 |

当前小智屏幕配置为 `320x480` 竖屏，`DISPLAY_MIRROR_X=false`、`DISPLAY_MIRROR_Y=true`、`DISPLAY_SWAP_XY=false`。如果画面出现上下颠倒，优先检查这三个方向参数，不要先改接线。

##### INMP441 麦克风

| INMP441 引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VDD` | 3.3V | 不能接 5V |
| `GND` | GND | 必须共地 |
| `SCK` | GPIO1 | 麦克风独立 BCLK，不能与功放共用 |
| `WS` | GPIO2 | 麦克风独立 WS，不能与功放共用 |
| `SD` | GPIO16 | 麦克风串行数据输出 |
| `L/R` | GND | 选择左声道；固件使用 `ONLY_LEFT` |

##### MAX98357A I2S 功放

| MAX98357A 引脚 | ESP32-S3/电源 | 说明 |
| --- | --- | --- |
| `VIN` | 稳定 5V | 功放电源 |
| `GND` | GND | 与 ESP32 共地 |
| `BCLK` | GPIO17 | 功放独立 BCLK |
| `LRC/WS` | GPIO18 | 功放独立 WS |
| `DIN` | GPIO15 | 功放音频数据输入 |
| `SD` | 3.3V | 拉高使功放保持开启 |
| `GAIN` | 暂时悬空 | 按模块默认增益工作 |
| `SPK+/-` | 喇叭正负端 | 喇叭两端只能接功放输出，不能接 ESP32 GND |

##### I2C 传感器

| 设备 | 总线 | 地址/引脚 | 当前状态 |
| --- | --- | --- | --- |
| BMI323 IMU | GPIO38/39 I2C | `0x68`；`CS -> 3.3V`，`SA0 -> GND` | PlatformIO 代码可探测，数据采集待接入 |
| VL53L1X ToF | GPIO38/39 I2C | `0x29` | PlatformIO 代码可启动测距，受 `kSensorsEnabled` 控制 |
| 电容触摸控制器 | GPIO38/39 I2C | `INT -> GPIO7`，`RST -> GPIO6` | 小智板卡当前只预留引脚 |
| I2C HUB | GPIO38/39 | `SDA -> GPIO38`，`SCL -> GPIO39` | 需要 3.3V、GND 和合适的上拉 |

##### TCRT5000 红外模块

| 模块 | VCC | GND | 数字输出 `DO` | 模拟输出 `AO` |
| --- | --- | --- | --- | --- |
| 左侧 | 3.3V | GND | GPIO14 | 暂不接 |
| 右侧 | 3.3V | GND | GPIO21 | 暂不接 |

`DO` 高低电平逻辑可能因模块比较器和电位器方向不同而相反，需要实际测量确认。先调好灵敏度，再接入运动安全逻辑。

##### 舵机、URT-2 和摄像头

| 设备 | 当前连接状态 |
| --- | --- |
| 12 个 SC-0017-C001 总线舵机 | 电源板 `VADJ+` -> URT-2/舵机集线器电源+；电源板 GND -> 集线器 GND；ESP32 GND -> URT-2 信号地 |
| URT-2 信号 TX/RX | 默认 `TX -> GPIO19`、`RX -> GPIO20`、1000000 8N1；接线前按 URT-2 丝印确认并可在 `src/config/pin_def.h` 修改 |
| 舵机电源 | 不得从 ESP32 5V/3.3V 取电；先断开舵机负载，用万用表将 `VADJ` 调到舵机额定电压 |
| OV2640 摄像头 | 硬件已列入规划，但当前 Robot-M 自定义板配置未定义摄像头 GPIO |

驱动已完成 UART 初始化、SCS/SMS 舵机 Ping、单舵机目标位置发送和广播停扭力；启动时不会自动移动。首次测试只接 1 个舵机，先确认 ID 和零位，再逐步接入其余舵机。当前默认 `TX=GPIO19`、`RX=GPIO20`、1000000 8N1；如果 URT-2 丝印或手册不同，必须修改配置后再烧录。

串口测试命令（每条命令后按回车）：

```text
servo ping 1
servo scan 1 20
servo move 1 2048 500 10
servo stop
```

`servo scan 1 20` 会扫描 ID 1 到 20；也可以省略范围，默认扫描 1 到 20。扫描只发送 Ping，不会移动舵机。位置范围为 `0..4095`，速度和加速度必须先使用较小值测试。第一次只接一个舵机，并让舵机机械结构处于安全、无负载位置；确认 `ping` 成功后，再执行 `move`。

#### Wi-Fi 配置

PlatformIO 固件使用本地头文件保存 Wi-Fi 凭据：

```sh
cp src/wifi_secrets.h.example src/wifi_secrets.h
```

编辑 `src/wifi_secrets.h`：

```cpp
#pragma once

namespace wifi_secrets {
constexpr char kSsid[] = "你的WiFi名称";
constexpr char kPassword[] = "你的WiFi密码";
}  // namespace wifi_secrets
```

`src/wifi_secrets.h` 已加入 `.gitignore`，不要把真实密码提交到仓库。PlatformIO 固件最多等待 15 秒连接路由器，失败后仍会继续启动屏幕和麦克风。

小智固件使用设备界面配网：烧录后按提示进入配网模式，用手机连接临时热点，再选择家庭 Wi-Fi 并输入密码。正常运行时按 BOOT 键可切换对话状态；启动阶段按 BOOT 键可进入配网流程。

#### 常见问题

| 现象 | 检查项 |
| --- | --- |
| `pio: command not found` | 安装 PlatformIO，并把 `pio` 加入当前终端 PATH |
| `idf.py: command not found` | 在小智目录先执行 `source /Users/hemingming/esp/esp-idf/export.sh` |
| 小智字体反着 | 检查 `DISPLAY_MIRROR_X/Y` 和 `DISPLAY_SWAP_XY`，当前正确值为 `false/true/false` |
| 屏幕花屏 | 检查 SPI GPIO、CS/DC/RST、供电、共地，以及是否烧录了对应的 Robot-M 板卡配置 |
| 麦克风无响应 | 检查 INMP441 的 3.3V、GND、GPIO1、GPIO2、GPIO16 和 `L/R -> GND` |
| 功放无声音 | 检查 MAX98357A 的 5V、共地、GPIO15、`SD -> 3.3V` 和喇叭接线 |
| I2C 设备未发现 | 检查 GPIO38/39、地址、上拉、电源和 GND；不要把 SDA/SCL 接反 |
| 烧录连接失败 | 先确认串口路径；必要时按住 BOOT，短按 EN/RESET，再松开 BOOT 后重试 |



### SCS0017 舵机与 URT-2 测试

包装标注为 `SCS0017`，属于 **SCS 系列**，不是 SMS 系列：

| 参数 | 范围/值 | 说明 |
| --- | --- | --- |
| 协议 | `SCSCL` | 飞特 SCS 系列串口总线协议 |
| 默认波特率 | `1000000 8N1` | SCS 系列默认波特率 |
| 额定电压 | `6.0V` / `7.4V` | 按实际电源板设置，不能接 ESP32 3.3V |
| 位置 | `0..1023` | `512` 约为中位 |
| 速度 | `0..1023` | 数值越大通常越快，先用小值测试 |
| 加速度 | `0..255` | 当前 SCSCL `WritePosEx` 实现不使用该值，保留为接口参数 |
| 默认 ID | 通常为 `1` | 多个舵机接入前必须逐个设置唯一 ID |

当前 Arduino 驱动使用 `SCSCL` 协议类，位置命令示例：

```text
servo move 1 512 100 2  //id 位置  速度  加速度
```

#### URT-2 接线

URT-2 顶部五针从左到右为：

```text
DTR | GND | >5V | RXD | TXD
```

ESP32-S3 连接：

```text
URT-2 DTR  -> 不接
URT-2 GND  -> ESP32 GND
URT-2 >5V  -> 稳定 5V 逻辑电源
URT-2 RXD  -> ESP32 GPIO19 TX
URT-2 TXD  -> ESP32 GPIO20 RX
```

URT-2 电平选择拨到 `3V3`。舵机接 URT-2 白色 `G/V1/S` 的 SCS 接口，不要接蓝色 `RS485-Bus` 接口。舵机动力电源使用 `VADJ`，不能从 ESP32 的 5V 或 3.3V 取电；ESP32、URT-2 和舵机电源必须共地。

#### 测试命令

先编译并烧录 Arduino 固件：

```sh
export PATH="$HOME/Library/Python/3.14/bin:$PATH"
cd /Users/hemingming/worker/Robot-M
pio run
pio run --target upload
```

打开串口监视器：

```sh
pio device monitor --port /dev/cu.usbmodem5B900929761 --baud 115200
```

以下命令必须输入到**串口监视器窗口**，不能在普通 Bash 终端执行：

```text
servo ping 1
servo scan 1 20
servo setid 1 2
servo move 1 512 100 2
servo stop
```

命令说明：

| 命令 | 作用 |
| --- | --- |
| `servo ping <id>` | 检查指定 ID 的舵机是否响应 |
| `servo scan [first] [last]` | 扫描舵机 ID，默认范围为 1 到 20，只发送 Ping 不移动 |
| `servo setid <old> <new>` | 修改舵机 ID；写入前解锁 EEPROM，写入后加锁 |
| `servo move <id> <position> <speed> <acc>` | 控制单个舵机移动；SCS0017 位置范围为 0 到 1023 |
| `servo stop` | 广播关闭扭力，停止保持力矩 |

#### ID 分配流程

每次只接一个舵机。默认舵机通常是 ID `1`：

```text
第1个舵机：保留 ID 1
第2个舵机：servo setid 1 2
第3个舵机：servo setid 1 3
第4个舵机：servo setid 1 4
...
第12个舵机：servo setid 1 12
```

每次 `setid` 成功后给舵机断电，再换下一个未配置舵机。全部完成后接入总线并执行：

```text
servo scan 1 20
```

建议的功能映射：

```text
ID 1..6   两条腿，每条 3 个舵机
ID 7..10  两条胳膊，每条 2 个舵机
ID 11..12 屏幕的 2 个舵机
```

首次测试只接一个舵机，并确认 `servo ping` 成功后再执行 `servo move`。舵机周围应无负载、无卡死风险；发现异常立即输入 `servo stop` 或断开舵机动力电源。

如果 USB 重插后串口名称变化，先执行：

```sh
pio device list
```

再把 `--port` 后面的路径替换为当前实际端口。
```rust

```

    IIC HUB模块分线器I2C集线器
    ESP32 GPIO38 -> HUB SDA  灰
    ESP32 GPIO39 -> HUB SCL  白
    ESP32 3.3V   -> HUB VCC  红
    ESP32 GND    -> HUB GND  黑


#### 当前显示屏接线

照片中屏幕排针从左到右是：`SD_CS`、`CTP_INT`、`CTP_SDA`、`CTP_RST`、`CTP_SCL`、`SDO`、`LED`、`SCK`、`SDI`、`LCD_RS`、`LCD_RST`、`LCD_CS`、`GND`、`VCC`。

| 屏幕引脚 | ESP32-S3 |
| --- | --- |
| SDO / MISO | GPIO13 |
| SCK | GPIO12 |
| SDI / MOSI | GPIO11 |
| LCD_RS / DC | GPIO9 |
| LCD_RST | GPIO8 |
| LCD_CS | GPIO10 |
| CTP_SCL | GPIO39 |
| CTP_SDA | GPIO38 |
| CTP_INT | GPIO7 |
| CTP_RST | GPIO6 |

屏幕规格为 `5.0V/3.3V`，推荐 `5V` 供电，因此 `VCC` 接稳定的 `5V`，`GND` 与 ESP32 共地。背光电流约 `103mA`，`LED` 按屏幕模块的背光输入要求接线，不要让 ESP32 GPIO 直接承担背光电流；若 `LED` 不是板载限流输入，应通过限流或背光驱动供电。`SD_CS` 暂时不接。

#### 音频接线

##### INMP441 麦克风

| INMP441 引脚 | ESP32-S3 |
| --- | --- |
| VDD | 3.3V |
| GND | GND |
| SCK | GPIO1 |
| WS | GPIO2 |
| SD | GPIO16 |
| L/R | GND |

##### MAX98357A 音频功放

| MAX98357A 引脚 | ESP32-S3 / 电源 |
| --- | --- |
| VIN | 稳定 5V |
| GND | GND |
| BCLK | GPIO17 |
| LRC | GPIO18 |
| DIN | GPIO15 |
| SD | 3.3V |
| GAIN | 暂时悬空 |
| 喇叭 `+` | 喇叭正极 |
| 喇叭 `-` | 喇叭负极 |

注意：INMP441 使用 3.3V，MAX98357A 的 VIN 使用 5V；两个模块必须与 ESP32 共地。MAX98357A 的 `SD` 是启停控制脚，接 3.3V 使功放保持开启；`DIN` 才是音频数据输入。喇叭直接接功放的 `+` 和 `-`，不能把任一喇叭端接 ESP32 GND。`GAIN` 建议接 GND。小智固件中 GPIO17/18 只给功放使用，GPIO1/2 只给麦克风使用，GPIO16 只接麦克风数据，GPIO15 只接功放数据。

#### 舵机供电

12 个总线舵机使用电源板右侧的 `VADJ` 可调输出，经 URT-2 和舵机集线器供电：

```text
电池 -> 电源板输入
电源板 VADJ+ -> URT-2/舵机集线器电源+
电源板 GND   -> URT-2/舵机集线器 GND
ESP32 GND     -> URT-2 信号地
```

接线前断开舵机负载，用万用表测量并调节 `VADJ` 到舵机铭牌或数据手册规定的额定电压；不能按电池电压或电位器位置直接接入。VADJ 的最大输出电流、散热能力和瞬态电流必须满足 12 个舵机同时启动/堵转的需求；如果电源板规格不足，应改用独立的大电流舵机电源，并与 ESP32 共地。舵机电源不要从 ESP32 的 5V 或 3.3V 引脚取电，建议在 VADJ 输出增加保险丝和总线端电容。

#### TCRT5000 红外模块

当前使用两个带 LM393 比较器的 TCRT5000 模块，先使用数字输出 `DO`：

| 模块 | VCC | GND | DO | AO |
| --- | --- | --- | --- | --- |
| 左侧 TCRT5000 | 3.3V | GND | GPIO14 | 暂不接 |
| 右侧 TCRT5000 | 3.3V | GND | GPIO21 | 暂不接 |

两个模块可以共用 3.3V 和 GND，但 `DO` 必须分别接不同 GPIO。先调节模块上的电位器设置检测灵敏度；`DO` 的高低电平逻辑可能因模块和电位器方向不同而相反，需要结合实际测试确认。`AO` 是模拟输出，暂时不接。

#### Wi-Fi 连接

ESP32-S3 使用 Wi-Fi STA 模式连接路由器。先复制 `src/wifi_secrets.h.example` 为 `src/wifi_secrets.h`，再填写本地 Wi-Fi 名称和密码：

```cpp
#pragma once

namespace wifi_secrets {
constexpr char kSsid[] = "你的WiFi名称";
constexpr char kPassword[] = "你的WiFi密码";
}  // namespace wifi_secrets
```

`src/wifi_secrets.h` 已加入 `.gitignore`，不会提交凭据。固件启动时最多等待 15 秒；连接成功会在 115200 串口输出 IP 地址，连接失败也不会阻塞屏幕和麦克风启动。上传命令：

```sh
pio run --target upload
```

#### 工程分层

当前 PlatformIO 工程按以下职责组织，`src/main.cpp` 只负责 Arduino 生命周期和模块调度：

```text
src/
├── config/       硬件引脚和系统开关
├── hal/          I2C 等底层总线初始化
├── drivers/      ST7796S 显示、I2S 麦克风/音频驱动
├── manager/      RobotRuntime 安全状态机和设备业务管理
├── network/      Wi-Fi 连接与后续小智通信入口
├── main.cpp      setup/loop 主入口
└── wifi_secrets.h 本地 Wi-Fi 凭据（已被 .gitignore 忽略）
```

后续接入小智时，优先在 `network/` 增加通信服务，在 `manager/` 增加语音会话状态，不把云端协议和 API 凭据放进硬件驱动层。

#### 舵机与四肢动作分层

舵机硬件和四肢动作单独分层，避免步态逻辑直接操作串口或舵机协议：

```text
src/
├── actuators/
│   └── servo_bus.h/.cpp       URT-2、舵机总线、目标位置、急停
└── motion/
    └── limb_controller.h/.cpp 四肢、站立、步态和动作编排
```

当前接口已经建立，但还没有自动驱动舵机。接入前必须确认 URT-2 的 TX/RX GPIO、波特率、舵机 ID、零位和 VADJ 电压；在这些参数确认前，固件不会发送动作指令。低电压和姿态异常仍由 `manager/robot_runtime` 作为安全边界处理。

#### 小智独立工程

小智使用官方 `xiaozhi-esp32` ESP-IDF 工程，与当前 PlatformIO/Arduino 硬件测试工程分开维护：

```text
Robot-M/
├── src/                      当前硬件测试和 RobotRuntime
└── xiaozhi-esp32/            小智 ESP-IDF 工程
    └── main/boards/robot-m/  Robot-M 自定义板
```

小智板级配置已创建：

- `main/boards/robot-m/config.h`
- `main/boards/robot-m/config.json`
- `main/boards/robot-m/robot_m.cc`
- `main/boards/robot-m/README.md`

小智使用的硬件映射：

```text
INMP441:   SCK/WS/SD = GPIO1/GPIO2/GPIO16
MAX98357A: BCLK/LRC/DIN = GPIO17/GPIO18/GPIO15
ST7796S:   SCK/MOSI/MISO/CS/DC/RST = GPIO12/11/13/10/9/8
```

小智工程要求 ESP-IDF 6.0.1 以上，推荐 6.1。配置好 ESP-IDF 后，在 `xiaozhi-esp32` 根目录执行：

```sh
python3 scripts/build.py robot-m --name robot-m
```

小智固件的完整编译和烧录命令见本文档前面的“快速开始”章节；不要把真实 Wi-Fi 密码或串口路径提交到仓库。

