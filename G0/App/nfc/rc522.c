#include "rc522.h"
#include "spi.h"
#include "stdio.h"
#include <string.h>
//#include "main.h"

// 写寄存器
void RC522_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = { (reg << 1) & 0x7E, value };
    HAL_GPIO_WritePin(RC522_NSS_GPIO_Port, RC522_NSS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_GPIO_Port, RC522_NSS_Pin, GPIO_PIN_SET);
}

// 读寄存器
uint8_t RC522_ReadRegister(uint8_t reg) {
    uint8_t tx_data = (uint8_t)(((reg << 1) & 0x7E) | 0x80);
    uint8_t rx_data = 0;
    HAL_GPIO_WritePin(RC522_NSS_GPIO_Port, RC522_NSS_Pin, GPIO_PIN_RESET); // 片选拉低
    HAL_SPI_Transmit(&hspi1, &tx_data, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &rx_data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_GPIO_Port, RC522_NSS_Pin, GPIO_PIN_SET);   // 片选拉高
    return rx_data;
}

// 设置寄存器位
void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = RC522_ReadRegister(reg); // 读取寄存器当前值
    RC522_WriteRegister(reg, tmp | mask);  // 设置指定位
}

// 清除寄存器位
void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp & (~mask));
}


// 打开天线
void PcdAntennaOn(void) 
{
     unsigned char i;
    i = RC522_ReadRegister(TxControlReg);
    if (!(i & 0x03))
    {
        RC522_SetBitMask(TxControlReg, 0x03);
    }
}

// 关闭天线
void PcdAntennaOff(void) {
    RC522_ClearBitMask(TxControlReg, 0x03);
}


////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
    unsigned char i,n;
    RC522_ClearBitMask(DivIrqReg,0x04);
    RC522_WriteRegister(CommandReg,PCD_IDLE);
    RC522_SetBitMask(FIFOLevelReg,0x80);
    for (i=0; i<len; i++)
    {   RC522_WriteRegister(FIFODataReg, *(pIndata+i));   }
    RC522_WriteRegister(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = RC522_ReadRegister(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));
    pOutData[0] = RC522_ReadRegister(CRCResultRegL);
    pOutData[1] = RC522_ReadRegister(CRCResultRegH);
}



// 初始化 RC522
void RC522_Init(void) {
    
    HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    
    RC522_WriteRegister(CommandReg, PCD_RESETPHASE); // 复位
    HAL_Delay(1);

    // 配置基础寄存器
    RC522_WriteRegister(TModeReg, 0x8D);  // 配置时钟源，调制和时钟预分频器
    RC522_WriteRegister(TPrescalerReg, 0x3E);  // 时钟预分频器设置
    RC522_WriteRegister(TReloadRegL, 30);   // 设置定时器重载值
    RC522_WriteRegister(TReloadRegH, 0);    // 设置定时器重载值高字节
    
    RC522_WriteRegister(TxASKReg, 0x40);     // 天线驱动
    RC522_WriteRegister(ModeReg, 0x3D);     // 设置调制模式
    RC522_WriteRegister(RFCfgReg, 0x70);   // 最高灵敏度48dB
  
    RC522_WriteRegister(RxThresholdReg, 0x55);   // 降低接收门限，增加灵敏度
    
    // 打开天线
    RC522_SetBitMask(TxControlReg, 0x03); // 设置寄存器低两位以打开天线

}


/////////////////////////////////////////////////////////////////////
//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字
//          pInData[IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度
//          pOutData[OUT]:接收到的卡片返回数据
//          *pOutLenBit[OUT]:返回数据的位长度
/////////////////////////////////////////////////////////////////////
//status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

unsigned char aaa = 0;

