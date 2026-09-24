# -*- coding: utf-8 -*-
"""Syntax-check all product sources with gcc (x86) as a proxy for ARM compilers.

Runs on Windows (Keil dev box) and on Linux/WSL: ROOT is derived from this
file's location and directory separators are normalised.
"""
import subprocess, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ARM_EXT = os.path.join(ROOT, "tools", "arm_ext.h")


def norm(path):
    """Accept the Windows-style paths used below on any platform."""
    return path.replace("\\", os.sep)


def check(files, macro, incdirs, label):
    fails = 0
    for f in files:
        cmd = ["gcc", "-fsyntax-only", "-x", "c"]
        if label.startswith("HK32"):
            cmd += ["-D" + macro + "=1", "-DHK32F030M", "-DHK32F030MF4P6"]
        elif label.startswith("PY32"):
            cmd += ["-DUSE_HAL_DRIVER", "-DPY32F002Bx5", "-D" + macro + "=1"]
        else:
            cmd += ["-DUSE_HAL_DRIVER", "-DSTM32G030xx", "-D" + macro + "=1"]
        cmd += ["-include", ARM_EXT]
        for d in incdirs:
            cmd += ["-I", os.path.join(ROOT, norm(d))]
        cmd.append(os.path.join(ROOT, norm(f)))
        p = subprocess.run(cmd, capture_output=True)
        err = p.stderr.decode("utf-8", errors="replace") + p.stdout.decode("utf-8", errors="replace")
        errs = [l for l in err.splitlines() if "error:" in l and "senords.h:" not in l]
        if errs:
            fails += 1
            print("  FAIL %s:" % f)
            for e in errs[:4]:
                print("    ", e.strip())
    print("%s: %d files with errors" % (label, fails))
    return fails


def rel(*parts):
    return os.path.join(*parts)


G0_INC = [r"G0\Core", r"G0\HAL\STM32G0xx_HAL_Driver\Inc", r"G0\HAL\STM32G0xx_HAL_Driver\Inc\Legacy",
          r"G0\HAL\CMSIS\Device\ST\STM32G0xx\Include", r"G0\HAL\CMSIS\Include", r"Project"]
HK_INC = [r"Source\Libraries\HK32F030M_Lib\inc", r"Source\Libraries\HK32F030M_Lib\src",
          r"Source\Libraries\CMSIS\CM0\Core", r"Source\Libraries\CMSIS\CM0",
          r"Source\Libraries\CMSIS\HK32F030M\Include", r"Source\Libraries\CMSIS\HK32F030M\Source",
          r"Source\User", r"Source\Handware\dataAnalysisProtocol", r"Source\Handware\queue",
          r"Source\Handware\USART", r"Source\Handware\TIMER", r"Source\Handware\PWM",
          r"Source\Handware\SysTick", r"Project", r"Source\Handware\ltr381xx", r"Source\Handware\IIC"]
PY32_INC = [r"PY32\Core", r"PY32\HAL\PY32F002B_HAL_Driver\Inc",
            r"PY32\HAL\CMSIS\Include", r"PY32\HAL\CMSIS\Device\PY32F0xx\Include",
            r"PY32\App\ir_remote", r"Project"]

total = 0

v1_files = [rel("G0", "App", "gray_v1", n) for n in
            ["main.c","adc.c","gpio.c","tim.c","usart.c","stm32g0xx_it.c","stm32g0xx_hal_msp.c","rx_data_queue.c"]]
total += check(v1_files, "GRAY_V1", G0_INC + [r"G0\App\gray_v1"], "GRAY_V1")

v2_files = [rel("G0", "App", "gray_v2", n) for n in
            ["main.c","adc.c","dma.c","gpio.c","tim.c","usart.c","stm32g0xx_it.c","stm32g0xx_hal_msp.c",
             "agreement_comx.c","driver_gray.c","driver_sys.c","stmflash.c","rx_data_queue.c"]]
total += check(v2_files, "GRAY_V2", G0_INC + [r"G0\App\gray_v2"], "GRAY_V2")

nfc_files = [rel("G0", "App", "nfc", n) for n in
             ["main.c","gpio.c","spi.c","usart.c","stm32g0xx_it.c","stm32g0xx_hal_msp.c","rc522.c"]] + \
            [rel("G0", "App", "nfc", "user", n) for n in ["data_analysis.c","queue.c"]]
total += check(nfc_files, "NFC_G030F6", G0_INC + [r"G0\App\nfc", r"G0\App\nfc\user"], "NFC")

HK_FILES = [r"Source\User\main.c", r"Source\User\hk32f030m_it.c", r"Project\senords.c",
            r"Source\Handware\dataAnalysisProtocol\data_analysis.c", r"Source\Handware\queue\queue.c",
            r"Source\Handware\USART\usart.c", r"Source\Handware\TIMER\timer.c",
            r"Source\Handware\PWM\pwm.c", r"Source\Handware\SysTick\systick_delay.c",
            r"Source\Handware\ltr381xx\ltr381xx.c", r"Source\Handware\IIC\bsp_i2c.c"]
for macro in ["BIG_MOTOR", "SMALL_MOTOR", "COLOR", "ELECTROMAGNETIC_SENSOR"]:
    total += check(HK_FILES, macro, HK_INC, "HK32-" + macro)

PY32_FILES = [rel("PY32", "App", "ir_remote", n) for n in
              ["main.c", "gpio.c", "usart.c", "tim.c", "adc.c", "ir_proto.c", "driver_ir.c",
               "agreement_comx.c", "rx_data_queue.c", "py32f002b_hal_msp.c", "py32f002b_it.c"]] + \
             [rel("PY32", "Core", "system_py32f002b.c")]
total += check(PY32_FILES, "IR_REMOTE", PY32_INC, "PY32-IR_REMOTE")

print("TOTAL errors:", total)
sys.exit(1 if total else 0)
