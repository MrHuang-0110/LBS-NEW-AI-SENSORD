# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Firmware for HK32F030MF4P6 (Cortex-M0, 16KB flash / 2KB RAM) — a family of small sensor/actuator peripherals that talk to a host over UART. One codebase compiles into **four mutually exclusive product variants**, selected by `#define` switches in [Project/senords.h](Project/senords.h):

| Define        | Product                         | `USER_ObjectID` |
|---------------|---------------------------------|-----------------|
| `KT_MOTOR`    | Motor w/ KT782xx magnetic encoder (SPI) | `0xB1` |
| `BIG_MOTOR`   | Big motor (encoder on TIM)      | `0xA1` |
| `SMALL_MOTOR` | Small motor (encoder on TIM)    | `0xA6` |
| `COLOR`       | LTR-381RGB color/light sensor (I2C) | `0xA2` |

Exactly one of these must be `1` (the rest `0`). Many files are gated with `#if (KT_MOTOR||BIG_MOTOR||SMALL_MOTOR)` vs `#if COLOR`, so the active variant changes which driver sources are compiled in ([main.c](Source/User/main.c) is the clearest example). `USER_SourceID` (`0x97`) is the host-facing ID used in every outbound packet.

## Build / flash / clean

This is a **Keil µVision (MDK-ARM)** project — there is no Makefile or CMake.

- **Open & build:** Open [Project/Project.uvprojx](Project/Project.uvprojx) in Keil µVision and build (F7). Target name is `HK32F030MF4P6`.
- **Flash / debug:** via J-Link (see [Project/JLinkSettings.ini](Project/JLinkSettings.ini)); download from Keil (F8). The flash algorithm is `HK32F030MXX_16.FLM`, sector `0x08000000`, size `0x4000`.
- **Preprocessor defines:** `HK32F030M,HK32F030MF4P6` (set in target options). Variant defines live in `senords.h`, **not** in the Keil defines — edit them there to switch product.
- **Clean intermediate files:** run `keilkill.bat` from the repo root (deletes `*.o *.crf *.axf *.map *.lst` etc. across the tree). Note: it does **not** delete `*.opt`, preserving J-Link settings.
- **No test framework.** Verification is on hardware over UART.

Toolchain includes (`..\Source\Handware\*`, `..\Source\Libraries\...`, `..\Project`) are preconfigured in the uvprojx — adding a new driver dir means adding it to the target's Include Paths too.

## Architecture

### Boot & runtime
- `main()` ([Source/User/main.c](Source/User/main.c)) relocates the vector table to `FLASH_BASE | 0x2000` — this firmware is built to be launched by a **bootloader at offset `0x2000`**. Device version is read back from flash address `0x08001900` (a reserved field, not the vector table).
- Init order: ring queue → TIM6 tick → SysTick delay → variant-specific driver → protocol → USART1 → IWDG, then the main loop only does `pull_data_from_queue()` + periodic upload + IWDG feed.
- **IWDG** is enabled (`IWDG_Prescaler_128`, reload 49) and fed in the main loop — a hung ISR or infinite loop will reset the chip.

### Communication protocol ([Source/Handware/dataAnalysisProtocol/data_analysis.c](Source/Handware/dataAnalysisProtocol/data_analysis.c))
Shared wire format on USART1, framed by `0x5A` head / `0xA5` tail with a trailing sum-CRC (`& 0xFF`):

```
[0x5A][ObjectID][SourceID][len][typeIndex][payload...][crc][0xA5]
```

- Outbound: `SendCOMdata` (text/C-string payload) and `SendCOMdataByte` (binary struct payload) — both build into a single `_port_.tx_cache` of `TX_CACHE_SIZE` (32 bytes), so **payloads > ~25 bytes overflow**. Outbound frames are written through a callback registered in `init_analysis()` (wired to `Usart_SendArray`).
- Inbound: parsed in [Project/senords.c](Project/senords.c) `pull_data_from_queue()`, dispatched on `typeIndex`:
  - `0x09` `"Please Link"` — host handshake; clears `linkState`, stops motors, replies `"Play Aplication"`, then sets `linkState=true`. **No data uploads until linked.**
  - `0xED` — motor PWM duty command (writes `TIM2->CCR1/CCR2`).
  - `0xDD` — encoder position reset.
  - `0xEE` — `NVIC_SystemReset()` (remote reboot).

