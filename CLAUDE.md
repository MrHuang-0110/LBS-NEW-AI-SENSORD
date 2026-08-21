# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Unified firmware family for small sensor/actuator peripherals that talk to a host over UART. **Six independent single-target Keil projects** (one per product) across two chip platforms:

> **Why single-target:** UV4 (GUI Rebuild and CLI `-b`, verified on official Keil projects) merges ALL targets' files into the active target for ANY multi-target project ‚Ä?a multi-target `.uvprojx` is unusable. Never create one here.

| Project file (Project/) | Chip | Product macro | ObjectID | Product |
|---|---|---|---|---|
| `HK32_BIG_MOTOR.uvprojx` | HK32F030MF4P6 | `BIG_MOTOR=1` | 0xA1 | Â§ßÁîµÊú?(TIM encoder) |
| `HK32_SMALL_MOTOR.uvprojx` | HK32F030MF4P6 | `SMALL_MOTOR=1` | 0xA6 | ‰∏≠ÁîµÊú?(TIM encoder) |
| `HK32_COLOR.uvprojx` | HK32F030MF4P6 | `COLOR=1` | 0xA2 | È¢úËâ≤‰º†ÊÑüÂô?LTR-381RGB |
| `STM32_GRAY_V1.uvprojx` | STM32G030F6P6 | `GRAY_V1=1` | 0xA9 | ÁÅ∞Â∫¶‰º†ÊÑüÂô?V1 (4ch ADC) |
| `STM32_GRAY_V2.uvprojx` | STM32G030K6T6 | `GRAY_V2=1` | 0xB0 | ÁÅ∞Â∫¶‰º†ÊÑüÂô?V2 (7ch ADC) |
| `STM32_NFC.uvprojx` | STM32G030F6P6 | `NFC_G030F6=1` | 0xB2 | Â∞ÑÈ¢ëËØªÂç° RC522 (SPI) |

**Product macro** (`BIG_MOTOR` / `SMALL_MOTOR` / `COLOR` / `GRAY_V1` / `GRAY_V2` / `NFC_G030F6`) is injected via the Keil target's preprocessor Define. Exactly one must be `1`; `[Project/senords.h](Project/senords.h)` enforces this with `#error`. `USER_SourceID` is `0x97` for all products.

Platform code is **not interchangeable**: HK32F030M uses the vendor standard-peripheral library (`Source/Libraries`), STM32G030 uses the STM32 HAL (`G0/HAL`). They cannot share a compilation unit ‚Ä?hence one project per chip platform.

## Build / flash / clean

Keil ¬µVision (MDK-ARM) projects ‚Ä?no Makefile or CMake.

- **Open & build:** open the product's project file in Keil ¬µVision (e.g. [Project/HK32_BIG_MOTOR.uvprojx](Project/HK32_BIG_MOTOR.uvprojx)), then build (F7). Each file is single-target and carries its own product Define.
- **Flash / debug:** HK32 via J-Link (flash algo `HK32F030MXX_16.FLM`); STM32G030 via ST-Link. Each project's after-build runs fromelf `--bin` ‚Ü?`Project/Objects/<PRODUCT>/<PRODUCT>.bin` (APP image for the bootloader).
- **Clean intermediate files:** `keilkill.bat` from repo root (keeps `*.opt`).
- **No test framework.** Verification is on hardware over UART.
- `tools/gen_single_uvprojx.py` regenerates the 6 single-target projects (templates = the generated projects themselves; only per-product TargetName/Output/Define/IncludePath/Groups/fromelf are rewritten; per-product AC5/AC6 compiler, memory layout and debugger are kept). `tools/gen_uvprojx.py` is the legacy multi-target generator ‚Ä?do not use.

## Source layout

