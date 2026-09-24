# -*- coding: utf-8 -*-
"""
Generate 7 SINGLE-TARGET Keil projects under Project/.

Why single-target: UV4 (GUI Rebuild AND CLI -b) merges ALL targets' files into
the active target for ANY multi-target project (verified with official Keil
projects - see Boot_All/tools/gen_boot_all_uvprojx.py). A multi-target
.uvprojx is unusable in UV4, so each product gets its own single-target
project sharing the Source/ + G0/ source trees.

Templates: the already-generated single-target projects themselves
(HK32_BIG_MOTOR.uvprojx, STM32_GRAY_V1.uvprojx, STM32_GRAY_V2.uvprojx),
whose TargetOption (per-product AC5/AC6 compiler, memory layout, debugger)
came from the original multi-target project. The generator only rewrites the
per-product fields (TargetName / OutputName / OutputDirectory / Define /
IncludePath / Groups / fromelf --bin), so templates keep their meaning no
matter which product they are used for.

Products (template file -> project file):
  HK32_BIG_MOTOR    (AC6) -> HK32_BIG_MOTOR.uvprojx / SMALL_MOTOR / COLOR / ELECTROMAGNETIC_SENSOR
  STM32_GRAY_V1     (AC6, bootloader app @ 0x08003800) -> STM32_GRAY_V1.uvprojx
  STM32_NFC         (AC5, standalone @ 0x08000000)     -> STM32_NFC.uvprojx
  STM32_GRAY_V2     (AC6) -> STM32_GRAY_V2.uvprojx
"""
import re, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRJ = os.path.join(ROOT, "Project")