char PcdComMF522(unsigned char Command, 
                 unsigned char *pInData, 
                 unsigned char InLenByte,
                 unsigned char *pOutData, 
                 unsigned int *pOutLenBit)
{
    char status = MI_ERR;
    unsigned char irqEn   = 0x00;
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;
    switch (Command)
    {
        case PCD_AUTHENT:
			irqEn   = 0x12;
			waitFor = 0x10;
			break;
		case PCD_TRANSCEIVE:
			irqEn   = 0x77;
			waitFor = 0x30;
			break;
		default:
			break;
    }
   
    //RC522_WriteRegister(ComIEnReg,irqEn|0x80);
    RC522_ClearBitMask(ComIrqReg,0x80);
    RC522_WriteRegister(CommandReg,PCD_IDLE);
    RC522_SetBitMask(FIFOLevelReg,0x80);
    
    for (i=0; i<InLenByte; i++)
    {   
		RC522_WriteRegister(FIFODataReg, pInData[i]);    
	}
    RC522_WriteRegister(CommandReg, Command);
   
    if (Command == PCD_TRANSCEIVE)
    {    
		RC522_SetBitMask(BitFramingReg,0x80);  
	}
    
    //i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
	i = 2000;
    do  
    {
        n = RC522_ReadRegister(ComIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitFor));
    RC522_ClearBitMask(BitFramingReg,0x80);

    if (i!=0)
    {   
		aaa = RC522_ReadRegister(ErrorReg);
		
        if(!(RC522_ReadRegister(ErrorReg)&0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {   status = MI_NOTAGERR;   }
            if (Command == PCD_TRANSCEIVE)
            {
               	n = RC522_ReadRegister(FIFOLevelReg);
              	lastBits = RC522_ReadRegister(ControlReg) & 0x07;
                if (lastBits)
                {   
					*pOutLenBit = (n-1)*8 + lastBits;   
				}
                else
                {   
					*pOutLenBit = n*8;   
				}
                if (n == 0)
                {   
					n = 1;    
				}
                if (n > MAXRLEN)
                {   
					n = MAXRLEN;   
				}
                for (i=0; i<n; i++)
                {   
					pOutData[i] = RC522_ReadRegister(FIFODataReg);    
				}
            }
        }
        else
        {   
			status = MI_ERR;   
		}
        
    }
   
    RC522_SetBitMask(ControlReg,0x80);           // stop timer now
    RC522_WriteRegister(CommandReg,PCD_IDLE); 
    return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
	char status;  
	unsigned int unLen;
	unsigned char ucComMF522Buf[MAXRLEN]; 

	RC522_ClearBitMask(Status2Reg,0x08);
	RC522_WriteRegister(BitFramingReg,0x07);
	RC522_SetBitMask(TxControlReg,0x03);
 
	ucComMF522Buf[0] = req_code;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
	if ((status == MI_OK) && (unLen == 0x10))
	{    
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
	}
	else
	{   
		status = MI_ERR;   
	}
   
	return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(unsigned char *pSnr, unsigned char anticollision_level)
{
    char status;
    unsigned char i,snr_check=0;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    

    RC522_ClearBitMask(Status2Reg,0x08);
    RC522_WriteRegister(BitFramingReg,0x00);
    RC522_ClearBitMask(CollReg,0x80);
 
    ucComMF522Buf[0] = anticollision_level;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

    if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{   
			*(pSnr+i)  = ucComMF522Buf[i];
			snr_check ^= ucComMF522Buf[i];
		}
		if (snr_check != ucComMF522Buf[i])
   		{   
			status = MI_ERR;    
		}
    }
    
    RC522_SetBitMask(CollReg,0x80);
    return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明: pSnr[IN]:卡片序列号，4字节
//返    回: 成功返回MI_OK
////////////////////////////////////////////////////////////////////
char PcdSelect (unsigned char * pSnr/*, unsigned char *sak*/)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);                                                                      
  
    RC522_ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   
		//*sak = ucComMF522Buf[0];
		status = MI_OK;  
	}
    else
    {   
		status = MI_ERR;    
	}

    return status;
}

char PcdSelect1 (unsigned char * pSnr, unsigned char *sak)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    RC522_ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   
		*sak = ucComMF522Buf[0];
		status = MI_OK;  
	}
    else
    {   
		status = MI_ERR;    
	}

    return status;
}

char PcdSelect2 (unsigned char * pSnr, unsigned char *sak)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL2;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    RC522_ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   
		*sak = ucComMF522Buf[0];
		status = MI_OK;  
	}
    else
    {   
		status = MI_ERR;    
	}

    return status;
}

char PcdSelect3 (unsigned char * pSnr, unsigned char *sak)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL2;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    RC522_ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   
		*sak = ucComMF522Buf[0];
		status = MI_OK;  
	}
    else
    {   
		status = MI_ERR;    
	}

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥 
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
	memcpy(&ucComMF522Buf[2], pKey, 6); 
	memcpy(&ucComMF522Buf[8], pSnr, 6); 
    
    status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (!(RC522_ReadRegister(Status2Reg) & 0x08)))
    {
		status = MI_ERR;   
	}
    
    return status;
}



