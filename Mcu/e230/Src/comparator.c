/*
 * comparator.c
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#include "comparator.h"
#include "isr_hotdata.h"  /* Refactor 3.9: HOT_ISR_FN macro */

#include "targets.h"

uint8_t getCompOutputLevel() { return cmp_output_level_get(); }

void maskPhaseInterrupts()
{
    //	EXTI->IMR &= (0 << 21);
    //	LL_EXTI_ClearFlag_0_31(EXTI_LINE);

    EXTI_INTEN &= ~(uint32_t)EXTI_LINE;
    EXTI_PD = (uint32_t)EXTI_LINE;
}

void enableCompInterrupts()
{
    //  EXTI->IMR |= (1 << 21);
    EXTI_INTEN |= (uint32_t)EXTI_LINE;
}

HOT_ISR_FN void changeCompInput()
{
    //	TIM3->CNT = 0;
    //	HAL_COMP_Stop_IT(&hcomp1);            // done in comparator interrupt
    // routine

    if (isr_hot.step == 1 || isr_hot.step == 4) { // c floating
        CMP_CS = PHASE_C_COMP;
        //	cmp_mode_init(CMP_HIGHSPEED, PHASE_C_COMP, CMP_HYSTERESIS_NO);
    }
    if (isr_hot.step == 2 || isr_hot.step == 5) { // a floating
        CMP_CS = PHASE_A_COMP;
        //	cmp_mode_init(CMP_HIGHSPEED, PHASE_A_COMP, CMP_HYSTERESIS_NO);
    }
    if (isr_hot.step == 3 || isr_hot.step == 6) { // b floating
        CMP_CS = PHASE_B_COMP;
        //		cmp_mode_init(CMP_HIGHSPEED, PHASE_B_COMP,
        // CMP_HYSTERESIS_NO);
    }
    if (isr_hot.rising) {
        //	EXTI->RTSR = 0x0;
        //	EXTI->FTSR = 0x200000;
        EXTI_RTEN &= ~(uint32_t)EXTI_LINE;
        EXTI_FTEN |= (uint32_t)EXTI_LINE;
    } else {
        // falling bemf
        //	EXTI->FTSR = 0x0;
        //	EXTI->RTSR = 0x200000;
        EXTI_RTEN |= (uint32_t)EXTI_LINE;
        EXTI_FTEN &= ~(uint32_t)EXTI_LINE;
    }
}
