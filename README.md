
# Robot-M 开发与接线手册

## 1. 先看这里

| 固件 | 工程目录 | 用途 | 串口命令 |
| --- | --- | --- | --- |
| 小智 ESP-IDF（当前使用） | [xiaozhi-esp32](xiaozhi-esp32/) | 大头语音、配网、屏幕、MCP、舵机查询 | 仅 `servo ping`、`servo scan` |
| Arduino / PlatformIO | 仓库根目录 | 外设单项调试、传感器、RobotRuntime | 查询、改 ID、单舵机移动和机器人动作等 |

两套固件不能同时运行。不要为了查询舵机误执行 Arduino 上传命令，覆盖正在使用的小智固件。本文所有 Bash 命令都需要在对应工程目录执行；`servo ...` 输入到串口监视器，不是在 Bash 中执行。

### 安全与 USB 选择

- 改线前断电；上电或停止扭力前支撑好机械结构。不要直接测试未经校准的步态。
- **GPIO19/20 与 ESP32-S3 原生 USB 共用**。接着 URT-2 信号线时，不要同时连接原生 USB 数据口。
- 日常日志和查询使用**板载 USB 转串口（UART0）**。原生 USB 烧录前，先断开舵机动力及 URT-2 到 GPIO19/20 的信号线。
- 舵机动力不得从 ESP32 的 5V/3.3V 引脚取电。电池标称 7.4V 不等于稳压板输出电压；2 串锂电池满电通常约 8.4V，不能据此直接给外设供电。
- URT-2 的 `>5V` 引脚方向、允许电压及逻辑/动力供电关系需按实际版本手册确认，不能仅凭丝印当作普通 5V 输入。

## 2. 小智：编译、烧录、串口

### 2.1 加载环境与查看端口

从仓库根目录打开终端。推荐 ESP-IDF 6.1，最低 6.0.1，不使用 IDF 5.x。SDK 路径按本机安装位置调整；统一使用 `python3`。

```sh
cd xiaozhi-esp32
source "$HOME/esp/esp-idf/export.sh"
idf.py --version
python3 -m serial.tools.list_ports -v
```

2026-09-20 的设备识别信息如下；端口路径会变化，以本次枚举为准，不按数字猜设备。

| 接口 | USB VID:PID | 用途 |
| --- | --- | --- |
| 板载 USB 转串口 | `1A86:55D3` | UART0 日志及本地查询；当天下载握手未成功 |
| ESP32-S3 原生 USB | `303A:1001` | 当天手动下载烧录成功；与 GPIO19/20 冲突 |

在当前终端设置实际端口，下面是占位符，必须替换：

```sh
export FLASH_PORT=/dev/cu.usbmodemXXXX
export LOG_PORT=/dev/cu.usbmodemYYYY
```

### 2.2 编译 Robot-M

工作目录：`xiaozhi-esp32/`。

```sh
python3 scripts/build.py robot-m --name robot-m
```

确认输出 `Project build complete`。修改代码或板级配置后重新编译；不要手工改生成的 `sdkconfig`、`build/` 文件。当前使用默认生成资源，不需要另外放入自定义 `assets.bin`。

### 2.3 正常自动下载

工作目录：`xiaozhi-esp32/`。先关闭占用端口的串口监视器，并按前面的安全要求隔离舵机和原生 USB 引脚。

```sh
idf.py -p "$FLASH_PORT" flash
```

正常自动下载无需按 BOOT。如果连接失败，先核对端口和接线，再使用下一节手动下载，不要反复擦除整片 Flash。

### 2.4 今天成功使用的手动烧录方式

1. 断开舵机动力与 GPIO19/20 的 URT-2 信号线，连接原生 USB。
2. 按住 BOOT，短按 EN/RESET。再次枚举端口，确认出现原生 USB 设备。
3. 在已完成编译的 `xiaozhi-esp32/` 目录运行下面命令。若保持 BOOT 按住，看到 `Connected to ESP32-S3` / `Stub flasher running` 后即可松开，不必按完整个烧录过程。

```sh
cd build
python3 -m esptool --chip esp32s3 \
  --port "$FLASH_PORT" --baud 460800 \
  --before no-reset --after no-reset \
  write-flash @flash_args
cd ..
```