/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          pData[OUT]：读出的数据，16字节
//返    回: 成功返回MI_OK
///////////////////////////////////////////////////////////////////// 
char PcdRead(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
   
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if ((status == MI_OK) && (unLen == 0x90))
   	{   
//		memcpy(pData, ucComMF522Buf, 16);
        memcpy(pData, ucComMF522Buf, 1);        
	}
    else
    {   
		status = MI_ERR;   
	}
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          pData[IN]：写入的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(unsigned char addr,unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   
		status = MI_ERR;   
	}
        
    if (status == MI_OK)
    {
        memcpy(ucComMF522Buf, pData, 16);
        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   
			status = MI_ERR;   
		}
    }
    
    return status;
}

void PCD_SI522A_TypeA_Init(void)
{
	RC522_ClearBitMask(Status2Reg, 0x08);  
	// Reset baud rates
	RC522_WriteRegister(TxModeReg, 0x00);
	RC522_WriteRegister(RxModeReg, 0x00);
	// Reset ModWidthReg
	RC522_WriteRegister(ModWidthReg, 0x26);
	// RxGain:110,43dB by default;
	RC522_WriteRegister(RFCfgReg, RFCfgReg_Val);
	// When communicating with a PICC we need a timeout if something goes wrong.
	// f_timer = 13.56 MHz / (2*TPreScaler+1) where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo].
	// TPrescaler_Hi are the four low bits in TModeReg. TPrescaler_Lo is TPrescalerReg.
	RC522_WriteRegister(TModeReg, 0x80);// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	RC522_WriteRegister(TPrescalerReg, 0xa9);// TPreScaler = TModeReg[3..0]:TPrescalerReg
	RC522_WriteRegister(TReloadRegH, 0x03); // Reload timer 
	RC522_WriteRegister(TReloadRegL, 0xe8); // Reload timer 
	RC522_WriteRegister(TxASKReg, 0x40);	// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	RC522_WriteRegister(ModeReg, 0x3D);	// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
	RC522_WriteRegister(CommandReg, 0x00);  // Turn on the analog part of receiver   

	PcdAntennaOn();
}

void PCD_SI522A_TypeA(void)
{
	while(1)
	{
		PCD_SI522A_TypeA_GetUID();
		
		//PCD_SI522A_TypeA_rw_block();
	
		HAL_Delay(1000);
	}
}

char PCD_SI522A_TypeA_GetUID(void)
{
	unsigned char ATQA[2];
	unsigned char UID[12];
	unsigned char SAK = 0;
	unsigned char UID_complate1 = 0;
	unsigned char UID_complate2 = 0;

	RC522_WriteRegister(RFCfgReg, RFCfgReg_Val); //复位接收增益
	
	//寻卡
	if( PcdRequest( PICC_REQIDL, ATQA) != MI_OK )  //寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		RC522_WriteRegister(RFCfgReg, 0x48);
		if(PcdRequest( PICC_REQIDL, ATQA) != MI_OK)
		{
			RC522_WriteRegister(RFCfgReg, 0x58);
			if(PcdRequest( PICC_REQIDL, ATQA) != MI_OK)
			{	
				return 1;
			}
			
	}
	
	
//UID长度=4
	//Anticoll 冲突检测 level1
	if(PcdAnticoll(UID, PICC_ANTICOLL1)!= MI_OK) 
	{
//		printf("\r\nAnticoll1:fail");
		return 1;		
	}
	else
	{
		if(PcdSelect1(UID,&SAK)!= MI_OK)
		{
			return 1;		
		}
		else
		{
			if(SAK&0x04)                         
			{
				UID_complate1 = 0;
				
				//UID长度=7
				if(UID_complate1 == 0)    
				{
					//Anticoll 冲突检测 level2
					if(PcdAnticoll(UID+4, PICC_ANTICOLL2)!= MI_OK) 
					{
						return 1;		
					}
					else
					{
						if(PcdSelect2(UID+4,&SAK)!= MI_OK)  
						{
							return 1;		
						}
						else
						{
							if(SAK&0x04)                         
							{
								UID_complate2 = 0;
								
								//UID长度=10
								if(UID_complate2 == 0)     
								{
									//Anticoll 冲突检测 level3
									if(PcdAnticoll(UID+8, PICC_ANTICOLL3)!= MI_OK) 
									{
										return 1;		
									}
									else
									{
										if(PcdSelect3(UID+8,&SAK)!= MI_OK)  
										{
											return 1;		
										}
						
									}
								}
							}
							else 
							{
								UID_complate2 = 1;                  
							}	
						}			
					}
				}
			}
			else 
			{
				UID_complate1 = 1;                   
			}
		}		
	}

  }	
	HAL_Delay(1);
	return 0;
}

