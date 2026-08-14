#ifndef __DATA_ANALYSIS_H
#define __DATA_ANALYSIS_H

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "senords.h"   /* USER_SourceID/USER_ObjectID 由产品宏 NFC_G030F6=1 决定 */

#define TX_CACHE_SIZE 32 * sizeof(uint8_t)
#define RX_CACHE_SIZE 32 * sizeof(uint8_t)
typedef struct
{ 
	 uint8_t *tx_cache;
	 uint8_t *rx_cache;
   void (*wrtieMoreData)(uint8_t*,uint8_t);
}_DATA_ANALYSIS;

uint8_t __GetCRC(uint8_t *bufer,uint16_t Len);
void init_analysis(void);
void SendCOMdata(uint8_t id,char *data,uint8_t len,uint8_t Index);
#endif