- `@flash_args` 使用本次构建的文件及地址，必须在 `build/` 内执行，不手工猜偏移。
- 此命令写入 bootloader、分区表、OTA 初始选择数据、默认资源和应用；会擦除相应写入区域，**不是整片擦除**。不要把完整烧录当成必然保留所有状态的操作。
- 当前分区方案未将 NVS 列为写入区域；更换分区方案前需另行评估数据保留。
- 每个区域应显示 `Hash of data verified`。末尾 `Staying in bootloader` 是预期结果，因为指定了 `--after no-reset`。
- 松开 BOOT 后短按 EN/RESET 启动。恢复舵机接线前再次断电、拔掉原生 USB，之后改用板载转串口查看日志。

仅应用更新的可选命令（工作目录仍为 `xiaozhi-esp32/`）：

```sh
idf.py -p "$FLASH_PORT" app-flash
```

`app-flash` 可能触发增量构建；只在分区/资源不变且确认应用槽位适用时使用，不能代替初次完整烧录。

### 2.5 日志与可输入命令的监视器

工作目录：`xiaozhi-esp32/`，使用板载转串口的 `$LOG_PORT`。

```sh
idf.py -p "$LOG_PORT" monitor
```

退出通常为 `Ctrl+]`。IDF monitor 的连接过程可能复位设备，机械结构须处于安全状态。需要本地回显和输入 `servo` 命令时，也可使用 pyserial：

```sh
python3 -m serial.tools.miniterm "$LOG_PORT" 115200 \
  --raw --echo --dtr 0 --rts 0
```

miniterm 退出为 `Ctrl+]`，帮助为 `Ctrl+T` 后按 `Ctrl+H`。上述参数不主动执行下载复位序列，但打开串口仍可能因驱动/电路导致复位。先等待启动完成再输入命令。

预期启动日志：

```text
Servo bus ready: TX=GPIO19, RX=GPIO20, baud=1000000
Read-only servo console ready: UART0, 115200 baud
```

## 3. 小智：查询与动作

### 3.1 本地只读查询（输入串口监视器）

```text
servo
servo ping 1
servo ping 2
servo scan 1 2
servo scan 1 20
```

| 命令/结果 | 含义 |
| --- | --- |
| `servo` | 只返回用法提示，用于确认电脑到 ESP32 的命令入口 |
| `servo ping <id>` | 查询一个 ID，不改变位置、扭力或 EEPROM |
| `servo scan [first last]` | 不带参数默认扫描 1..20；指定范围时须同时提供两个 ID |
| `valid Ping reply` | 收到指定 ID 的有效应答，不代表无故障或已校准 |
| `no valid Ping reply` | 未接受到有效应答，不等于舵机损坏，也不能确定完全没有收到字节 |
| `Servo query complete: N responding ID(s)` | 本次查询完成；N 是有应答的 ID 数 |

小智入口只接受 ID 1..253，起点不得大于终点；超长输入、非法参数和其他命令会被拒绝。**不支持本地 `servo move`、`servo setid`、`servo stop`**。查询在独立任务执行，与 MCP 共用总线锁；启动不会自动扫描或启动步态。

### 3.2 大头语音 / MCP

当前唤醒词：**你好大头**。首次配网按屏幕提示操作；启动阶段 BOOT 可进入配网，正常运行时用于切换对话状态。

| MCP 工具 | 实际行为 |
| --- | --- |
| `self.robot.servo_ping` | 查询指定 `id`；首次启动异步任务，用相同 ID 再调用取得结果，未取完结果前不能换 ID |
| `self.robot.walk_forward` | 持续运行当前前进位置循环，直到停止 |
| `self.robot.turn_left` | 持续运行当前左转位置循环 |
| `self.robot.turn_right` | 持续运行当前右转位置循环 |
| `self.robot.stand` | 停止循环，将 ID 1..6 移到位置 512，并开启扭力 |
| `self.robot.stop` | 停止循环，广播关闭扭力；结构可能失去支撑 |

共 5 个动作控制接口，另有 1 个查询接口。前进/转向目前是简单的 467/557 位置循环，约每 350ms 切相，**不是经过机械校准或验证的稳定步态**。点头、摇头、挥手、跳舞尚未实现。MCP 返回执行成功不等于所有舵机都已到位。

## 4. 接线与供电

### 4.1 硬件清单