char PCD_SI522A_TypeA_rw_block(void)
{
	unsigned char ATQA[2];
	unsigned char UID[12];
	unsigned char SAK = 0;
	unsigned char CardReadBuf[16] = {0};
	unsigned char CardWriteBuf[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	unsigned char DefaultKeyABuf[10] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	//printf("\r\n\r\nTest_Si522_GetCard");
	
	//request 寻卡
	if( PcdRequest( PICC_REQIDL, ATQA) != MI_OK )  //寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		//printf("\r\nRequest:fail");
		return 1;		
	}
	else
	{
		//printf("\r\nRequest:ok  ATQA:%02x %02x",ATQA[0],ATQA[1]);
	}
	

	//Anticoll 冲突检测
	if(PcdAnticoll(UID, PICC_ANTICOLL1)!= MI_OK)
	{		
		//printf("\r\nAnticoll:fail");
		return 1;		
	}
	else
	{	
		//printf("\r\nAnticoll:ok  UID:%02x %02x %02x %02x",UID[0],UID[1],UID[2],UID[3]);
	}
	
	//Select 选卡
	if(PcdSelect1(UID,&SAK)!= MI_OK)
	{
		//printf("\r\nSelect:fail");
		return 1;		
	}
	else
	{
		//printf("\r\nSelect:ok  SAK:%02x",SAK);
	}

	//Authenticate 验证密码
	if(PcdAuthState( PICC_AUTHENT1A, 4, DefaultKeyABuf, UID ) != MI_OK )
	{
		//printf("\r\nAuthenticate:fail");
		return 1;		
	}
	else
	{
		//printf("\r\nAuthenticate:ok");
	}

	//读BLOCK原始数据
	if( PcdRead( 4, CardReadBuf ) != MI_OK )
	{
		//printf("\r\nPcdRead:fail");
		return 1;		
	}
	else
	{
		//printf("\r\nPcdRead:ok  ");
		for(unsigned char i=0;i<16;i++)
		{
			//printf(" %02x",CardReadBuf[i]);
		}
	}

	//产生随机数
	for(unsigned char i=0;i<16;i++)
//		CardWriteBuf[i] = rand();             //!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	//写BLOCK 写入新的数据
	if( PcdWrite( 4, CardWriteBuf ) != MI_OK )
	{
		//printf("\r\nPcdWrite:fail");
		return 1;	
	}
	else
	{
		//printf("\r\nPcdWrite:ok  ");
		for(unsigned char i=0;i<16;i++)
		{
			//printf(" %02x",CardWriteBuf[i]);
		}
	}
		
	//读BLOCK 读出新写入的数据
	if( PcdRead( 4, CardReadBuf ) != MI_OK )
	{
		//printf("\r\nPcdRead:fail");
		return 1;		
	}
	else
	{
		//printf("\r\nPcdRead:ok  ");
		for(unsigned char i=0;i<16;i++)
		{
			//printf(" %02x",CardReadBuf[i]);
		}
	}
		
//	//Halt
//	if(PcdHalt() != MI_OK)
//	{
//		//printf("\r\nHalt:fail");
//		return 1;		
//	}
//	else
//	{
//		//printf("\r\nHalt:ok");
//	}	
	
	return 0;
}

//***********************************//修改新增内容

/*
 * 函数名：PcdReset
 * 描述  ：复位RC522 
 * 输入  ：无
 * 返回  : 无
 * 调用  ：外部调用
 */
void PcdReset ( void )
{
	//hard reset
//	HAL_GPIO_WritePin(S52_NRSTPD_GPIO_Port,S52_NRSTPD_Pin,GPIO_PIN_RESET);
//	delay_us(100);
//	HAL_GPIO_WritePin(S52_NRSTPD_GPIO_Port,S52_NRSTPD_Pin,GPIO_PIN_SET);
//	delay_us(100);
	
	RC522_WriteRegister(CommandReg, 0x0f);			//向CommandReg 写入 0x0f	作用是使RC522复位
	while(RC522_ReadRegister(CommandReg) & 0x10 );	//Powerdown位为0时，表示RC522已准备好
	HAL_Delay(1);
}

