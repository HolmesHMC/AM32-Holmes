/*
 * comparator.c
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#include "comparator.h"
#include "common.h"
#include "isr_hotdata.h"
#include "targets.h"

COMP_TypeDef* active_COMP = COMP1;

uint8_t getCompOutputLevel() { return LL_COMP_ReadOutputLevel(active_COMP); }

void maskPhaseInterrupts()
{
  EXTI->IMR &= ~(1 << 21);
  EXTI->PR = EXTI_LINE;
}

void enableCompInterrupts() { EXTI->IMR |= (1 << 21); }

void changeCompInput()
{

if((isr_hot.average_interval < 400)){
COMP->CSR = COMP->CSR & ~(1<<2);
}else{
COMP->CSR  = COMP->CSR | 1<<2;
}
  EXTI->RTSR = !isr_hot.rising << 21;
  EXTI->FTSR = isr_hot.rising << 21;
}
