# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Unified firmware family for small sensor/actuator peripherals that talk to a host over UART. **Eight independent single-target Keil projects** (one per product) across three chip platforms:

> **Why single-target:** UV4 (GUI Rebuild and CLI `-b`, verified on official Keil projects) merges ALL targets' files into the active target for ANY multi-target project �?a multi-target `.uvprojx` is unusable. Never create one here.

| Project file (Project/) | Chip | Product macro | ObjectID | Product |
| --- | --- | --- | --- | --- |
| `HK32_BIG_MOTOR.uvprojx` | HK32F030MF4P6 | `BIG_MOTOR=1` | 0xA1 | 大电�?(TIM encoder) |
| `HK32_SMALL_MOTOR.uvprojx` | HK32F030MF4P6 | `SMALL_MOTOR=1` | 0xA6 | 中电�?(TIM encoder) |
| `HK32_COLOR.uvprojx` | HK32F030MF4P6 | `COLOR=1` | 0xA2 | 颜色传感�?LTR-381RGB |
| `HK32_ELECTROMAGNETIC_SENSOR.uvprojx` | HK32F030MF4P6 | `ELECTROMAGNETIC_SENSOR=1` | 0xE0 | 电磁传感器（TIM2 PWM 吸合，无编码器） |
| `STM32_GRAY_V1.uvprojx` | STM32G030F6P6 | `GRAY_V1=1` | 0xA9 | 灰度传感�?V1 (4ch ADC) |
| `STM32_GRAY_V2.uvprojx` | STM32G030K6T6 | `GRAY_V2=1` | 0xB0 | 灰度传感�?V2 (7ch ADC) |
| `STM32_NFC.uvprojx` | STM32G030F6P6 | `NFC_G030F6=1` | 0xB2 | 射频读卡 RC522 (SPI) |
| `PY32_IR_REMOTE.uvprojx` | PY32F002Bx5 | `IR_REMOTE=1` | 0xA3 | 红外发射/接收（一份固件，PB2 决定角色） |

**Product macro** (`BIG_MOTOR` / `SMALL_MOTOR` / `COLOR` / `ELECTROMAGNETIC_SENSOR` / `GRAY_V1` / `GRAY_V2` / `NFC_G030F6` / `IR_REMOTE`) is injected via the Keil target's preprocessor Define. Exactly one must be `1`; `[Project/senords.h](Project/senords.h)` enforces this with `#error`. `USER_SourceID` is `0x97` for all products.

Platform code is **not interchangeable**: HK32F030M uses the vendor standard-peripheral library (`Source/Libraries`), STM32G030 uses the STM32 HAL (`G0/HAL`), PY32F002B uses the Puya HAL (`PY32/HAL`). They cannot share a compilation unit �?hence one project per chip platform.

## Build / flash / clean

Keil µVision (MDK-ARM) projects �?no Makefile or CMake.

- **Open & build:** open the product's project file in Keil µVision (e.g. [Project/HK32_BIG_MOTOR.uvprojx](Project/HK32_BIG_MOTOR.uvprojx)), then build (F7). Each file is single-target and carries its own product Define.
- **Flash / debug:** HK32 via J-Link (flash algo `HK32F030MXX_16.FLM`); STM32G030 via ST-Link, PY32F002B via UL2CM3 (`PY32F002Bxx_24.FLM`). Each project's after-build runs fromelf `--bin` �?`Project/Objects/<PRODUCT>/<PRODUCT>.bin` (APP image for the bootloader).
- **Clean intermediate files:** `keilkill.bat` from repo root (keeps `*.opt`).
- **No test framework.** Verification is on hardware over UART.
- `tools/gen_single_uvprojx.py` regenerates the 8 single-target projects (templates = the generated projects themselves; only per-product TargetName/Output/Define/IncludePath/Groups/fromelf are rewritten; per-product AC5/AC6 compiler, memory layout and debugger are kept). `tools/gen_uvprojx.py` is the legacy multi-target generator �?do not use.

## Source layout

