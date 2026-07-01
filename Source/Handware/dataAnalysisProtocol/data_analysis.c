#include "data_analysis.h"
#include "senords.h"
static _DATA_ANALYSIS _port_;


__attribute__((weak)) void *data_malloc(size_t size)
{ 
    return malloc(size);
}

/*校验和计算 取低8位*/
uint8_t __GetCRC(uint8_t *bufer,uint16_t Len)
{ 
   uint16_t i = 0,myCRC = 0;
	 for(i = 0;i<Len;i++)
	{ 
	  myCRC+=bufer[i];
	}
	return (myCRC&0xFF);
}

void SendCOMdata(uint8_t id,char *data,uint16_t len,uint8_t Index)
{ 
	uint8_t Loop = 0,mycrc = 0;

 
	memset(_port_.tx_cache,0,TX_CACHE_SIZE);

	_port_.tx_cache[0] = 0x5A;
	_port_.tx_cache[1] = USER_ObjectID;
	_port_.tx_cache[2] = id;
	_port_.tx_cache[3] = len;
	_port_.tx_cache[4] = Index;
	
  strcpy((char*)&_port_.tx_cache[5],data);
	
	for(Loop = 0;Loop<(_port_.tx_cache[3]+7)-2;Loop++)
	{ 
	   mycrc+=_port_.tx_cache[Loop];
	}
	
	mycrc&=0xFF;

	_port_.tx_cache[(_port_.tx_cache[3]+7)-2]= mycrc;
	_port_.tx_cache[(_port_.tx_cache[3]+7)-1] = 0xA5;
  _port_.wrtieMoreData(_port_.tx_cache,_port_.tx_cache[3]+7);
}

void SendCOMdataByte(uint8_t id,uint8_t *data,uint16_t len,uint8_t Index)
{ 
	uint8_t Loop = 0;
	uint32_t mycrc = 0;

 
	memset(_port_.tx_cache,0,TX_CACHE_SIZE);

	_port_.tx_cache[0] = 0x5A;
	_port_.tx_cache[1] = USER_ObjectID;
	_port_.tx_cache[2] = id;
	_port_.tx_cache[3] = len;
	_port_.tx_cache[4] = Index;
	
		for(uint16_t i = 0;i<len;i++)
	 { 
	   _port_.tx_cache[5+i] = data[i];
	 }
	 
 
	for(Loop = 0;Loop<(_port_.tx_cache[3]+7)-2;Loop++)
	{ 
	   mycrc+=_port_.tx_cache[Loop];
	}
	
	mycrc&=0xFF;

	_port_.tx_cache[(_port_.tx_cache[3]+7)-2]= mycrc;
	_port_.tx_cache[(_port_.tx_cache[3]+7)-1] = 0xA5;
  _port_.wrtieMoreData(_port_.tx_cache,_port_.tx_cache[3]+7);
}


void init_analysis(void (*wrtieMoreData)(uint8_t*,uint16_t))
{ 
   _port_.rx_cache = data_malloc(RX_CACHE_SIZE);
	
	 if(_port_.rx_cache!=NULL)
		  memset(_port_.rx_cache,0,RX_CACHE_SIZE);
	 
	 _port_.tx_cache = data_malloc(TX_CACHE_SIZE);
	 if(_port_.tx_cache!=NULL)
		  memset(_port_.tx_cache,0,TX_CACHE_SIZE);
	 
	 
	 _port_.wrtieMoreData = wrtieMoreData;
}
