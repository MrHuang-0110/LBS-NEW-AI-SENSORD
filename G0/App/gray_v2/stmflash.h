#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "string.h"
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;
typedef volatile unsigned long long vu64;
typedef unsigned int   uint32_t;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef  void (*iapfun)(void);				//定义一个函数类型的参数.
//extern void  FLASH_PageErase(uint32_t PageAddress);

#define STM32_FLASH_SIZE 	32 	 		     //所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 	1            //使能FLASH写入(0，不是能;1，使能)
#define FLASH_WAITETIME  	50000      	//FLASH等待超时时间
#define STM_SECTOR_SIZE	2048
//FLASH起始地址
#define STM32_FLASH_BASE 		0x08000000 		//STM32 FLASH的起始地址
#define AplicationAddr      STM32_FLASH_BASE + 0x3800
#define AplicationVERSION   STM32_FLASH_BASE + 0X7900
#define CFG_FILE_ADDR     0x08007800  // 最后一页的起始地址

void FLASH_unlock(void);
void FLASH_lock(void);
u8 FLASH_GetStatus(void);
u8 FLASH_wait_time(u32 times);
u8 FLASH_earse(u32 faddr);
u8 FLASH_mass_earse(void);
u8 FLASH_write_four(u32 faddr,u64 data);
u8 FLASH_write(u32 adder,u8 *pbuff,u32 para_size);
u32 FLASH_read(u32 adder);
void MSR_MSP(u32 addr);

void STMFLASH_Read(u32 ReadAddr,u64 *pBuffer,u16 NumToRead) ;
void STMFLASH_Write_NoCheck(u32 WriteAddr,u64 *pBuffer,u16 NumToWrite);
u8 iap_write_appbin(u32 appxaddr,u8 *appbuf,u32 appsize);
void Flash_Read(uint32_t address, uint32_t *buffer, uint32_t length);
void iap_load_app(u32 appxaddr);
 void reverseBytesAndStoreSimplified(uint32_t value, unsigned char *array);
#endif

















