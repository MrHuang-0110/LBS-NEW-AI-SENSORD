#ifndef __ESP_DATA_QUEUE_H_
#define __ESP_DATA_QUEUE_H_
 

#include "stdio.h"
#include "string.h"
#include <stdint.h>

//??????快????????2????
#define QUEUE_NODE_NUM        (4)         //??????快??妊???忪????????????
#define QUEUE_NODE_DATA_LEN   (32)       //???????????????妊

//???快???????????????
#define QUEUE_DATA_TYPE  				ESP_USART_FRAME
//???快??????????
#define DATA_QUEUE_LOG  				QUEUE_DEBUG
#define DATA_QUEUE_LOG_ARRAY 	  QUEUE_DEBUG_ARRAY

//????????
typedef struct 
{
	char  *head; 	//??????????	
	volatile uint16_t len; //??????????????

}ESP_USART_FRAME;


//???扶?
typedef struct {
	int         size;    /* ????????妊           */
	int         read;    /* ?????               */
	int         write;   /* 忱???  */
	int read_using;	     /* ???????????????? */
	int write_using;		 /* ????忱??????????? */
	QUEUE_DATA_TYPE    *elems[QUEUE_NODE_NUM];    /* ?????????                   */
} QueueBuffer;


extern QueueBuffer rx_queue;




/*??????*/
#define QUEUE_DEBUG_ON         0
#define QUEUE_DEBUG_ARRAY_ON		0

#define QUEUE_INFO(fmt,arg...)           printf("<<-QUEUE-INFO->> "fmt"\r\n",##arg)
#define QUEUE_ERROR(fmt,arg...)          printf("<<-QUEUE-ERROR->> "fmt"\r\n",##arg)
#define QUEUE_DEBUG(fmt,arg...)          do{\
                                          if(QUEUE_DEBUG_ON)\
                                          printf("<<-QUEUE-DEBUG->> [%d]"fmt"\r\n",__LINE__, ##arg);\
                                          }while(0)

#define QUEUE_DEBUG_ARRAY(array, num)    do{\
																									 int32_t i;\
																									 uint8_t* a = array;\
																									 if(QUEUE_DEBUG_ARRAY_ON)\
																									 {\
																											printf("\n<<-QUEUE-DEBUG-ARRAY->>\r\n");\
																											for (i = 0; i < (num); i++)\
																											{\
																													printf("%02x   ", (a)[i]);\
																													if ((i + 1 ) %10 == 0)\
																													{\
																															printf("\r\n");\
																													}\
																											}\
																											printf("\r\n");\
																									}\
																								 }while(0)	

//??????快??????
#define cbPrint(cb)		    DATA_QUEUE_LOG("size=0x%x, read=%d, write=%d\r\n", cb.size, cb.read, cb.write);\
	  DATA_QUEUE_LOG("size=0x%x, read_using=%d, write_using=%d\r\n", cb.size, cb.read_using, cb.write_using);


QUEUE_DATA_TYPE* cbWrite(QueueBuffer *cb);
QUEUE_DATA_TYPE* cbRead(QueueBuffer *cb);
void cbReadFinish(QueueBuffer *cb);
void cbWriteFinish(QueueBuffer *cb);
//void cbPrint(QueueBuffer *cb);
QUEUE_DATA_TYPE* cbWriteUsing(QueueBuffer *cb) ;
int cbIsFull(QueueBuffer *cb) ; 
int cbIsEmpty(QueueBuffer *cb) ;

void rx_queue_init(void);																				 
void push_data_to_queue(char *src_dat,uint16_t src_len);


#endif



