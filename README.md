# LBS-NEW-AI-SENSORD

统一固件仓库：小型传感器/执行器外设固件，通过 UART 与主机通信。**8 个产品、3 个芯片平台，每个产品一个独立的单 target Keil 工程。**

## 产品

| 工程文件（Project/） | 芯片 | 产品宏 | ObjectID | 产品 |
| --- | --- | --- | --- | --- |
| `HK32_BIG_MOTOR.uvprojx` | HK32F030MF4P6 | `BIG_MOTOR=1` | 0xA1 | 大电机（TIM 编码器） |
| `HK32_SMALL_MOTOR.uvprojx` | HK32F030MF4P6 | `SMALL_MOTOR=1` | 0xA6 | 中电机（TIM 编码器） |
| `HK32_COLOR.uvprojx` | HK32F030MF4P6 | `COLOR=1` | 0xA2 | 颜色传感器 LTR-381RGB |
| `HK32_ELECTROMAGNETIC_SENSOR.uvprojx` | HK32F030MF4P6 | `ELECTROMAGNETIC_SENSOR=1` | 0xE0 | 电磁传感器（TIM2 PWM 吸合） |
| `STM32_GRAY_V1.uvprojx` | STM32G030F6P6 | `GRAY_V1=1` | 0xA9 | 灰度传感器 V1（4 路 ADC） |
| `STM32_GRAY_V2.uvprojx` | STM32G030K6T6 | `GRAY_V2=1` | 0xB0 | 灰度传感器 V2（7 路 ADC） |
| `STM32_NFC.uvprojx` | STM32G030F6P6 | `NFC_G030F6=1` | 0xB2 | 射频读卡 RC522（SPI） |
| `PY32_IR_REMOTE.uvprojx` | PY32F002Bx5 | `IR_REMOTE=1` | 0xA3 | 红外发射/接收（共板一份固件） |

产品宏由 Keil 工程的 C/C++ Define 注入，`Project/senords.h` 强制同一时间只有一个为 1（否则 `#error`）。`USER_SourceID = 0x97`。

## 编译与烧录

- 用 Keil µVision 打开对应产品的 `.uvprojx`，`Project → Rebuild all target files`（F7）。
- 每个工程均为单 target、自带产品宏；编译后 after-build 自动执行 fromelf 生成 `.bin`。
- 产物目录：`Project/Objects/<产品>/`（`.axf` + `.bin`）。
- 烧录：HK32 用 J-Link（Flash 算法 `HK32F030MXX_16.FLM`），STM32G030 用 ST-Link，PY32F002B 用 UL2CM3（Flash 算法 `PY32F002Bxx_24.FLM`）。
- APP 镜像（`.bin`）配合 bootloader 使用：
  - HK32：APP 基址 `0x08002000`，版本槽 `0x08001900`
  - G0（GRAY_V2）：APP 基址 `0x08003800`，版本槽 `0x08007900`（Flash）
  - PY32（IR_REMOTE）：**无 bootloader**，直接从 `0x08000000` 运行（24 KB Flash / 3 KB RAM）

## 为什么是 8 个独立工程？

Keil µVision（V6.24，官方工程验证）对**任何多 target 工程**都会把所有 target 的文件合并进当前激活的 target（GUI Rebuild 与 CLI `-b` 均如此）。多 target `.uvprojx` 在 UV4 中不可用，因此本仓库按 `E:\LBS-NEW-AI-SENSORD-BOOT\Boot_All` 的方式拆分为 8 个单 target 工程。**请勿再创建多 target 工程。**

## 目录结构

```
Project/   8 个单 target Keil 工程（含 PY32_IR_REMOTE.uvprojx）；senords.h/c（产品宏、ObjectID、协议包结构）
Source/    HK32F030M 平台（vendor std-periph lib；Libraries/ 为上游代码，勿改）
G0/        STM32G030 平台（STM32 HAL；HAL/ 为上游代码，勿改）
  App/gray_v1, App/gray_v2, App/nfc   各产品独立源码（按 target 文件组隔离）
PY32/      PY32F002B 平台（Puya HAL；HAL/ 为上游代码，勿改）
  App/ir_remote   红外发射/接收产品源码（一份固件，PB2 决定角色）
tools/     gen_single_uvprojx.py（重生成工程）、syntax_check.py（gcc 语法体检）
Doc/       Readme.txt
```

