#ifndef __DATA_ANALYSIS_H
#define __DATA_ANALYSIS_H

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

 

#define TX_CACHE_SIZE 32 * sizeof(uint8_t)
#define RX_CACHE_SIZE 32 * sizeof(uint8_t)
typedef struct
{ 
	 uint8_t *tx_cache;
	 uint8_t *rx_cache;
   void (*wrtieMoreData)(uint8_t*,uint16_t);
}_DATA_ANALYSIS;

uint8_t __GetCRC(uint8_t *bufer,uint16_t Len);
void init_analysis(void (*wrtieMoreData)(uint8_t*,uint16_t));
void SendCOMdata(uint8_t id,char *data,uint16_t len,uint8_t Index);
void SendCOMdataByte(uint8_t id,uint8_t *data,uint16_t len,uint8_t Index);
#endif