void Pcd_Hard_Reset(void)
{
    HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
}




void I_SI522A_SiModifyReg(unsigned char RegAddr, unsigned char ModifyVal, unsigned char MaskByte)
{
	unsigned char RegVal;
	RegVal = RC522_ReadRegister(RegAddr);
	if(ModifyVal)
	{
			RegVal |= MaskByte;
	}
	else
	{
			RegVal &= (~MaskByte);
	}
	RC522_WriteRegister(RegAddr, RegVal);
}




//等待卡离开
void WaitCardOff(void)
{
	char          status;
  unsigned char	TagType[2];
    
	while(1)
	{
		status = PcdRequest(REQ_ALL, TagType);
		if(status)
		{
			status = PcdRequest(REQ_ALL, TagType);
			if(status)
			{
				status = PcdRequest(REQ_ALL, TagType);
				if(status)
				{
                    return;
				}
			}
		}
    HAL_Delay(50);
	}
}


char PCD_IRQ(void)
{
	unsigned char status_Si522ACD_IRQ;  
	unsigned char temp_Si522ACD_IRQ; 
	
	temp_Si522ACD_IRQ = RC522_ReadRegister(DivIrqReg);   
	if	( temp_Si522ACD_IRQ & 0x40)	//ACD中断
	{
		RC522_WriteRegister(DivIrqReg, 0x40);		//Clear ACDIRq 
		
		status_Si522ACD_IRQ =1;
		return status_Si522ACD_IRQ;
	}
	
	if ( temp_Si522ACD_IRQ & 0x20)	//ACD看门狗中断
	{
		RC522_WriteRegister(DivIrqReg, 0x20);		//Clear ACDTIMER_IRQ
		
		status_Si522ACD_IRQ = 2;
		return status_Si522ACD_IRQ;
	}
	
	RC522_WriteRegister(DivIrqReg, 0x40);		//Clear ACDIRq
	RC522_WriteRegister(DivIrqReg, 0x20);		//Clear ACDTIMER_IRQ
	RC522_WriteRegister(0x20, (0x0f << 2) | 0x40);		
	RC522_WriteRegister(0x0f, 0x0a);	//Clear OSCMon_IRQ,RFLowDetect_IRQ
	RC522_WriteRegister(0x20, (0x09 << 2) | 0x40);		
	RC522_WriteRegister(0x0f, 0x55);	//Clear ACC_IRQ

	return status_Si522ACD_IRQ = 0;

}




//extern uint8_t PCD_IRQ_flagA ;
uint8_t PCD_IRQ_flagA = 0 ;
unsigned char ACDConfigRegK_Val ;
unsigned char ACDConfigRegC_Val ;


void ACD_Fun(void)
{
//	EXTI->IMR |= 0x00000008;	// Enable external interrupt
    EXTI->IMR1 |= (1 << 3); 
	PCD_IRQ_flagA = 0;	        //clear IRQ flag
	
	while(1)
	{	
		if(PCD_IRQ_flagA)
		{
			//printf("\r\n\r\n\r\n PCD_IRQ_flagA");
//			EXTI->IMR &= 0xFFFFFFF7;		// Disable external interrupt			
            EXTI->IMR1 &= ~(1 << 3);        // Enable EXTI3 interrupt
			switch( PCD_IRQ() )
			{
				case 0:	//Other_IRQ 			
					////printf("Other IRQ Occur\r\n");
					PCD_SI522A_TypeA_GetUID();
					PcdReset();			//软复位				
					//PcdReset();				//硬复位
					PCD_SI522A_TypeA_Init();
					PCD_ACD_Init();
					break;
						
				case 1:	//ACD_IRQ
					I_SI522A_SiModifyReg(0x01, 0, 0x20);	// Turn on the analog part of receiver 			
					PCD_SI522A_TypeA_GetUID();
					//RC522_WriteRegister(ComIEnReg, 0x80);		//复位02寄存器,在读卡函数中被改动
					RC522_WriteRegister(CommandReg, 0xb0);	 	//进入软掉电,重新进入ACD（ALPPL）
				break;
				
				case 2:	//ACDTIMER_IRQ			
					//printf("ACDTIMER_IRQ:Reconfigure the register \r\n");
					//PcdReset();			//软复位
					Pcd_Hard_Reset();		//硬复位
					PCD_SI522A_TypeA_Init();
					PCD_ACD_Init();
					break;
				
			}		
			
//			EXTI->IMR |= 0x00000008;		// Enable external interrupt
            EXTI->IMR1 |= (1 << 3); 
			PCD_IRQ_flagA = 0;	
		}
		else
		{
			HAL_Delay(500);
		}
	}
}