三个平台不共享编译单元：HK32 用 std-periph lib，STM32G030 用 HAL，PY32F002B 用 Puya HAL。

## 通信协议（USART1，115200 8N1）

帧格式：`[0x5A][ObjectID][SourceID][len][typeIndex][payload...][crc&0xFF][0xA5]`

- `0x09 "Please Link"` 握手 → 回复 `"Play Aplication"`，握手完成后才上传数据
- `0xED` 数据上传（电机 PWM 控制）；`0xDD` 编码器清零；`0xEE` 重启
- `0xD1` 设置 LED 颜色（GRAY_V2 校准灯 / IR_REMOTE 切换接收端颜色，payload 1 字节）
- `0xD1` / `0xD2`（HK32 电磁传感器）：吸合 / 断开，payload 必须为空；当前状态用 `0xED` 上传 1 字节（0=断开 1=吸合）
- 完整命令集与各产品外设映射见 [CLAUDE.md](CLAUDE.md)

## 电磁传感器（ELECTROMAGNETIC_SENSOR，ObjectID 0xE0）

复用中电机的 PWM 控制引脚驱动吸合线圈，**不启用编码器**（PD1/PD2 不配置）。

| 引脚 | 功能 |
| --- | --- |
| PD3 | TIM2_CH1 吸合输出（吸合时 100% 占空比，否则 0） |
| PD4 | TIM2_CH2 备用输出，恒为 0 |

- 上电、握手（`0x09 "Please Link"`）与复位后均为**断开**（CH1/CH2 = 0）。
- `0xD1`：吸合（CH1 = 100% 占空比，状态 1）；`0xD2`：断开（两路 = 0，状态 0）。两条命令 **payload 必须为空**，带载荷或截断的帧不改变输出。
- **无失联超时**：`0xD1` 后持续保持吸合，直到 `0xD2`、重新握手或复位。
- `0xED` 只上传已下发的命令状态（1 字节），不代表物理吸合反馈。

完整协议（帧格式、设备校验范围、示例帧、常见坑、主机接入清单）见 [Doc/ELECTROMAGNETIC_SENSOR_protocol.md](Doc/ELECTROMAGNETIC_SENSOR_protocol.md)。

## 红外发射/接收（IR_REMOTE，ObjectID 0xA3）

收发共板（`PY30F002BF15`）：**一份固件**，上电读 PB2 决定角色（高 = 发射端，低 = 接收端）。

| 引脚 | 功能 |
| --- | --- |
| PA0 | 红外 LED 驱动（MCU 产生 38K 载波），低电平发射 |
| PA1 | 接 38K 解调接收头，低电平有载波 |
| PA3 / PA4 | USART1 TX / RX（仅发射端接主机） |
| PA5 / PA6 / PA7 | 红 / 绿 / 蓝，高电平亮 |
| PB0 | 状态指示 LED，低电平亮（接收端低电量闪烁） |
| PB1 | 电池 ADC（仅接收端，1/2 分压，3×7 号电池） |
| PB2 | 角色选择，内部上拉输入 |

- **发射端**：`0x09` 握手后每 10 ms 上传 `{state, bat}`（index `0xED`）；收到 `0xD1 <state>`（0=灭 1=红 2=绿 3=蓝）后点亮本地 RGB 并向接收端连发 3 帧红外，之后每 1 s 刷新 1 帧。
- **接收端**：不接主机、不初始化串口；红外解码颜色并驱动 RGB；每秒采样电池，**只在本地提示**（≤10% 时 PB0 闪烁）。发射端板无红外接收头，故无反向链路，主机侧 `bat` 恒为 `0xFF`。
- **红外链路**：NEC 风格包络（9 ms + 4.5 ms 引导码，560 µs 位宽；38 kHz 载波由 MCU 产生）。地址字节区分方向：`0x5A` 发射→接收（颜色），`0xA5` 接收→发射（电量）。
- 独立运行：本产品**无 bootloader、无 VTOR 重定位**，直接从 `0x08000000` 运行。

## 工程重生成与体检

改动文件结构后重生成工程（模板为已生成的 8 个工程自身，只重写 TargetName/Output/Define/IncludePath/Groups/fromelf，编译器与调试配置保持不变）：

```
python tools/gen_single_uvprojx.py
```

语法体检（gcc + `tools/arm_ext.h`，无需 Keil）：

```
python tools/syntax_check.py
```