```
Project/
  senords.h      Product macros + USER_ObjectID selection + HK32 packet structs
  senords.c      HK32 app layer: pull_data_from_queue() + uploading_data()
  <PRODUCT>.uvprojx  8 single-target Keil projects (HK32_*, STM32_*, PY32_IR_REMOTE) �?one per product
Source/          HK32F030M platform (std-periph lib)
  User/          main.c (vector-table relocation, init, IWDG), hk32f030m_it.c, define.h
  Handware/      dataAnalysisProtocol/ (shared wire protocol), queue/, USART/, TIMER/, PWM/,
                 SysTick/, ltr381xx/, IIC/   (tftprintf/ is a tiny printf)
  Libraries/     Vendor HK32F030M CMSIS + peripheral lib �?treat as upstream, don't edit.
G0/              STM32G030 platform (STM32 HAL)
  Core/          startup_stm32g030xx.s, system_stm32g0xx.c (shared)
  HAL/           STM32G0xx HAL + CMSIS libs (from FW_G0 V1.6.3)
  App/gray_v1/   GRAY_V1: main.c, adc/gpio/tim/usart, it, hal_msp, rx_data_queue
  App/gray_v2/   GRAY_V2: main.c, adc/dma/gpio/tim/usart, it, hal_msp, driver_gray,
                 agreement_comx, driver_sys, stmflash, rx_data_queue
  App/nfc/       NFC: main.c, gpio/spi/usart, it, hal_msp, rc522.c (+522.c not compiled),
                 user/data_analysis.c, user/queue.c
PY32/            PY32F002B platform (Puya HAL)
  Core/          startup_py32f002xx.s, system_py32f002b.c (shared)
  HAL/           PY32F002B HAL + CMSIS (do not edit)
  App/ir_remote/ IR_REMOTE: main, gpio/usart/tim/adc, ir_proto, driver_ir,
                 agreement_comx, rx_data_queue, hal_msp, it
Doc/             Readme.txt + host-facing protocol docs (IR_REMOTE_protocol.md, ELECTROMAGNETIC_SENSOR_protocol.md)
tools/           gen_single_uvprojx.py (regen the 8 single-target projects), gen_uvprojx.py (legacy
                 multi-target generator �?do not use), syntax_check.py, arm_ext.h
```

## Communication protocol ([Source/Handware/dataAnalysisProtocol/data_analysis.c](Source/Handware/dataAnalysisProtocol/data_analysis.c))

Shared wire format on USART1 (115200 8N1), framed by `0x5A` head / `0xA5` tail with a trailing sum-CRC (`& 0xFF`):

```
[0x5A][ObjectID][SourceID][len][typeIndex][payload...][crc][0xA5]
```

- Outbound: `SendCOMdata` (text/C-string) and `SendCOMdataByte` (binary struct). HK32 builds into a 32-byte `tx_cache` �?**payloads > ~25 bytes overflow**.
- All eight products use the same framing and command set:
  - `0x09 "Please Link"` handshake �?reply `"Play Aplication"`; **no uploads until linked**.
  - `0xED` �?HK32 motor targets: PWM duty (TIM2->CCR1/CCR2); all products: data upload index.
  - `0xDD` �?HK32 motor: encoder reset. `0xEE` �?reboot (`NVIC_SystemReset()`).
  - GRAY_V2 adds `0xD0` calibrate / `0xD1` LED RGB / `0xD2` set threshold.
  - IR_REMOTE (`0xA3`) uses `0xD1 <state>` (0=off 1=red 2=green 3=blue) to set the receiver colour and uploads `ir_packet_t{state, bat}` on `0xED`.
  - ELECTROMAGNETIC_SENSOR (`0xE0`) uses empty-payload `0xD1` = pull in / `0xD2` = release and uploads the single-byte commanded state on `0xED`.
- G0 products each carry their own copy of the protocol (GrayV1 inlines `SendCOMdata` in usart.c; GrayV2 uses agreement_comx.c `MultiUart_SendFrame`; NFC uses `user/data_analysis.c`) �?same framing, `USER_ObjectID` from senords.h.

## Architecture notes

### HK32F030M (projects `HK32_*`)