```
Project/
  senords.h      Product macros + USER_ObjectID selection + HK32 packet structs
  senords.c      HK32 app layer: pull_data_from_queue() + uploading_data()
  <PRODUCT>.uvprojx  6 single-target Keil projects (HK32_*, STM32_*) ‚Ä?one per product
Source/          HK32F030M platform (std-periph lib)
  User/          main.c (vector-table relocation, init, IWDG), hk32f030m_it.c, define.h
  Handware/      dataAnalysisProtocol/ (shared wire protocol), queue/, USART/, TIMER/, PWM/,
                 SysTick/, ltr381xx/, IIC/   (tftprintf/ is a tiny printf)
  Libraries/     Vendor HK32F030M CMSIS + peripheral lib ‚Ä?treat as upstream, don't edit.
G0/              STM32G030 platform (STM32 HAL)
  Core/          startup_stm32g030xx.s, system_stm32g0xx.c (shared)
  HAL/           STM32G0xx HAL + CMSIS libs (from FW_G0 V1.6.3)
  App/gray_v1/   GRAY_V1: main.c, adc/gpio/tim/usart, it, hal_msp, rx_data_queue
  App/gray_v2/   GRAY_V2: main.c, adc/dma/gpio/tim/usart, it, hal_msp, driver_gray,
                 agreement_comx, driver_sys, stmflash, rx_data_queue
  App/nfc/       NFC: main.c, gpio/spi/usart, it, hal_msp, rc522.c (+522.c not compiled),
                 user/data_analysis.c, user/queue.c
Doc/             empty Readme.txt
tools/           gen_single_uvprojx.py (regen the 6 single-target projects), gen_uvprojx.py (legacy
                 multi-target generator ‚Ä?do not use), syntax_check.py, arm_ext.h
```

## Communication protocol ([Source/Handware/dataAnalysisProtocol/data_analysis.c](Source/Handware/dataAnalysisProtocol/data_analysis.c))

Shared wire format on USART1 (115200 8N1), framed by `0x5A` head / `0xA5` tail with a trailing sum-CRC (`& 0xFF`):

```
[0x5A][ObjectID][SourceID][len][typeIndex][payload...][crc][0xA5]
```

- Outbound: `SendCOMdata` (text/C-string) and `SendCOMdataByte` (binary struct). HK32 builds into a 32-byte `tx_cache` ‚Ä?**payloads > ~25 bytes overflow**.
- All six products use the same framing and command set:
  - `0x09 "Please Link"` handshake ‚Ü?reply `"Play Aplication"`; **no uploads until linked**.
  - `0xED` ‚Ä?HK32 motor targets: PWM duty (TIM2->CCR1/CCR2); all products: data upload index.
  - `0xDD` ‚Ä?HK32 motor: encoder reset. `0xEE` ‚Ä?reboot (`NVIC_SystemReset()`).
  - GRAY_V2 adds `0xD0` calibrate / `0xD1` LED RGB / `0xD2` set threshold.
- G0 products each carry their own copy of the protocol (GrayV1 inlines `SendCOMdata` in usart.c; GrayV2 uses agreement_comx.c `MultiUart_SendFrame`; NFC uses `user/data_analysis.c`) ‚Ä?same framing, `USER_ObjectID` from senords.h.

## Architecture notes

### HK32F030M (projects `HK32_*`)
- `main()` ([Source/User/main.c](Source/User/main.c)) relocates the vector table to `FLASH_BASE | 0x2000` ‚Ä?bootloader at offset `0x2000`. Device version read from `0x08001900`.
- UART RX: interrupt ‚Ü?ring queue (`queue.c`, 16√ó32B) ‚Ü?main loop `pull_data_from_queue()`. Never parse protocol in ISRs.
- **IWDG enabled** (`IWDG_Prescaler_128`, reload 49) and fed only in main loop ‚Ä?a hung ISR resets the chip.
- TIM6 is the 1 ms tick (`timer.c`); upload cadence ~5 ms when linked.
- `Project/senords.c` `uploading_data()` sends packed structs via `SendCOMdataByte(..., 0xED)`:
  - Motor: `motor_packet_t {int speed,pos,angle,version}`; angle = pos/PPR*360. PPR: BIG_MOTOR 90, SMALL_MOTOR 62 (pwm.c).
  - COLOR: `color_packet_t {uint version; float lux; ushort ReadRaw,GreenRaw,BlueRaw}` from LTR-381RGB over bit-banged I2C on PC5/PC6 (`ltr381xx.c` + `bsp_i2c.c`). COLOR LED on PD2/PD3.
