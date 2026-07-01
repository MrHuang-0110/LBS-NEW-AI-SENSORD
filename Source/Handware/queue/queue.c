#include "queue.h"


QUEUE_DATA_TYPE  node_data[QUEUE_NODE_NUM]; 
QueueBuffer rx_queue;
char node_buff[QUEUE_NODE_NUM][QUEUE_NODE_DATA_LEN]__attribute__((aligned(4)));

void cbInit(QueueBuffer *cb, int size) 
{
    cb->size  = size;	  /* maximum number of elements           */
    cb->read = 0; 		  /* index of oldest element              */
    cb->write   = 0; 	 	/* index at which to write new element  */
//    cb->elems = (uint8_t *)calloc(cb->size, sizeof(uint8_t));  //elems 要额外初始化
}
 

int cbIsFull(QueueBuffer *cb) 
{
    return cb->write == (cb->read ^ cb->size); /* This inverts the most significant bit of read before comparison */ 
}

int cbIsEmpty(QueueBuffer *cb) 
{
    return cb->write == cb->read; 
}

int cbIncr(QueueBuffer *cb, int p) 
{
    return (p + 1)&(2*cb->size-1);
}
 

QUEUE_DATA_TYPE* cbWrite(QueueBuffer *cb) 
{
    if (cbIsFull(cb)) 
    {
			return NULL;
		}		
		else
		{
			if(cb->write == cb->write_using)
			{
				cb->write_using = cbIncr(cb, cb->write); //未满，则增加1
			}
		}
		
	return  cb->elems[cb->write_using&(cb->size-1)];
}


void cbWriteFinish(QueueBuffer *cb)
{
    cb->write = cb->write_using;
}
 

QUEUE_DATA_TYPE* cbRead(QueueBuffer *cb) 
{
		if(cbIsEmpty(cb))
			return NULL;

	if(cb->read == cb->read_using)	
		cb->read_using = cbIncr(cb, cb->read);
	
	return cb->elems[cb->read_using&(cb->size-1)];
}



void cbReadFinish(QueueBuffer *cb) 
{	
		cb->elems[cb->read_using&(cb->size-1)]->len = 0;
    cb->read = cb->read_using;
}



//队列的指针指向的缓冲区全部销毁
void camera_queue_free(void)
{
    uint32_t i = 0;

    for(i = 0; i < QUEUE_NODE_NUM; i ++)
    {
        if(node_data[i].head != NULL)
        {
					//若是动态申请的空间才要free
//            free(node_data[i].head);
            node_data[i].head = NULL;
        }
    }

    return;
}



void rx_queue_init(void)
{
  uint32_t i = 0;

  memset(node_data, 0, sizeof(node_data));
		 
	cbInit(&rx_queue,QUEUE_NODE_NUM);

    for(i = 0; i < QUEUE_NODE_NUM; i ++)
    {
        node_data[i].head = node_buff[i];
        
        rx_queue.elems[i] = &node_data[i];
        
        memset(node_data[i].head, 0, QUEUE_NODE_DATA_LEN);
    }	
}

void push_data_to_queue(char *src_dat,uint16_t src_len)
{
	QUEUE_DATA_TYPE *data_p;
	uint8_t i;
	
	for(i=0;i<src_len;i++)
	{
		data_p = cbWrite(&rx_queue);
		
		if (data_p != NULL)	
		{		
			*(data_p->head + i) = src_dat[i];
				data_p->len++;
		}else return;	
	}	
	cbWriteFinish(&rx_queue);
}


 
