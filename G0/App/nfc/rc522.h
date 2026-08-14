#ifndef RC522_H
#define RC522_H

#include "stm32g0xx_hal.h"


#define  RFCfgReg_Val  0x68
//#define  DivIEnReg_Val  0x80
#define  DivIEnReg_Val  0xC0


#define  ComIEnReg_Val  0x80
#define  IRQMODE  0x01
#define  EXTI_Trigger_Mode  0x0C
#define  ACDConfigRegA_Val  0x02
#define  ACDConfigRegB_Val  0xA8
#define  ACDConfigRegD_Val  0x04
#define  ACDConfigRegH_Val  0x26
#define  ACDConfigRegI_Val  0x00
#define  ACDConfigRegJ_Val  0x55
#define  ACDConfigRegM_Val  0x01
#define  ACDConfigRegO_Val  0x00
// 常量定义
#define 	MI_OK                 0
#define 	MI_NOTAGERR           1
#define 	MI_ERR                2

// RC522 寄存器地址定义
#define CommandReg       0x01
#define CommIEnReg       0x02
#define CommIrqReg       0x04
#define FIFODataReg      0x09
#define FIFOLevelReg     0x0A
#define BitFramingReg    0x0D
#define TxControlReg     0x14
#define RFCfgReg         0x26
#define TModeReg         0x2A
#define TPrescalerReg    0x2B
#define TReloadRegH      0x2C
#define TReloadRegL      0x2D
#define VersionReg       0x37
#define ModeReg 0x11
#define TxASKReg 0x15
// RC522 指令定义
#define PCD_IDLE         0x00
#define PCD_TRANSCEIVE   0x0C

// 卡片请求模式
#define PCD_RESETPHASE   0x0F
#define PCD_TRANSCEIVE   0x0C
#define PICC_REQIDL      0x26
#define PICC_REQALL      0x52
#define PICC_ANTICOLL    0x93


// M1 卡相关指令
#define PICC_AUTHENT1A   0x60  // 验证密钥 A
#define PICC_READ        0x30  // 读数据块


//Page 0: Command and status
#define    RFU00                0x00
#define    CommandReg           0x01
#define    ComIEnReg            0x02
#define    DivIEnReg            0x03
#define    ComIrqReg            0x04
#define    DivIrqReg            0x05
#define    ErrorReg             0x06
#define    Status1Reg           0x07
#define    Status2Reg           0x08
#define    FIFODataReg          0x09
#define    FIFOLevelReg         0x0A
#define    WaterLevelReg        0x0B
#define    ControlReg           0x0C
#define    BitFramingReg        0x0D
#define    CollReg              0x0E
#define    ACDConfigReg         0x0F

//Page 1: Command	                
#define    RFU10                0x10
#define    ModeReg              0x11
#define    TxModeReg            0x12
#define    RxModeReg            0x13
#define    TxControlReg         0x14
#define    TxASKReg             0x15
#define    TxSelReg             0x16
#define    RxSelReg             0x17
#define    RxThresholdReg       0x18 
#define    DemodReg             0x19
#define    RFU1A                0x1A
#define    RFU1B                0x1B
#define    MfTxReg              0x1C
#define    MfRxReg              0x1D
#define    TypeBReg             0x1E
#define    SerialSpeedReg       0x1F
				                    
// Page 2: Configuration 	        
#define    ACDConfigSelReg      0x20
#define    CRCResultRegH        0x21
#define    CRCResultRegL        0x22
#define    RFU23                0x23
#define    ModWidthReg          0x24
#define    RFU25                0x25
#define    RFCfgReg             0x26
#define    GsNReg               0x27
#define    CWGsPReg             0x28
#define    ModGsPReg            0x29
#define    TModeReg             0x2A
#define    TPrescalerReg        0x2B
#define    TReloadRegH          0x2C
#define    TReloadRegL          0x2D
#define    TCounterValueRegH    0x2E
#define    TCounterValueRegL    0x2F
                   
//Page 3: Test Register 	        
#define    RFU30                0x30
#define    TestSel1Reg          0x31
#define    TestSel2Reg          0x32
#define    TestPinEnReg         0x33
#define    TestPinValueReg      0x34
#define    TestBusReg           0x35
#define    AutoTestReg          0x36
#define    VersionReg           0x37
#define    AnalogTestReg        0x38
#define    TestDAC1Reg          0x39
#define    TestDAC2Reg          0x3A
#define    TestADCReg           0x3B
#define    RFU3C                0x3C
#define    RFU3D                0x3D
#define    RFU3E                0x3E
#define    RFU3F                0x3F



/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算



/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte
#define MAXRLEN  18

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_ANTICOLL3        0x97               //防冲撞

#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠

#define     REQ_ALL               0x52
#define     KEYA                  0x60



#define ACDConfigA			 0x00
#define ACDConfigB			 0x01
#define ACDConfigC			 0x02
#define ACDConfigD			 0x03
#define ACDConfigE			 0x04
#define ACDConfigF			 0x05
#define ACDConfigG			 0x06
#define ACDConfigH			 0x07
#define ACDConfigI			 0x08
#define ACDConfigJ			 0x09
#define ACDConfigK			 0x0a
#define ACDConfigL			 0x0b   
#define ACDConfigM			 0x0c
#define ACDConfigN			 0x0d
#define ACDConfigO			 0x0e
#define ACDConfigP			 0x0f

void ACD_init_Fun(void);
void ACD_Fun(void);
void PCD_ACD_AutoCalc(void);
void PCD_ACD_Init(void);
char PCD_IRQ(void);


// 默认密钥 A (6字节)
static const uint8_t DefaultKeyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 函数声明
void RC522_Init(void);
uint8_t RC522_ReadRegister(uint8_t reg);
char PCD_SI522A_TypeA_GetUID(void);
void PCD_SI522A_TypeA(void);
void PCD_SI522A_TypeA_Init(void);
char PcdRequest(unsigned char req_code, unsigned char * pTagType ); //寻卡
char PcdAnticoll(unsigned char * pSnr, unsigned char anticollision_level); //读卡号
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);
char PcdWrite (unsigned char ucAddr, unsigned char * pData );
char PcdRead (unsigned char ucAddr, unsigned char * pData );
char PcdSelect (unsigned char * pSnr/*, unsigned char *sak*/);
void PcdAntennaOn(void);
void PcdAntennaOff(void);
void WaitCardOff(void);

uint8_t get_card(void);

#endif // RC522_H