- `main()` ([Source/User/main.c](Source/User/main.c)) relocates the vector table to `FLASH_BASE | 0x2000` �?bootloader at offset `0x2000`. Device version read from `0x08001900`.
- UART RX: interrupt �?ring queue (`queue.c`, 16×32B) �?main loop `pull_data_from_queue()`. Never parse protocol in ISRs.
- **IWDG enabled** (`IWDG_Prescaler_128`, reload 49) and fed only in main loop �?a hung ISR resets the chip.
- TIM6 is the 1 ms tick (`timer.c`); upload cadence ~5 ms when linked.
- `Project/senords.c` `uploading_data()` sends packed structs via `SendCOMdataByte(..., 0xED)`:
  - Motor: `motor_packet_t {int speed,pos,angle,version}`; angle = pos/PPR*360. PPR: BIG_MOTOR 90, SMALL_MOTOR 62 (pwm.c).
  - COLOR: `color_packet_t {uint version; float lux; ushort ReadRaw,GreenRaw,BlueRaw}` from LTR-381RGB over bit-banged I2C on PC5/PC6 (`ltr381xx.c` + `bsp_i2c.c`). COLOR LED on PD2/PD3.
  - ELECTROMAGNETIC_SENSOR: `electromagnetic_packet_t {uchar state}` (0 = released, 1 = pulled in) — the commanded state only, no physical feedback. Reuses the SMALL_MOTOR PWM output pins (TIM2 CH1 `PD3`, CH2 `PD4`, `pwm_init()` only — `encorder_init()` is never called, so the TIM1 encoder on PD1/PD2 stays uninitialised and its code is dropped by the linker). Empty-payload `0xD1` = pull in (`TIM2->CCR1 = TIM2->ARR + 1` = 100 %, CH2 0), empty-payload `0xD2` = both channels 0; boot, `0x09` handshake and reset all force CH1/CH2 = 0 and state 0. Frames with a non-zero `len` or truncated frames are ignored, and there is **no link-loss timeout**.
- KT_MOTOR variant and its drivers (KTH782xx/, SPI/) were **removed** �?do not reintroduce.

### STM32G030 (projects `STM32_*`)

- HAL-based, CubeMX-style layout per product under `G0/App/`. Each product has its own `main.c`, peripherals, and IRQ handlers �?they are isolated by target file groups.
- GRAY_V1 (`G0/App/gray_v1`): ADC1 4ch (PA0–PA3) DMA circular �?50-sample average �?normalize �?text upload `"n/n/n/n/s/s/s/s/ver"`; LED1-5 PA11/PA8/PA7/PA6/PA5, LED6/7 PC14/PC15, KEY PA12 long-press calibrate (RAM only, not Flash). TIM3 1 ms drives upload. **SCB->VTOR = FLASH_BASE | 0x3800** (bootloader at 0x3800, linked 0x08003800). USART1 PB6/PB7.
- GRAY_V2 (`G0/App/gray_v2`): ADC1 7ch (PA0–PA6) DMA single-shot; median filter + ambient compensation + moving average �?0�?99; threshold configurable, calibration persisted to Flash (`stmflash`, cfg @ 0x08007800, version @ 0x08007900); **SCB->VTOR = FLASH_BASE | 0x3800** (bootloader at 0x3800, linked 0x08003800). Binary `gray_packet_t` upload. USART1 PA9/PA10.
- NFC (`G0/App/nfc`): RC522/SI522A RFID via SPI1 (PA1 SCK, PA2 MOSI, PA6 MISO, PA4 NSS, PA5 RST); card number from MIFARE sector read �?text upload `"num/100"`. USART1 PB6/PB7. `522.c` is a legacy driver, **not compiled** �?only `rc522.c`.
- G0 targets have **no IWDG** (except none configured) and no vector-table relocation except GRAY_V1/GRAY_V2's 0x3800 (NFC runs standalone from flash base).
- PY32 `PY32_IR_REMOTE` is standalone at flash base (no bootloader, no VTOR change, no IWDG).

### PY32F002B (project `PY32_IR_REMOTE`)

