# -*- coding: utf-8 -*-
"""
Generate multi-target Project.uvprojx for the merged sensor firmware project.

Targets (one per product, macro-switch = select target):
  1. HK32F030MF4P6_BIG_MOTOR   (BIG_MOTOR=1)
  2. HK32F030MF4P6_SMALL_MOTOR (SMALL_MOTOR=1)
  3. HK32F030MF4P6_COLOR       (COLOR=1)
  4. STM32G030F6P6_GRAY_V1     (GRAY_V1=1)
  5. STM32G030F6P6_NFC         (NFC_G030F6=1)
  6. STM32G030K6T6_GRAY_V2     (GRAY_V2=1)

Shared trees:
  Source/  -> HK32F030M (std-periph lib)
  G0/      -> STM32G030 (HAL lib, Core, per-product App dirs)
"""
import re, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRJ = os.path.join(ROOT, "Project")

def read(p):
    with open(p, "r", encoding="utf-8", errors="replace") as f:
        return f.read()

def extract_target(xml):
    """Extract the first <Target>...</Target> block (incl. tag)."""
    m = re.search(r"<Target>.*?</Target>", xml, re.S)
    if not m:
        raise RuntimeError("no <Target> block found")
    return m.group(0)

# ---------- templates ----------
hk32_xml   = read(os.path.join(PRJ, "Project.uvprojx.bak"))
g0f6_xml   = read(r"E:\LBS-Project\NEW-AI-PROJECT\piKaNewAI-sensord\app\NFC_G030F6\MDK-ARM\NFC_G030F6.uvprojx")
g0k6_xml   = read(r"E:\LBS-Project\NEW-AI-PROJECT\piKaNewAI-sensord\app\LB_GrayV2\grayscale_sensor\MDK-ARM\grayscale_sensor.uvprojx")

hk32_tgt = extract_target(hk32_xml)
g0f6_tgt = extract_target(g0f6_xml)
g0k6_tgt = extract_target(g0k6_xml)

# ---------- helpers ----------
def set_target_name(t, name):
    return re.sub(r"<TargetName>.*?</TargetName>", "<TargetName>%s</TargetName>" % name, t, count=1)

def set_define(t, define):
    # first <Define> inside <Cads><VariousControls> after the compiler settings
    return re.sub(r"(<Cads>.*?<VariousControls>\s*<MiscControls>.*?</MiscControls>\s*<Define>).*?(</Define>)",
                  lambda m: m.group(1) + define + m.group(2), t, count=1, flags=re.S)

def set_include_path(t, path):
    return re.sub(r"(<Cads>.*?<IncludePath>).*?(</IncludePath>)",
                  lambda m: m.group(1) + path + m.group(2), t, count=1, flags=re.S)

def set_output(t, outdir, outname):
    t = re.sub(r"<OutputDirectory>.*?</OutputDirectory>",
               lambda m: "<OutputDirectory>%s</OutputDirectory>" % outdir, t, count=1)
    t = re.sub(r"<OutputName>.*?</OutputName>",
               lambda m: "<OutputName>%s</OutputName>" % outname, t, count=1)
    t = re.sub(r"<ListingPath>.*?</ListingPath>",
               lambda m: "<ListingPath>%s</ListingPath>" % (outdir.replace("Objects", "Listings")), t, count=1)
    return t

def set_groups(t, groups_xml):
    return re.sub(r"<Groups>.*?</Groups>",
                  lambda m: "<Groups>" + groups_xml + "</Groups>", t, count=1, flags=re.S)

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

# ---------- HK32 groups ----------
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

# ---------- G0 groups ----------
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
    """app_dir: relative path to the product App dir (e.g. ..\G0\App\gray_v1)."""
    files = []
    if startup:
        files.append(("startup_stm32g030xx.s", 2, r"..\G0\Core\startup_stm32g030xx.s"))
    # system + app core files
    files += [("system_stm32g0xx.c", 1, r"..\G0\Core\system_stm32g0xx.c")]
    for n, ft, p in extra_app:
        files.append((n, ft, p))
    g1 = group("Application/Core", files)
    # HAL drivers
    g2 = group("Drivers/STM32G0xx_HAL_Driver", hal_list)
    g3 = group("Drivers/CMSIS", [
        ("stm32g0xx_hal_conf.h", 5, app_dir + r"\stm32g0xx_hal_conf.h"),
    ])
    return g1 + g2 + g3

G0_INC_COMMON = (r"..\G0\Core;..\G0\HAL\STM32G0xx_HAL_Driver\Inc;"
                 r"..\G0\HAL\STM32G0xx_HAL_Driver\Inc\Legacy;"
                 r"..\G0\HAL\CMSIS\Device\ST\STM32G0xx\Include;"
                 r"..\G0\HAL\CMSIS\Include;..\Project")

# ---- GRAY_V1 (STM32G030F6P6) ----
GRAYV1_APP = [("main.c", 1, r"..\G0\App\gray_v1\main.c"),
              ("adc.c", 1, r"..\G0\App\gray_v1\adc.c"),
              ("gpio.c", 1, r"..\G0\App\gray_v1\gpio.c"),
              ("tim.c", 1, r"..\G0\App\gray_v1\tim.c"),
              ("usart.c", 1, r"..\G0\App\gray_v1\usart.c"),
              ("stm32g0xx_it.c", 1, r"..\G0\App\gray_v1\stm32g0xx_it.c"),
              ("stm32g0xx_hal_msp.c", 1, r"..\G0\App\gray_v1\stm32g0xx_hal_msp.c"),
              ("rx_data_queue.c", 1, r"..\G0\App\gray_v1\rx_data_queue.c")]