### UART RX path (interrupt → ring queue → main loop)
- [Source/Handware/USART/usart.c](Source/Handware/USART/usart.c): USART1 @ 115200 8N1, **PA3 (TX, AF1) / PB4 (RX, AF1)**.
- RX is interrupt-driven. `USART1_IRQHandler` writes bytes one at a time into a slot from `cbWrite(&rx_queue)`, finalizing the frame on either a full 32-byte node or the IDLE line event.
- The ring queue ([Source/Handware/queue/queue.c](Source/Handware/queue/queue.c)) holds `QUEUE_NODE_NUM`(16) frames × `QUEUE_NODE_DATA_LEN`(32) bytes. The main loop pulls whole frames; **never parse protocol inside the ISR** — enqueue there, parse in `pull_data_from_queue()`.
- `Usart_SendByte`/`Usart_SendArray` use a TX timeout (`USART_TX_TIMEOUT_MS = 50`) based on `getTickTime()` to avoid blocking forever on a stuck line.

### Timing
- [Source/Handware/TIMER/timer.c](Source/Handware/TIMER/timer.c): TIM6 is the 1 ms system tick (`ticktime`, 32 MHz / 32 prescaler / 1000 period). `getTickTime()` is the monotonic ms counter used for TX timeout and encoder speed.
- TIM6 IRQ also drives the upload cadence: when linked, sets `uploadState = true` every ~5 ticks; the main loop then calls `uploading_data()`.
- **SysTick** ([Source/Handware/SysTick/systick_delay.c](Source/Handware/SysTick/systick_delay.c)) provides blocking `delay_ms`/`delay_us` — separate from TIM6.

### Upload payloads ([Project/senords.c](Project/senords.c))
`uploading_data()` packs a variant-specific `__attribute__((packed))` struct and sends it via `SendCOMdataByte(..., 0xED)`:
- Motor variants: `motor_packet_t { int speed, pos, angle, version }`. Speed/position come from the TIM encoder ([Source/Handware/PWM/pwm.c](Source/Handware/PWM/pwm.c)); angle = `pos / PPR * 360`.
- `KT_MOTOR` additionally uses the KT782xx magnetic encoder over SPI ([Source/Handware/KTH782xx/kt782xx.c](Source/Handware/KTH782xx/kt782xx.c)) for absolute angle.
- `COLOR`: `color_packet_t { uint version; float lux; ushort R, G, B raw }` from the LTR-381RGB over bit-banged I2C ([Source/Handware/ltr381xx/ltr381xx.c](Source/Handware/ltr381xx/ltr381xx.c), [Source/Handware/IIC/bsp_i2c.c](Source/Handware/IIC/bsp_i2c.c)). The driver expects the porting functions `ltr381_i2c_write`/`ltr381_i2c_read` to be implemented against the bsp.

### Library layout
- [Source/Libraries/](Source/Libraries/) — vendor HK32F030M CMSIS + peripheral firmware (HK32F030M_Lib). Treat as upstream; don't edit.
- [Source/User/](Source/User/) — app config: [hk32f030m_conf.h](Source/User/hk32f030m_conf.h) (peripheral enable map), [hk32f030m_it.c](Source/User/hk32f030m_it.c) (other ISRs), [define.h](Source/User/define.h) (fixed-width typedefs `u8int`/`u16int`/`u32int`/`BOOL`/`bool` — note these `bool`/`BOOL` are `volatile unsigned char`, **not** `stdbool`).
- [Source/Handware/](Source/Handware/) — board-support drivers, one folder per peripheral.
- `Project/Objects` and `Project/Listings` are build outputs — ignore them.

## Conventions specific to this codebase
- `0x5A`/`0xA5` framing and the `_DATA_ANALYSIS` singletons (`_port_`) are shared across all variants — keep new message types inside the existing `typeIndex` switch rather than adding a parallel parser.
- Source files in `Source/User/` and `Source/Handware/` frequently have GBK-encoded Chinese comments. Preserve existing encoding when editing; don't re-encode wholesale.
- [printf.c](Source/Handware/tftprintf/printf.c) is a tiny printf ported for output routing — not the toolchain's printf.