| 类别 | 当前器件/规划 |
| --- | --- |
| 主控 | ESP32-S3-DEV-KIT-N32R16V-M，32MB Flash、16MB PSRAM |
| 电源 | 标称 7.4V 2200mAh 锂电池；可调稳压板，型号/输出以实物为准 |
| 舵机 | URT-2、TTL 总线分线板；12 个 SC-0017-C001 / 包装 SCS0017 |
| 显示 | 4.0 英寸 SPI ST7796S 电容触摸屏 |
| 音频 | INMP441、MAX98357A、喇叭 |
| 传感器 | BMI323、VL53L1X、两个 TCRT5000、I2C HUB |
| 摄像头 | 串口 OV2640 模组列入规划，当前小智板卡未集成 |

### 4.2 URT-2 / SCS0017

| ESP32 端 | URT-2 端 | 说明 |
| --- | --- | --- |
| GPIO19，UART1 TX | RXD | ESP32 发送接 URT-2 接收 |
| GPIO20，UART1 RX | TXD | ESP32 接收接 URT-2 发送 |
| GND | GND | 与舵机电源共地 |
| 不接 | DTR | 不用于当前控制链路 |

- 舵机连接 TTL/SCS 接口（本板标识 G/V1/S），不是 RS485 接口；按实物核对引脚、极性。ESP32 侧必须是兼容的 3.3V 逻辑电平，不能将高电压接入 GPIO。
- SCSCL 协议，当前总线 1000000 8N1；位置范围 **0..1023**，512 只是数值中位，不是校准后的机械零位。SCSCL 16 位字段高字节在前。
- TX/RX/GND 不是给 URT-2 供电的线路。逻辑供电来自哪个输入、是否与舵机电源相连，取决于 URT-2 实际板型。
- 舵机动力由符合额定电压和电流要求的电源，经动力输入/集线器分配。先核对板型与供电路径，再用万用表测舵机 V+ 对 GND；不要根据电池标称或电位器位置推断。
- 12 个舵机的启动/堵转电流、稳压板散热及导线载流能力必须评估，动力侧建议保险丝；不能默认 USB 或现有小稳压板可以带全部舵机。
- 已确认 ID 分配（2026-09-20）：1..3 左腿、4..6 右腿、7..8 左臂、9..10 右臂、11 屏幕左右摇头、12 屏幕上下点头。当前动作代码只使用 1..6，手臂/头部动作待标定后实现。
- 阶段 1 电气层完成（2026-09-20）：12 个舵机逐个 `setid` 完成，全量验收 `servo scan 1 12` 为 12/12 OK，已贴标签，`servo center 1..12` 全部回中验证通过。下一步：按回中流程装配铝合金骨架（每个关节先回中→断电→装摇臂），装配完成后用 `servo sweep` 逐关节标定机械限位，再开发手臂/头部动作。

### 4.3 ST7796S 屏幕（小智与 Arduino 主要 SPI 引脚一致）

| 屏幕端 | ESP32/电源 |
| --- | --- |
| SDO/MISO | GPIO13 |
| SCK | GPIO12 |
| SDI/MOSI | GPIO11 |
| LCD_RS/DC | GPIO9 |
| LCD_RST | GPIO8 |
| LCD_CS | GPIO10 |
| CTP_SCL / CTP_SDA | GPIO39 / GPIO38 |
| CTP_INT / CTP_RST | GPIO7 / GPIO6（小智仅预留） |
| VCC / GND | 按该屏模块规格使用稳定 5V / 公共地 |
| LED | 按模块背光要求供电，不由 GPIO 承担背光电流 |
| SD_CS | 暂不接 |

当前小智显示为 320x480，`MIRROR_X=false`、`MIRROR_Y=true`、`SWAP_XY=false`。背光标注约 103mA；需确认模块是否已有驱动/限流。

### 4.4 音频（注意两套固件的麦克风时钟不同）

| 模块引脚 | 小智固件接线 | Arduino 当前配置 |
| --- | --- | --- |
| INMP441 VDD / GND / L/R | 3.3V / GND / GND | 相同 |
| INMP441 SCK / WS | GPIO1 / GPIO2 | GPIO17 / GPIO18，与功放共用时钟 |
| INMP441 SD | GPIO16 | 相同 |
| MAX98357A VIN / GND | 按模块规格使用稳定 5V / GND | 相同 |
| MAX98357A BCLK / LRC / DIN | GPIO17 / GPIO18 / GPIO15 | 相同 |
| MAX98357A SD（启停） | 3.3V，按模块规格保持开启 | 按实物核对 |
| MAX98357A GAIN | 按实际模块增益需求设置，不混用悬空/接地说明 | 按实物核对 |

