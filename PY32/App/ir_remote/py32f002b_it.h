/* py32f002b_it.h - interrupt handler prototypes */
#ifndef PY32F002B_IT_H
#define PY32F002B_IT_H

#include "main.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif /* PY32F002B_IT_H */