- KT_MOTOR variant and its drivers (KTH782xx/, SPI/) were **removed** ‚Ä?do not reintroduce.

### STM32G030 (projects `STM32_*`)
- HAL-based, CubeMX-style layout per product under `G0/App/`. Each product has its own `main.c`, peripherals, and IRQ handlers ‚Ä?they are isolated by target file groups.
- GRAY_V1 (`G0/App/gray_v1`): ADC1 4ch (PA0‚ÄìPA3) DMA circular ‚Ü?50-sample average ‚Ü?normalize ‚Ü?text upload `"n/n/n/n/s/s/s/s/ver"`; LED1-5 PA11/PA8/PA7/PA6/PA5, LED6/7 PC14/PC15, KEY PA12 long-press calibrate (RAM only, not Flash). TIM3 1 ms drives upload. **SCB->VTOR = FLASH_BASE | 0x3800** (bootloader at 0x3800, linked 0x08003800). USART1 PB6/PB7.
- GRAY_V2 (`G0/App/gray_v2`): ADC1 7ch (PA0‚ÄìPA6) DMA single-shot; median filter + ambient compensation + moving average ‚Ü?0‚Ä?99; threshold configurable, calibration persisted to Flash (`stmflash`, cfg @ 0x08007800, version @ 0x08007900); **SCB->VTOR = FLASH_BASE | 0x3800** (bootloader at 0x3800, linked 0x08003800). Binary `gray_packet_t` upload. USART1 PA9/PA10.
- NFC (`G0/App/nfc`): RC522/SI522A RFID via SPI1 (PA1 SCK, PA2 MOSI, PA6 MISO, PA4 NSS, PA5 RST); card number from MIFARE sector read ‚Ü?text upload `"num/100"`. USART1 PB6/PB7. `522.c` is a legacy driver, **not compiled** ‚Ä?only `rc522.c`.
- G0 targets have **no IWDG** (except none configured) and no vector-table relocation except GRAY_V1/GRAY_V2's 0x3800 (NFC runs standalone from flash base).

## Conventions specific to this codebase
- Keep new message types inside the existing `typeIndex` switch (or the per-product parse in G0 apps); don't add a parallel parser.
- GBK-encoded Chinese comments are common in `Source/` and `G0/App/*` ‚Ä?preserve encoding, don't re-encode wholesale.
- HK32 `bool` is split two ways and must not be mixed: `Source/User/define.h` typedefs its own `bool`/`BOOL` as `volatile unsigned char` (legacy), but `Source/User/main.c`, `TIMER/timer.c`, and `Project/senords.c` use C99 `<stdbool.h>` for the `linkState`/`uploadState` flags. `uploadState` is **defined in `timer.c`** and `extern` in `main.c` ‚Ä?both must see the same stdbool `bool` or the extern type mismatches; any TU touching these flags needs `#include "stdbool.h"`, not `define.h`'s typedef.
- `Source/Handware/tftprintf/printf.c` is a tiny ported printf for output routing ‚Ä?not the toolchain printf.
- `G0/App/nfc/user/queue.c` and `Source/Handware/queue/queue.c` are near-identical ring queues (16√ó32B) kept per-platform; don't merge them across platforms.
- `tools/arm_ext.h` + `tools/syntax_check.py` provide a GCC-based syntax sanity check (gcc doesn't know `__weak`/`__align`, so arm_ext.h stubs them). Run `python tools/syntax_check.py` after structural edits ‚Ä?it checks all 6 product source sets.