/********************************************************
*PCD_ACD_AutoCalc(void) 函数主要为了上电后，自动计算并选择ADC的基准电压和合适的增益放大。
*1.第一个VOCN数组是为了从最大基准电压开始轮询查，并将VCON值写入0F_K寄存器的bit[3:0],然后读取0F_G寄存器，计算100次的平均值，直到找到非0的最小值。
*2. 0F_K寄存器在第一步获取的VCON值基础上，或上TR增益控制位bit[6:5]，然后将换算的值写入0F_K寄存器，读取0F_G,获取最大非超7F阈值的增益控制位
*3. 将获取的VCON和TR，写入0F_G。
*4. 关于0F_K寄存器的定义请参考 《ACD初始化代码解析》文档
********************************************************/
void PCD_ACD_AutoCalc(void)
{
	unsigned char temp; 
	unsigned char temp_Compare=0; 
	unsigned char VCON_TR[8]={ 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};//acd灵敏度调节
	unsigned char TR_Compare[4]={ 0x00, 0x00, 0x00, 0x00};
	ACDConfigRegC_Val = 0x7f;
	unsigned char	ACDConfigRegK_RealVal = 0;
	RC522_WriteRegister(TxControlReg, 0x83);	//打开天线
	RC522_SetBitMask(CommandReg, 0x06);	//开启ADC_EXCUTE 
	HAL_Delay(1);
	
	for(int i=7; i>0; i--)
	{	
		RC522_WriteRegister(ACDConfigSelReg, (ACDConfigK << 2) | 0x40);		
		RC522_WriteRegister(ACDConfigReg, VCON_TR[i]);
		
		RC522_WriteRegister(ACDConfigSelReg, (ACDConfigG << 2) | 0x40);
		temp_Compare = RC522_ReadRegister(ACDConfigReg);
		for(int m=0;m<100;m++)
		{
			RC522_WriteRegister(ACDConfigSelReg, (ACDConfigG << 2) | 0x40);		
			temp = RC522_ReadRegister(ACDConfigReg);
			
			if(	temp	==	0) 	break;          //处在接近的VCON值附近值，如果偶合出现0值，均有概率误触发，应舍弃该值。
				
			temp_Compare=(temp_Compare+temp)/2;		
			HAL_Delay(1);
		}		
		
		if(temp_Compare == 0 || temp_Compare == 0x7f) //比较当前值和所存值
		{

		}
		else
		{
			if(temp_Compare < ACDConfigRegC_Val)
			{
				ACDConfigRegC_Val = temp_Compare;
				ACDConfigRegK_Val = VCON_TR[i];
			}
		}
	}
	ACDConfigRegK_RealVal	=	ACDConfigRegK_Val;     //取得最接近的参考电压VCON
	
	
	for(int j=0; j<4; j++)
	{
		RC522_WriteRegister(ACDConfigSelReg, (ACDConfigK << 2) | 0x40);		
		RC522_WriteRegister(ACDConfigReg, j*32+ACDConfigRegK_Val);
		
		RC522_WriteRegister(ACDConfigSelReg, (ACDConfigG << 2) | 0x40);
		temp_Compare = RC522_ReadRegister(ACDConfigReg);
		for(int n=0;n<100;n++)
		{
			RC522_WriteRegister(ACDConfigSelReg, (ACDConfigG << 2) | 0x40);		
			temp = RC522_ReadRegister(ACDConfigReg);
			temp_Compare=(temp_Compare+temp)/2;		
			HAL_Delay(1);
		}		
		TR_Compare[j] = temp_Compare;
	}//再调TR的档位，将采集值填入TR_Compare[]

	for(int z=0; z<3; z++)                   //TR有四档可调，但是最大档的时候，电源有抖动，可能会导致ADC的值抖动较大，造成误触发
	{
		if(TR_Compare[z] ==0x7F)         
		{
			
		}
		else
		{
			ACDConfigRegC_Val = TR_Compare[z];//最终选择的配置
			ACDConfigRegK_Val = ACDConfigRegK_RealVal + z*32;
		}
	}//再选出一个非7f大值
	
	
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigK << 2) | 0x40);
//	//printf("\r\n ACDConfigRegK_Val:%02x ",ACDConfigRegK_Val);	
	
	RC522_SetBitMask(CommandReg, 0x06);		//关闭ADC_EXCUTE
}