def read(p):
    try:
        with open(p, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError as e:
        raise SystemExit("cannot read %s: %s" % (p, e))

def split_tpl(path):
    """Return (header, first <Target> block, footer) of a single-target project."""
    x = read(path)
    i = x.index("<Targets>")
    j = x.index("</Targets>") + len("</Targets>")
    m = re.search(r"[ \t]*<Target>.*?</Target>", x[i:j], re.S)
    if m is None:
        raise ValueError("no <Target> block in %s" % path)
    # drop the blank-line run before <RTE>/<LayerInfo>: build_single() emits its
    # own separator, otherwise one blank line is added on every run
    footer = x[j:].lstrip("\r\n")
    return x[:i], m.group(0), footer

TPL = {
    "HK32":  split_tpl(os.path.join(PRJ, "HK32_BIG_MOTOR.uvprojx")),
    "G0F6":  split_tpl(os.path.join(PRJ, "STM32_GRAY_V1.uvprojx")),
    "G0K6":  split_tpl(os.path.join(PRJ, "STM32_GRAY_V2.uvprojx")),
    "NFC":   split_tpl(os.path.join(PRJ, "STM32_NFC.uvprojx")),
    "PY32":  split_tpl(os.path.join(PRJ, "PY32_IR_REMOTE.uvprojx")),
}

# ---------- file group helpers ----------
def file_entry(name, ftype, path):
    return ("            <File>\n"
            "              <FileName>%s</FileName>\n"
            "              <FileType>%d</FileType>\n"
            "              <FilePath>%s</FilePath>\n"
            "            </File>\n") % (name, ftype, path)

def group(name, files):
    s = "        <Group>\n"
    s += "          <GroupName>%s</GroupName>\n" % name
    s += "          <Files>\n"
    for n, ft, p in files:
        s += file_entry(n, ft, p)
    s += "          </Files>\n"
    s += "        </Group>\n"
    return s

# ---------- HK32 groups (Source/ tree) ----------
hk32_user = group("User", [
    ("hk32f030m_it.c", 1, r"..\Source\User\hk32f030m_it.c"),
    ("main.c", 1, r"..\Source\User\main.c"),
    ("data_analysis.c", 1, r"..\Source\Handware\dataAnalysisProtocol\data_analysis.c"),
    ("queue.c", 1, r"..\Source\Handware\queue\queue.c"),
    ("usart.c", 1, r"..\Source\Handware\USART\usart.c"),
    ("senords.c", 1, r".\senords.c"),
    ("timer.c", 1, r"..\Source\Handware\TIMER\timer.c"),
    ("pwm.c", 1, r"..\Source\Handware\PWM\pwm.c"),
    ("systick_delay.c", 1, r"..\Source\Handware\SysTick\systick_delay.c"),
    ("senords.h", 5, r".\senords.h"),
    ("ltr381xx.c", 1, r"..\Source\Handware\ltr381xx\ltr381xx.c"),
    ("bsp_i2c.c", 1, r"..\Source\Handware\IIC\bsp_i2c.c"),
])
hk32_stdperiph = group("StdPeriph_Driver", [
    ("hk32f030m_awu.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_awu.c"),
    ("hk32f030m_beep.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_beep.c"),
    ("hk32f030m_crc.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_crc.c"),
    ("hk32f030m_exti.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_exti.c"),
    ("hk32f030m_flash.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_flash.c"),
    ("hk32f030m_gpio.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_gpio.c"),
    ("hk32f030m_i2c.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_i2c.c"),
    ("hk32f030m_iwdg.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_iwdg.c"),
    ("hk32f030m_misc.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_misc.c"),
    ("hk32f030m_pwr.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_pwr.c"),
    ("hk32f030m_rcc.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_rcc.c"),
    ("hk32f030m_syscfg.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_syscfg.c"),
    ("hk32f030m_tim.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_tim.c"),
    ("hk32f030m_usart.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_usart.c"),
    ("hk32f030m_wwdg.c", 1, r"..\Source\Libraries\HK32F030M_Lib\src\hk32f030m_wwdg.c"),
])
hk32_cmsis = group("CMSIS", [
    ("system_hk32f030m.c", 1, r"..\Source\Libraries\CMSIS\HK32F030M\Source\system_hk32f030m.c"),
])
hk32_startup = group("Startup", [
    ("KEIL_Startup_hk32f030m.s", 2, r"..\Source\Libraries\CMSIS\HK32F030M\Source\KEIL_Startup_hk32f030m.s"),
])
hk32_doc = group("Doc", [
    ("Readme.txt", 5, r"..\Doc\Readme.txt"),
])
HK32_GROUPS = hk32_user + hk32_stdperiph + hk32_cmsis + hk32_startup + hk32_doc

HK32_INC = (r"..\Source\Libraries\HK32F030M_Lib\inc;..\Source\Libraries\HK32F030M_Lib\src;"
            r"..\Source\Libraries\CMSIS\CM0\Core;..\Source\Libraries\CMSIS\CM0;"
            r"..\Source\Libraries\CMSIS\HK32F030M\Include;..\Source\Libraries\CMSIS\HK32F030M\Source;"
            r"..\Source\User;..\Source\User\led;..\Source\Handware\dataAnalysisProtocol;"
            r"..\Source\Handware\queue;..\Source\Handware\USART;..\Source\Handware\TIMER;"
            r"..\Source\Handware\PWM;..\Source\Handware\SysTick;..\Project;"
            r"..\Source\Handware\ltr381xx;..\Source\Handware\IIC")

# ---------- G0 groups (G0/ tree) ----------
G0_HAL_SRC = r"..\G0\HAL\STM32G0xx_HAL_Driver\Src"
G0_HAL_BASE = [("stm32g0xx_hal_rcc.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_rcc.c"),
               ("stm32g0xx_hal_rcc_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_rcc_ex.c"),
               ("stm32g0xx_ll_rcc.c", 1, G0_HAL_SRC + r"\stm32g0xx_ll_rcc.c"),
               ("stm32g0xx_hal_flash.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_flash.c"),
               ("stm32g0xx_hal_flash_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_flash_ex.c"),
               ("stm32g0xx_hal_gpio.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_gpio.c"),
               ("stm32g0xx_hal_dma.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_dma.c"),
               ("stm32g0xx_hal_dma_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_dma_ex.c"),
               ("stm32g0xx_hal_pwr.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_pwr.c"),
               ("stm32g0xx_hal_pwr_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_pwr_ex.c"),
               ("stm32g0xx_hal_cortex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_cortex.c"),
               ("stm32g0xx_hal.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal.c"),
               ("stm32g0xx_hal_exti.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_exti.c"),
               ("stm32g0xx_hal_uart.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_uart.c"),
               ("stm32g0xx_hal_uart_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_uart_ex.c"),
               ("stm32g0xx_hal_tim.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_tim.c"),
               ("stm32g0xx_hal_tim_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_tim_ex.c")]
G0_HAL_ADC = [("stm32g0xx_hal_adc.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_adc.c"),
              ("stm32g0xx_hal_adc_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_adc_ex.c"),
              ("stm32g0xx_ll_adc.c", 1, G0_HAL_SRC + r"\stm32g0xx_ll_adc.c")]
G0_HAL_SPI = [("stm32g0xx_hal_spi.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_spi.c"),
              ("stm32g0xx_hal_spi_ex.c", 1, G0_HAL_SRC + r"\stm32g0xx_hal_spi_ex.c")]

def g0_core_group(app_dir, extra_app, startup, hal_list):
    r"""app_dir: relative path to the product App dir (e.g. ..\G0\App\gray_v1)."""
    files = []
    if startup:
        files.append(("startup_stm32g030xx.s", 2, r"..\G0\Core\startup_stm32g030xx.s"))
    files += [("system_stm32g0xx.c", 1, r"..\G0\Core\system_stm32g0xx.c")]
    for n, ft, p in extra_app:
        files.append((n, ft, p))
    g1 = group("Application/Core", files)
    g2 = group("Drivers/STM32G0xx_HAL_Driver", hal_list)
    g3 = group("Drivers/CMSIS", [
        ("stm32g0xx_hal_conf.h", 5, app_dir + r"\stm32g0xx_hal_conf.h"),
    ])
    return g1 + g2 + g3

G0_INC_COMMON = (r"..\G0\Core;..\G0\HAL\STM32G0xx_HAL_Driver\Inc;"
                 r"..\G0\HAL\STM32G0xx_HAL_Driver\Inc\Legacy;"
                 r"..\G0\HAL\CMSIS\Device\ST\STM32G0xx\Include;"
                 r"..\G0\HAL\CMSIS\Include;..\Project")

GRAYV1_APP = [("main.c", 1, r"..\G0\App\gray_v1\main.c"),
              ("adc.c", 1, r"..\G0\App\gray_v1\adc.c"),
              ("gpio.c", 1, r"..\G0\App\gray_v1\gpio.c"),
              ("tim.c", 1, r"..\G0\App\gray_v1\tim.c"),
              ("usart.c", 1, r"..\G0\App\gray_v1\usart.c"),
              ("stm32g0xx_it.c", 1, r"..\G0\App\gray_v1\stm32g0xx_it.c"),
              ("stm32g0xx_hal_msp.c", 1, r"..\G0\App\gray_v1\stm32g0xx_hal_msp.c"),
              ("rx_data_queue.c", 1, r"..\G0\App\gray_v1\rx_data_queue.c")]
GRAYV1_GROUPS = g0_core_group(r"..\G0\App\gray_v1", GRAYV1_APP, True, G0_HAL_BASE + G0_HAL_ADC)
GRAYV1_INC = r"..\G0\App\gray_v1;" + G0_INC_COMMON

NFC_APP = [("main.c", 1, r"..\G0\App\nfc\main.c"),
           ("gpio.c", 1, r"..\G0\App\nfc\gpio.c"),
           ("spi.c", 1, r"..\G0\App\nfc\spi.c"),
           ("usart.c", 1, r"..\G0\App\nfc\usart.c"),
           ("stm32g0xx_it.c", 1, r"..\G0\App\nfc\stm32g0xx_it.c"),
           ("stm32g0xx_hal_msp.c", 1, r"..\G0\App\nfc\stm32g0xx_hal_msp.c"),
           ("rc522.c", 1, r"..\G0\App\nfc\rc522.c"),
           ("data_analysis.c", 1, r"..\G0\App\nfc\user\data_analysis.c"),
           ("queue.c", 1, r"..\G0\App\nfc\user\queue.c")]
NFC_GROUPS = g0_core_group(r"..\G0\App\nfc", NFC_APP, True, G0_HAL_BASE + G0_HAL_SPI)
NFC_INC = r"..\G0\App\nfc;..\G0\App\nfc\user;" + G0_INC_COMMON

GRAYV2_APP = [("main.c", 1, r"..\G0\App\gray_v2\main.c"),
              ("adc.c", 1, r"..\G0\App\gray_v2\adc.c"),
              ("dma.c", 1, r"..\G0\App\gray_v2\dma.c"),
              ("gpio.c", 1, r"..\G0\App\gray_v2\gpio.c"),
              ("tim.c", 1, r"..\G0\App\gray_v2\tim.c"),
              ("usart.c", 1, r"..\G0\App\gray_v2\usart.c"),
              ("stm32g0xx_it.c", 1, r"..\G0\App\gray_v2\stm32g0xx_it.c"),
              ("stm32g0xx_hal_msp.c", 1, r"..\G0\App\gray_v2\stm32g0xx_hal_msp.c"),
              ("agreement_comx.c", 1, r"..\G0\App\gray_v2\agreement_comx.c"),
              ("driver_gray.c", 1, r"..\G0\App\gray_v2\driver_gray.c"),
              ("driver_sys.c", 1, r"..\G0\App\gray_v2\driver_sys.c"),
              ("stmflash.c", 1, r"..\G0\App\gray_v2\stmflash.c"),
              ("rx_data_queue.c", 1, r"..\G0\App\gray_v2\rx_data_queue.c")]
GRAYV2_GROUPS = g0_core_group(r"..\G0\App\gray_v2", GRAYV2_APP, True, G0_HAL_BASE + G0_HAL_ADC)
GRAYV2_INC = r"..\G0\App\gray_v2;" + G0_INC_COMMON

# ---------- PY32 groups (PY32/ tree) ----------
PY32_HAL_SRC = r"..\PY32\HAL\PY32F002B_HAL_Driver\Src"
PY32_HAL = [(n, 1, PY32_HAL_SRC + "\\" + n) for n in [
    "py32f002b_hal.c", "py32f002b_hal_adc.c", "py32f002b_hal_comp.c", "py32f002b_hal_cortex.c",
    "py32f002b_hal_crc.c", "py32f002b_hal_exti.c", "py32f002b_hal_flash.c", "py32f002b_hal_gpio.c",
    "py32f002b_hal_i2c.c", "py32f002b_hal_iwdg.c", "py32f002b_hal_lptim.c", "py32f002b_hal_pwr.c",
    "py32f002b_hal_rcc.c", "py32f002b_hal_rcc_ex.c", "py32f002b_hal_spi.c", "py32f002b_hal_tim.c",
    "py32f002b_hal_tim_ex.c", "py32f002b_hal_uart.c", "py32f002b_hal_usart.c"]]

PY32_APP = [("startup_py32f002xx.s", 2, r"..\PY32\Core\startup_py32f002xx.s"),
            ("system_py32f002b.c", 1, r"..\PY32\Core\system_py32f002b.c"),
            ("main.c", 1, r"..\PY32\App\ir_remote\main.c"),
            ("gpio.c", 1, r"..\PY32\App\ir_remote\gpio.c"),
            ("usart.c", 1, r"..\PY32\App\ir_remote\usart.c"),
            ("tim.c", 1, r"..\PY32\App\ir_remote\tim.c"),
            ("adc.c", 1, r"..\PY32\App\ir_remote\adc.c"),
            ("ir_proto.c", 1, r"..\PY32\App\ir_remote\ir_proto.c"),
            ("driver_ir.c", 1, r"..\PY32\App\ir_remote\driver_ir.c"),
            ("agreement_comx.c", 1, r"..\PY32\App\ir_remote\agreement_comx.c"),
            ("rx_data_queue.c", 1, r"..\PY32\App\ir_remote\rx_data_queue.c"),
            ("py32f002b_hal_msp.c", 1, r"..\PY32\App\ir_remote\py32f002b_hal_msp.c"),
            ("py32f002b_it.c", 1, r"..\PY32\App\ir_remote\py32f002b_it.c"),
            ("main.h", 5, r"..\PY32\App\ir_remote\main.h")]

PY32_GROUPS = group("Application/Core", PY32_APP) + \
              group("Drivers/PY32F002B_HAL_Driver", PY32_HAL) + \
              group("Drivers/CMSIS", [("py32f002b_hal_conf.h", 5,
                                       r"..\PY32\App\ir_remote\py32f002b_hal_conf.h")]) + \
              group("Doc", [("Readme.txt", 5, r"..\Doc\Readme.txt")])

PY32_INC = (r"..\PY32\App\ir_remote;..\PY32\Core;"
            r"..\PY32\HAL\PY32F002B_HAL_Driver\Inc;..\PY32\HAL\CMSIS\Include;"
            r"..\PY32\HAL\CMSIS\Device\PY32F0xx\Include;..\Project")


# ---------- single-target builder ----------
FROMELF = r"D:\MDK\ARM\ARMCLANG\bin\fromelf.exe --bin -o .\Objects\{o}\{o}.bin .\Objects\{o}\{o}.axf"

def indent_block(block):
    """Emit a target block at a fixed 4-space base indentation.

    Templates are the generated projects themselves, so the block read back
    already carries the indentation left by the previous run. Shifting every
    line by (4 - first-line indent) instead of blindly prepending 4 spaces
    makes the output stable no matter how often the generator runs.
    """
    lines = block.split("\n")
    first = lines[0]
    cur = len(first) - len(first.lstrip(" "))
    delta = 4 - cur
    out = []
    for line in lines:
        if not line.strip():
            out.append(line)
            continue
        if delta >= 0:
            out.append(" " * delta + line)
        else:
            have = len(line) - len(line.lstrip(" "))
            out.append(line[min(-delta, have):])
    return "\n".join(out)


def build_single(tpl, tname, outname, define, incpath, groups):
    header, tgt, footer = tpl
    t = tgt
    t = re.sub(r"<TargetName>.*?</TargetName>", "<TargetName>%s</TargetName>" % tname, t, count=1)
    t = re.sub(r"<OutputName>.*?</OutputName>", "<OutputName>%s</OutputName>" % outname, t, count=1)
    t = re.sub(r"<OutputDirectory>.*?</OutputDirectory>",
               lambda m: "<OutputDirectory>.\\Objects\\%s\\</OutputDirectory>" % outname, t, count=1)
    t = re.sub(r"<ListingPath>.*?</ListingPath>",
               lambda m: "<ListingPath>.\\Listings\\%s\\</ListingPath>" % outname, t, count=1)
    t = re.sub(r"(<Cads>.*?<VariousControls>.*?<Define>).*?(</Define>)",
               lambda m: m.group(1) + define + m.group(2), t, count=1, flags=re.S)
    t = re.sub(r"(<Cads>.*?<VariousControls>.*?<IncludePath>).*?(</IncludePath>)",
               lambda m: m.group(1) + incpath + m.group(2), t, count=1, flags=re.S)
    t = re.sub(r"(<Aads>.*?<VariousControls>.*?<IncludePath>).*?(</IncludePath>)",
               lambda m: m.group(1) + incpath + m.group(2), t, count=1, flags=re.S)
    t = re.sub(r"<Groups>.*?</Groups>", lambda m: "<Groups>\n" + groups + "    </Groups>",
               t, count=1, flags=re.S)
    # AfterMake: force fromelf --bin conversion for bootloader flashing
    am = re.search(r"<AfterMake>.*?</AfterMake>", t, re.S)
    if am:
        am_new = re.sub(r"<RunUserProg1>\d+</RunUserProg1>", "<RunUserProg1>1</RunUserProg1>", am.group(0), count=1)
        am_new = re.sub(r"<UserProg1Name>.*?</UserProg1Name>",
                        lambda m: "<UserProg1Name>%s</UserProg1Name>" % FROMELF.format(o=outname),
                        am_new, count=1, flags=re.S)
        t = t.replace(am.group(0), am_new)
    else:
        print("WARNING: no <AfterMake>, fromelf not injected for", tname)
    return header + "<Targets>\n" + indent_block(t) + "\n  </Targets>\n\n\n" + footer

# ---------- 7 products ----------
PRODUCTS = [
    # (project file, template key, TargetName, OutputName, Define, IncludePath, Groups)
    ("HK32_BIG_MOTOR.uvprojx",    "HK32", "HK32_BIG_MOTOR",    "BIG_MOTOR",   "HK32F030M,HK32F030MF4P6,BIG_MOTOR=1",   HK32_INC, HK32_GROUPS),
    ("HK32_SMALL_MOTOR.uvprojx",  "HK32", "HK32_SMALL_MOTOR",  "SMALL_MOTOR", "HK32F030M,HK32F030MF4P6,SMALL_MOTOR=1", HK32_INC, HK32_GROUPS),
    ("HK32_COLOR.uvprojx",        "HK32", "HK32_COLOR",        "COLOR",       "HK32F030M,HK32F030MF4P6,COLOR=1",       HK32_INC, HK32_GROUPS),
    ("HK32_ELECTROMAGNETIC_SENSOR.uvprojx", "HK32", "HK32_ELECTROMAGNETIC_SENSOR", "ELECTROMAGNETIC_SENSOR", "HK32F030M,HK32F030MF4P6,ELECTROMAGNETIC_SENSOR=1", HK32_INC, HK32_GROUPS),
    ("STM32_GRAY_V1.uvprojx",     "G0F6", "STM32_GRAY_V1",     "GRAY_V1",     "USE_HAL_DRIVER,STM32G030xx,GRAY_V1=1",   GRAYV1_INC, GRAYV1_GROUPS),
    ("STM32_NFC.uvprojx",         "NFC",  "STM32_NFC",         "NFC_G030F6",  "USE_HAL_DRIVER,STM32G030xx,NFC_G030F6=1", NFC_INC, NFC_GROUPS),
    ("STM32_GRAY_V2.uvprojx",     "G0K6", "STM32_GRAY_V2",     "GRAY_V2",     "USE_HAL_DRIVER,STM32G030xx,GRAY_V2=1",   GRAYV2_INC, GRAYV2_GROUPS),
    ("PY32_IR_REMOTE.uvprojx",    "PY32", "PY32_IR_REMOTE",    "IR_REMOTE",   "USE_HAL_DRIVER,PY32F002Bx5,IR_REMOTE=1", PY32_INC, PY32_GROUPS),
]

for fname, tplkey, tname, outname, define, inc, groups in PRODUCTS:
    content = build_single(TPL[tplkey], tname, outname, define, inc, groups)
    dst = os.path.join(PRJ, fname)
    try:
        with open(dst, "w", encoding="utf-8", newline="") as f:
            f.write(content)
    except OSError as e:
        raise SystemExit("cannot write %s: %s" % (dst, e))
    print("written:", dst, len(content), "bytes")