- One firmware for both ends of an IR link (`PY32/App/ir_remote/`): PB2 is sampled once at boot (HIGH = emitter, LOW = receiver); the role never changes at run time.
- **Emitter**: host UART on PA3/PA4 (115200 8N1, `agreement_comx.c` framing, ObjectID 0xA3). `0x09` handshake ??reply `"Play Aplication"`; `0xD1 <state>` sets the colour (0=off 1=red 2=green 3=blue) and only a **changed** state re-bursts 3 IR frames �?a host that re-sends the same state at 10 Hz is harmless and must not turn the link into a ~100 % duty stream (that overloads the receiver AGC); uploads `ir_packet_t{state, bat}` on `0xED` every 10 ms while linked (`bat` = 0..100, `0xFF` = unknown).
- **Receiver**: no USART1, no host link. Decodes the IR colour frame, drives PA5/PA6/PA7 (active high), samples PB1 (1/2 divider, 3xAAA) once a second and shows a low battery on PB0 (blinks when <=10%). The emitter board has only an IR LED and no IR receiver head, so the reverse link is compiled out (IR_REVERSE_LINK 0), bat is always 0xFF and the receiver's battery is reported locally only. The last valid red/green/blue IR frame is held for **5 s** (`IR_HOLD_MS` in `driver_ir.c`): repeats, colour changes and the emitter's 1 s refresh **burst (3 frames)** all restart the timer, and a frame that carries the forward address but was damaged in the command bytes still counts as "the emitter is sending" (so a marginal link is not dropped on one corrupted refresh); any 38 kHz carrier seen on PA1 also restarts the timer, so an overloaded demodulator (boards held at very short range) cannot make the colour drop while the emitter is still transmitting; `0xD1 00` turns the RGB off immediately; a stopped or blocked emitter is dropped ~5 s after the last frame.
- **IR link** (`ir_proto.c`): NEC-style envelope generated in software ??9 ms mark + 4.5 ms space header, 560 µs bit marks, 560/1690 µs spaces, 32 bits = address, ~address, command, ~command. The 38 kHz carrier is generated by the MCU itself: PA0 drives a bare IR LED and every mark is emitted as a 38 kHz burst (active low), because the receiver is a 38 kHz demodulator module. The address byte selects the direction: `0x5A` emitter->receiver (colour), `0xA5` receiver->emitter (battery percent, currently unused because there is no reverse link).
- Timing uses TIM14 as a 1 µs free-running counter (`tim.c`); `HAL_GetTick()` is far too coarse. `ir_send_frame()` blocks ~68 ms per frame. Emitter schedule: 3 frames on a colour change and 3 frames on the 1 s refresh, then a 350 ms receive window (when `IR_REVERSE_LINK` is on).
- Standalone: 24 KB Flash / 3 KB RAM, AC5, links at `0x08000000`, no bootloader, no VTOR relocation, no IWDG.

## Conventions specific to this codebase

- Keep new message types inside the existing `typeIndex` switch (or the per-product parse in G0 apps); don't add a parallel parser.
- GBK-encoded Chinese comments are common in `Source/` and `G0/App/*` �?preserve encoding, don't re-encode wholesale.
- HK32 `bool` is split two ways and must not be mixed: `Source/User/define.h` typedefs its own `bool`/`BOOL` as `volatile unsigned char` (legacy), but `Source/User/main.c`, `TIMER/timer.c`, and `Project/senords.c` use C99 `<stdbool.h>` for the `linkState`/`uploadState` flags. `uploadState` is **defined in `timer.c`** and `extern` in `main.c` �?both must see the same stdbool `bool` or the extern type mismatches; any TU touching these flags needs `#include "stdbool.h"`, not `define.h`'s typedef.
- `Source/Handware/tftprintf/printf.c` is a tiny ported printf for output routing �?not the toolchain printf.
- `G0/App/nfc/user/queue.c` and `Source/Handware/queue/queue.c` are near-identical ring queues (16×32B) kept per-platform; don't merge them across platforms.
- `tools/arm_ext.h` + `tools/syntax_check.py` provide a GCC-based syntax sanity check (gcc doesn't know `__weak`/`__align`, so arm_ext.h stubs them). Run `python tools/syntax_check.py` after structural edits �?it checks all 8 product source sets.
