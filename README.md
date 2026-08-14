# LBS-NEW-AI-SENSORD

统一固件仓库：小型传感器/执行器外设固件，通过 UART 与主机通信。**6 个产品、2 个芯片平台，每个产品一个独立的单 target Keil 工程。**

## 产品

| 工程文件（Project/） | 芯片 | 产品宏 | ObjectID | 产品 |
|---|---|---|---|---|
| `HK32_BIG_MOTOR.uvprojx` | HK32F030MF4P6 | `BIG_MOTOR=1` | 0xA1 | 大电机（TIM 编码器） |
| `HK32_SMALL_MOTOR.uvprojx` | HK32F030MF4P6 | `SMALL_MOTOR=1` | 0xA6 | 中电机（TIM 编码器） |
| `HK32_COLOR.uvprojx` | HK32F030MF4P6 | `COLOR=1` | 0xA2 | 颜色传感器 LTR-381RGB |
| `STM32_GRAY_V1.uvprojx` | STM32G030F6P6 | `GRAY_V1=1` | 0xA9 | 灰度传感器 V1（4 路 ADC） |
| `STM32_GRAY_V2.uvprojx` | STM32G030K6T6 | `GRAY_V2=1` | 0xB0 | 灰度传感器 V2（7 路 ADC） |
| `STM32_NFC.uvprojx` | STM32G030F6P6 | `NFC_G030F6=1` | 0xB2 | 射频读卡 RC522（SPI） |

产品宏由 Keil 工程的 C/C++ Define 注入，`Project/senords.h` 强制同一时间只有一个为 1（否则 `#error`）。`USER_SourceID = 0x97`。

## 编译与烧录

- 用 Keil µVision 打开对应产品的 `.uvprojx`，`Project → Rebuild all target files`（F7）。
- 每个工程均为单 target、自带产品宏；编译后 after-build 自动执行 fromelf 生成 `.bin`。
- 产物目录：`Project/Objects/<产品>/`（`.axf` + `.bin`）。
- 烧录：HK32 用 J-Link（Flash 算法 `HK32F030MXX_16.FLM`），STM32G030 用 ST-Link。
- APP 镜像（`.bin`）配合 bootloader 使用：
  - HK32：APP 基址 `0x08002000`，版本槽 `0x08001900`
  - G0（GRAY_V2）：APP 基址 `0x08003800`，版本槽 `0x08007900`（Flash）

## 为什么是 6 个独立工程？

Keil µVision（V6.24，官方工程验证）对**任何多 target 工程**都会把所有 target 的文件合并进当前激活的 target（GUI Rebuild 与 CLI `-b` 均如此）。多 target `.uvprojx` 在 UV4 中不可用，因此本仓库按 `E:\LBS-NEW-AI-SENSORD-BOOT\Boot_All` 的方式拆分为 6 个单 target 工程。**请勿再创建多 target 工程。**

## 目录结构

```
Project/   6 个单 target Keil 工程；senords.h/c（产品宏、ObjectID、协议包结构）
Source/    HK32F030M 平台（vendor std-periph lib；Libraries/ 为上游代码，勿改）
G0/        STM32G030 平台（STM32 HAL；HAL/ 为上游代码，勿改）
  App/gray_v1, App/gray_v2, App/nfc   各产品独立源码（按 target 文件组隔离）
tools/     gen_single_uvprojx.py（重生成工程）、syntax_check.py（gcc 语法体检）
Doc/       Readme.txt
```

两平台不共享编译单元：HK32 用 std-periph lib，STM32G030 用 HAL。

## 通信协议（USART1，115200 8N1）

帧格式：`[0x5A][ObjectID][SourceID][len][typeIndex][payload...][crc&0xFF][0xA5]`

- `0x09 "Please Link"` 握手 → 回复 `"Play Aplication"`，握手完成后才上传数据
- `0xED` 数据上传（电机 PWM 控制）；`0xDD` 编码器清零；`0xEE` 重启
- 完整命令集与各产品外设映射见 [CLAUDE.md](CLAUDE.md)

## 工程重生成与体检

改动文件结构后重生成工程（模板为已生成的 6 个工程自身，只重写 TargetName/Output/Define/IncludePath/Groups/fromelf，编译器与调试配置保持不变）：

```
python tools/gen_single_uvprojx.py
```

语法体检（gcc + `tools/arm_ext.h`，无需 Keil）：

```
python tools/syntax_check.py
```