void PCD_ACD_Init(void)
{
	RC522_WriteRegister(DivIrqReg, 0x60);	//清中断，该处不清中断，进入ACD模式后会异常产生有卡中断。
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigJ << 2) | 0x40);		
	RC522_WriteRegister(ACDConfigReg, 0x55);	//Clear ACC_IRQ
	
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigA << 2) | 0x40);          //设置轮询时间
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegA_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigB << 2) | 0x40);					//设置相对模式或者绝对模式
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegB_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigC << 2) | 0x40);					//设置无卡场强值
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegC_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigD << 2) | 0x40);					//设置灵敏度，一般建议为4，在调试时，可以适当降低验证该值，验证ACD功能
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegD_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigH << 2) | 0x40);					//设置看门狗定时器时间
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegH_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigI << 2) | 0x40);         //设置ARI功能，在天线场强打开前1us产生ARI电平控制触摸芯片Si12T的硬件屏蔽引脚SCT
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegI_Val );	
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigK << 2) | 0x40);					//设置ADC的基准电压和放大增益
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegK_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigM << 2) | 0x40);					//设置监测ACD功能是否产生场强，意外产生可能导致读卡芯片复位或者寄存器丢失
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegM_Val );
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigO << 2) | 0x40);					//设置ACD模式下相关功能的标志位传导到IRQ引脚
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegO_Val );
	
	RC522_WriteRegister(ComIEnReg, ComIEnReg_Val);												//ComIEnReg，DivIEnReg   设置IRQ选择上升沿或者下降沿
	RC522_WriteRegister(DivIEnReg, DivIEnReg_Val);												
	
	RC522_WriteRegister(ACDConfigSelReg, (ACDConfigJ << 2) | 0x40);				//设置监测ACD功能下的重要寄存器的配置值，寄存器丢失后会立即产生中断
	RC522_WriteRegister(ACDConfigReg, ACDConfigRegJ_Val );								// 写非0x55的值即开启功能，写0x55清除使能停止功能。
	
	RC522_WriteRegister(CommandReg, 0xb0);	//进入ACD
}



void ACD_init_Fun(void)
{
	PCD_SI522A_TypeA_Init();	//Reader模式的初始化
	
	PCD_ACD_AutoCalc(); //自动获取阈值
	
	PCD_ACD_Init();   //初始化ACD配置寄存器，并且进入ACD模式
}



//读卡
uint8_t wait_flag = 0;

uint8_t get_card(void)
{
   char status;
   unsigned char snr,  TagType[2], card_buf,SelectedSnr[4], DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  
   RC522_Init();
   status= PcdRequest(REQ_ALL,TagType);
    if(!status)
		{
			status = PcdAnticoll(SelectedSnr,PICC_ANTICOLL1);
			if(!status)
			{
				status=PcdSelect(SelectedSnr);
				if(!status)
				{
					snr = 5;  //扇区号5
                    status = PcdAuthState(KEYA, (snr*4+3), DefaultKey, SelectedSnr);// 校验1扇区密码，密码位于每一扇区第3块
					{
						if(!status)
						{
							status = PcdRead((snr*4+2), &card_buf);         // 读卡，读取5扇区第2块数据到buf[0]-buf[16] 
							if(!status)
							{
                                if (card_buf)
                                {
//								  WaitCardOff();
                                  wait_flag = 1;
                                  return card_buf;  
                                }
                              
							}
						}
					}
				}
			}
		}
       HAL_GPIO_WritePin(RC522_RST_GPIO_Port,RC522_RST_Pin,GPIO_PIN_RESET);    //rc522休眠
		    //等待卡片离开,防止抖动重复刷卡
    if(wait_flag)
    {
        WaitCardOff();  
			  wait_flag = 0;
    }
        return 0;
}