喇叭只接功放 SPK+、SPK-，不能将任一喇叭输出接 ESP32 GND。更换固件前检查麦克风时钟接线，不能假定两套固件完全相同。

### 4.5 I2C、红外与预留接口

| 功能 | 引脚/配置 | 状态/注意事项 |
| --- | --- | --- |
| I2C HUB、BMI323、VL53L1X | SDA GPIO38，SCL GPIO39，400kHz | HUB/上拉使用兼容的 3.3V 电平 |
| BMI323 | 地址 0x68；CS 接 3.3V、SA0 接 GND | Arduino 有 `imu read` 调试入口 |
| VL53L1X | 默认地址 0x29 | Arduino 测距受配置开关控制 |
| 左/右 TCRT5000 DO | GPIO14 / GPIO21 | VCC 3.3V、共地，AO 暂不接 |
| 电池采样 | GPIO4 | Arduino；必须分压，电池不能直连 ADC |
| 外设供电使能 | GPIO5 | Arduino；是否接稳压板使能需核对实物 |
| 外接 ASR UART | TX GPIO41、RX GPIO40 | Arduino 预留/配置，不是小智麦克风语音通道 |

TCRT5000 数字输出可能低有效，先用不同颜色/距离物体测试并调电位器；Arduino 日志示例为 `TCRT5000: left=1 right=1`。未校准前不能把它当作可靠的悬崖保护。小智不能默认继承 Arduino 的传感器及 RobotRuntime 安全逻辑。

N32R16V 的 GPIO33..37 为 Octal 总线保留；GPIO47/48 不应未经核对就当成普通 3.3V UART 替代脚。

## 5. Arduino / PlatformIO（仅在明确切换固件时使用）

### 编译与串口

从仓库根目录执行，需要 `pio` 在 PATH 中。配置文件里旧的 `monitor_port` 可能已过期，显式传入实际端口。

```sh
pio device list
pio run
pio device monitor --port "$LOG_PORT" --baud 115200
```

只有确认要覆盖小智固件时才上传：

```sh
pio run --target upload --upload-port "$FLASH_PORT"
```

Arduino 启动时自动运行 `setup()` / `loop()`，不需要另一个启动命令。根目录当前没有独立主机测试框架；不要把 `pio test` 当作已有完整测试覆盖。

### Wi-Fi

先确认本地文件不存在，再创建，避免覆盖已有凭据：

```sh
cp -n src/wifi_secrets.h.example src/wifi_secrets.h
```

在本地头文件填写 `wifi_secrets::kSsid` / `kPassword`；该文件被忽略，不提交真实凭据。此方式只用于 Arduino，小智使用设备配网界面。

### Arduino 串口命令速查

以下不是当前小智串口入口的能力，动作命令和 EEPROM 写入必须单独确认后执行。

| 命令 | 作用 |
| --- | --- |
| `imu read` | 读取 BMI323 调试数据 |
| `servo ping <id>`、`servo scan [first] [last]` | 只读 Ping，扫描默认 1..20 |
| `servo setid <old> <new>` | 写 EEPROM 改 ID，仅连接一个舵机时使用 |
| `servo move <id> <position> <speed> <acc>` | 单舵机移动，位置 0..1023；SCSCL 当前忽略 acc |
| `servo center <id>` | 单舵机回到位置 512，用于零位确认 |
| `servo sweep <id> <from> <to>` | 10 位置步进缓慢扫动找机械限位；异响/卡住立即 `servo stop` |
| `servo stop` | 广播关闭扭力，可能失去支撑 |
| `robot stand`、`robot forward` | 中位姿态/持续动作，未完成校准不要执行 |
| `robot turn left`、`robot turn right`、`robot stop` | 转向/停止，需支撑结构 |

ID 配置流程：每次断电只接一个舵机，先 Ping 确认当前 ID，再按需 `setid`，成功后断电重启验证并贴标签，最后接入总线扫描。不要假定出厂 ID 一定是 1。禁止使用旧文档中的位置 2048 或范围 0..4095 来测试 SCS0017。

