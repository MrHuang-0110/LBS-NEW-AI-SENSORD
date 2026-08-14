#ifndef __DRIVER_SYS_H
#define __DRIVER_SYS_H

#include "stm32g0xx.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

__weak void sys_free(void *ptr);
__weak void *sys_malloc(uint32_t size);

#endif
