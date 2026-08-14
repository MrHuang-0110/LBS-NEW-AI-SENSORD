#include "driver_sys.h"
 
__weak void *sys_malloc(uint32_t size)
{ 
   return malloc(size);
}

__weak void sys_free(void *ptr)
{ 
   free(ptr);
}