## 6. 常见问题与当天结论

| 现象 | 下一步 |
| --- | --- |
| 找不到 USB 串口 | 先枚举；检查数据线、接口及供电，不对蓝牙端口执行烧录 |
| 能看到串口但 `No serial data received` | 只说明下载握手失败；核对接口、GPIO19/20 冲突、BOOT/EN 时序，不能认定烧录成功或芯片损坏 |
| `Staying in bootloader` | `--after no-reset` 的预期结果；松开 BOOT，短按 EN/RESET |
| 串口打开后没有日志 | 检查 UART0/原生 USB 区别及 115200；短时间静默不代表舵机故障 |
| 只有启动日志，没有查询结果 | 等 `Read-only servo console ready`，再发送命令；避免启动前发送导致命令丢失 |
| 本地无应答、大头也查不到 | 核对 GPIO19 TX -> URT-2 RXD、GPIO20 RX <- URT-2 TXD，再查实际供电、共地、ID、波特率 |
| 1、2 号能 Ping，但只有一个动 | Ping 不能证明位置写入成功；字节序已修复并烧录，实际动作仍待逐个验证，不直接启动整套步态 |
| 小智拒绝 `servo move` 等 | 这是只读入口；不要混用 Arduino 命令 |
| 屏幕方向错误 | 核对第 4 节方向参数及板卡配置 |
| 麦克风无输入 | 首先核对第 4 节两套固件不同的 SCK/WS，再查电源与 SD |
| TCRT5000 状态不变 | 检查 3.3V、共地、DO 和电位器；用实物校准极性 |
| `pio` / `idf.py` 找不到 | 修正 PATH 或加载正确环境，不在不同固件工程中替换执行无关构建命令 |

## 7. 项目入口与日常检查

| 文件/目录 | 职责 |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Arduino 生命周期、诊断和串口命令 |
| [src/config/pin_def.h](src/config/pin_def.h)、[src/config/project_config.h](src/config/project_config.h) | Arduino 引脚与开关 |
| [src/manager/robot_runtime.cpp](src/manager/robot_runtime.cpp) | Arduino 传感器快照与模式安全边界 |
| [src/actuators/servo_bus.cpp](src/actuators/servo_bus.cpp)、[src/motion/limb_controller.cpp](src/motion/limb_controller.cpp) | Arduino 舵机驱动与动作 |
| [platformio.ini](platformio.ini) | Arduino 板型、Flash/PSRAM、屏幕与依赖 |
| [xiaozhi-esp32/main/boards/robot-m/config.h](xiaozhi-esp32/main/boards/robot-m/config.h) | 小智硬件映射 |
| [xiaozhi-esp32/main/boards/robot-m/config.json](xiaozhi-esp32/main/boards/robot-m/config.json) | 小智板卡、唤醒词和构建配置 |
| [xiaozhi-esp32/main/boards/robot-m/robot_m.cc](xiaozhi-esp32/main/boards/robot-m/robot_m.cc) | 小智板级初始化、MCP 和本地查询 |
| [xiaozhi-esp32/main/boards/robot-m/scs_servo_bus.cc](xiaozhi-esp32/main/boards/robot-m/scs_servo_bus.cc) | 小智 UART 舵机协议 |
| [xiaozhi-esp32/main/boards/robot-m/limb_controller.cc](xiaozhi-esp32/main/boards/robot-m/limb_controller.cc) | 小智动作循环 |
| [xiaozhi-esp32/main/boards/robot-m/README.md](xiaozhi-esp32/main/boards/robot-m/README.md) | 小智板卡及离线语音补充说明 |

普通终端中的只读 Git 检查（仓库根目录）：

```sh
git status --short
git diff --stat
git diff --check
```

小智构建脚本的主机测试（`xiaozhi-esp32/` 内，不是舵机实测）：

```sh
python3 -m unittest discover -s scripts/tests -v
```

Git 推送遇到 `Permission denied (publickey)` 时，可先执行 `ssh-add -l` 查看代理身份，再用正确的 GitHub SSH 身份测试；不要因为认证失败修改代码、重复提交或公开私钥。硬件调试无需反复执行 Git 推送。