GRAYV1_HAL = G0_HAL_BASE + G0_HAL_ADC
GRAYV1_GROUPS = g0_core_group(r"..\G0\App\gray_v1", GRAYV1_APP, True, GRAYV1_HAL)
GRAYV1_INC = r"..\G0\App\gray_v1;" + G0_INC_COMMON

# ---- NFC (STM32G030F6P6) ----
NFC_APP = [("main.c", 1, r"..\G0\App\nfc\main.c"),
           ("gpio.c", 1, r"..\G0\App\nfc\gpio.c"),
           ("spi.c", 1, r"..\G0\App\nfc\spi.c"),
           ("usart.c", 1, r"..\G0\App\nfc\usart.c"),
           ("stm32g0xx_it.c", 1, r"..\G0\App\nfc\stm32g0xx_it.c"),
           ("stm32g0xx_hal_msp.c", 1, r"..\G0\App\nfc\stm32g0xx_hal_msp.c"),
           ("rc522.c", 1, r"..\G0\App\nfc\rc522.c"),
           ("data_analysis.c", 1, r"..\G0\App\nfc\user\data_analysis.c"),
           ("queue.c", 1, r"..\G0\App\nfc\user\queue.c")]
NFC_HAL = G0_HAL_BASE + G0_HAL_SPI
NFC_GROUPS = g0_core_group(r"..\G0\App\nfc", NFC_APP, True, NFC_HAL)
NFC_INC = r"..\G0\App\nfc;..\G0\App\nfc\user;" + G0_INC_COMMON

# ---- GRAY_V2 (STM32G030K6T6) ----
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
GRAYV2_HAL = G0_HAL_BASE + G0_HAL_ADC
GRAYV2_GROUPS = g0_core_group(r"..\G0\App\gray_v2", GRAYV2_APP, True, GRAYV2_HAL)
GRAYV2_INC = r"..\G0\App\gray_v2;" + G0_INC_COMMON

# ---------- build HK32 targets (BIG_MOTOR / SMALL_MOTOR / COLOR) ----------
def make_hk32(name, define, outname):
    t = hk32_tgt
    t = set_target_name(t, name)
    t = set_define(t, "HK32F030M,HK32F030MF4P6,%s" % define)
    t = set_include_path(t, HK32_INC)
    t = set_output(t, ".\\Objects\\%s\\" % outname, outname)
    t = set_groups(t, HK32_GROUPS)
    return t

hk32_big    = make_hk32("HK32F030MF4P6_BIG_MOTOR",   "BIG_MOTOR=1",   "BIG_MOTOR")
hk32_small  = make_hk32("HK32F030MF4P6_SMALL_MOTOR", "SMALL_MOTOR=1", "SMALL_MOTOR")
hk32_color  = make_hk32("HK32F030MF4P6_COLOR",       "COLOR=1",       "COLOR")

# ---------- build G0 targets ----------
def make_g0(template, name, device, define, inc, groups, outname, outdir):
    t = template
    t = set_target_name(t, name)
    if device:
        t = re.sub(r"<Device>.*?</Device>", "<Device>%s</Device>" % device, t, count=1)
        t = re.sub(r"<Cpu>.*?</Cpu>", lambda m: m.group(0), t, count=1)  # keep cpu, Keil re-derives
    t = set_define(t, define)
    t = set_include_path(t, inc)
    t = set_output(t, outdir, outname)
    t = set_groups(t, groups)
    return t

g0_v1 = make_g0(g0f6_tgt, "STM32G030F6P6_GRAY_V1", "STM32G030F6Px",
                "USE_HAL_DRIVER,STM32G030xx,GRAY_V1=1", GRAYV1_INC, GRAYV1_GROUPS,
                "GRAY_V1", ".\\Objects\\GRAY_V1\\")
g0_nfc = make_g0(g0f6_tgt, "STM32G030F6P6_NFC", "STM32G030F6Px",
                 "USE_HAL_DRIVER,STM32G030xx,NFC_G030F6=1", NFC_INC, NFC_GROUPS,
                 "NFC_G030F6", ".\\Objects\\NFC_G030F6\\")
g0_v2 = make_g0(g0k6_tgt, "STM32G030K6T6_GRAY_V2", "STM32G030K6Tx",
                "USE_HAL_DRIVER,STM32G030xx,GRAY_V2=1", GRAYV2_INC, GRAYV2_GROUPS,
                "GRAY_V2", ".\\Objects\\GRAY_V2\\")

targets = [hk32_big, hk32_small, hk32_color, g0_v1, g0_nfc, g0_v2]

# ---------- assemble ----------
header = read(os.path.join(PRJ, "Project.uvprojx.bak"))
header = header[:header.index("<Targets>")]
footer = read(os.path.join(PRJ, "Project.uvprojx.bak"))
footer = footer[footer.index("</Targets>") + len("</Targets>"):]

out = header + "<Targets>\n" + "\n".join(targets) + "\n  </Targets>" + footer
out = out.replace("<LayName>Project</LayName>", "<LayName>LBS-NEW-AI-SENSORD</LayName>")

dst = os.path.join(PRJ, "Project.uvprojx")
with open(dst, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(out)
print("written:", dst, len(out), "bytes")
print("targets:", [re.search(r"<TargetName>(.*?)</TargetName>", t).group(1) for t in targets])
