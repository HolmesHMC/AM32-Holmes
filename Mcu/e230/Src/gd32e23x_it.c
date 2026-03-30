#include <stdint.h>
#include "isr_hotdata.h"   /* Refactor 3.1: ISR hot data struct */

extern void transfercomplete();
extern void PeriodElapsedCallback();
/* interruptRoutine() is now fully inlined in ADC_CMP_IRQHandler (refactor 4.1) */

/* Externs for inlined PeriodElapsedCallback (refactor 2.3) */
extern void commutate();
/* Externs for inlined interruptRoutine (refactor 4.1) */
extern uint8_t filter_level;
/* Refactor 3.1: isr_hot.commutation_interval, isr_hot.lastzctime, isr_hot.thiszctime, isr_hot.advance, isr_hot.waitTime
 * now accessed via isr_hot struct (declared in isr_hotdata.h)             */
extern uint8_t temp_advance;
extern uint8_t auto_advance_level;
extern uint8_t ec_auto_advance; /* Refactor 3.8: cached eepromBuffer.auto_advance */
extern volatile uint8_t transfercomplete_pending; /* Refactor 3.2 */
extern void processDshot();  /* 3.2: no longer called via EXTI_SWIEV */
extern char old_routine;
extern volatile uint32_t zero_crosses;
extern void doPWMChanges();
extern void tenKhzRoutine();
extern void sendDshotDma();
extern void receiveDshotDma();
extern void signalEdgeRoutine();
extern void processDshot();
extern volatile char send_telemetry;
extern char telemetry_done;
extern volatile char servoPwm;
#ifndef NO_DSHOT
extern volatile char dshot_telemetry;  /* 5.2: dead on NO_DSHOT targets */
#endif
extern volatile char out_put;
extern volatile char armed;

uint16_t interrupt_time = 0;

#include "gd32e23x_it.h"

#include "common.h"
#include "main.h"
#include "systick.h"
#include "targets.h"
#include "WS2812.h"
#include "peripherals.h"   // for DISABLE_COM_TIMER_INT() macro
#include "comparator.h"    // for enableCompInterrupts()

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void) { }

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while (1) {
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void) { }

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void) { }

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void) { delay_decrement(); }

void DMA_Channel3_4_IRQHandler(void)
{
    /* Read DMA_INTF once — avoids redundant peripheral reads, saves 10-15 cycles */
    register uint32_t dma_intf_reg = DMA_INTF;

    #ifdef USE_LED_STRIP
        /* Check transfer complete for WS2812 — direct register access */
        if (dma_intf_reg & DMA_FLAG_ADD(DMA_INT_FLAG_FTF, LED_DMA_CHANNEL)) {
            DMA_INTC = DMA_FLAG_ADD(DMA_INT_FLAG_G, LED_DMA_CHANNEL);  /* Write-1-to-clear */
            DMA_CHCTL(LED_DMA_CHANNEL) &= ~DMA_CHXCTL_CHEN;
            dma_busy = 0;  /* Ready for next LED update */
            return;
        }
        if (dma_intf_reg & DMA_FLAG_ADD(DMA_INT_FLAG_ERR, LED_DMA_CHANNEL)) {
            DMA_INTC = DMA_FLAG_ADD(DMA_INT_FLAG_G, LED_DMA_CHANNEL);  /* Write-1-to-clear */
            DMA_CHCTL(LED_DMA_CHANNEL) &= ~DMA_CHXCTL_CHEN;
            dma_busy = 0;  /* Ready for next LED update */
            return;
        }
    #endif

#ifndef NO_DSHOT
    /* Dead code on NO_DSHOT targets (5.2): dshot_telemetry is always 0 when
     * DShot is disabled, so the volatile read and branch are pure overhead.
     * Wrapped to eliminate them entirely at compile time.                  */
    if (__builtin_expect(dshot_telemetry && armed, 0)) {  /* Rare: normal path is standard DMA */
        DMA_INTC |= DMA_FLAG_ADD(DMA_INT_FLAG_G, INPUT_DMA_CHANNEL);
        DMA_CHCTL(INPUT_DMA_CHANNEL) &= ~DMA_CHXCTL_CHEN;
        if (out_put) {
            receiveDshotDma();
            compute_dshot_flag = 2;
        } else {
            sendDshotDma();
            compute_dshot_flag = 1;
        }
        EXTI_SWIEV |= (uint32_t)EXTI_15;
        return;
    }
#endif /* NO_DSHOT */

    /* Direct register checks for INPUT_DMA_CHANNEL — saves 12-18 cycles */
    if (dma_intf_reg & DMA_FLAG_ADD(DMA_INT_FLAG_HTF, INPUT_DMA_CHANNEL)) {
        if (servoPwm) {
            TIMER_CHCTL2(TIMER2) |= (uint32_t)(TIMER_IC_POLARITY_FALLING);
            DMA_INTC = DMA_FLAG_ADD(DMA_INT_FLAG_HTF, INPUT_DMA_CHANNEL);
        }
    }
    if (dma_intf_reg & DMA_FLAG_ADD(DMA_INT_FLAG_FTF, INPUT_DMA_CHANNEL)) {
        DMA_INTC = DMA_FLAG_ADD(DMA_INT_FLAG_G, INPUT_DMA_CHANNEL);
        DMA_CHCTL(INPUT_DMA_CHANNEL) &= ~DMA_CHXCTL_CHEN;
        transfercomplete_pending = 1; /* Refactor 3.2: defer to main loop */
    } else if (dma_intf_reg & DMA_FLAG_ADD(DMA_INT_FLAG_ERR, INPUT_DMA_CHANNEL)) {
        DMA_INTC = DMA_FLAG_ADD(DMA_INT_FLAG_G, INPUT_DMA_CHANNEL);
    }
}

/**
 * @brief ADC and COMP interrupt handler (comparator output via EXTI_21).
 *
 * Refactor 4.1+4.3: interruptRoutine() fully inlined; BEMF filter loop
 * unrolled into switch-case for filter_level = 12 / 8 / 5 / 3 with
 * fallthrough so total iterations match the original loop count.
 *
 * Direct register access replaces:
 *   exti_interrupt_flag_get()  →  EXTI_PD & EXTI_21
 *   exti_flag_clear()          →  EXTI_PD = EXTI_21  (write-1-to-clear)
 *   getCompOutputLevel()       →  (CMP_CS & CMP_CS_CMPO) != 0
 *
 * Each inline eliminates one call/return pair (10-15 cycles).
 * The unrolled switch removes loop-counter update per iteration (3-6 cyc).
 */
void ADC_CMP_IRQHandler(void)
{
    if (__builtin_expect(EXTI_PD & EXTI_21, 1)) {
        EXTI_PD = EXTI_21;   /* write-1-to-clear (replaces exti_flag_clear) */

        /* Cache rising once — one struct read beats N volatile accesses */
        register uint8_t rising_local = (uint8_t)isr_hot.rising;

        /*
         * CMP_OUT_EQ_RISING: true when comparator output state == rising_local.
         * Direct CMP_CS read replaces getCompOutputLevel() → cmp_output_level_get()
         * call chain (~10 cycles saved per check).
         * Each invocation re-reads the hardware register to correctly detect
         * transients (preserves original filter debounce semantics).
         */
#define CMP_OUT_EQ_RISING() (((CMP_CS & CMP_CS_CMPO) != 0U) == rising_local)

        /* Unrolled BEMF filter — fallthrough ensures correct iteration count:
         *   case 12: 4 + 3 + 2 + 3 = 12 checks
         *   case  8:     3 + 2 + 3 =  8 checks
         *   case  5:         2 + 3 =  5 checks
         *   case  3:             3 =  3 checks          */
        switch (filter_level) {
            case 12:
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                /* fallthrough */
            case 8:
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                /* fallthrough */
            case 5:
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                /* fallthrough */
            case 3:
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                if (CMP_OUT_EQ_RISING()) return;
                break;
            default: {
                /* Handles any filter_level not covered above */
                for (int i = 0; i < filter_level; i++) {
                    if (CMP_OUT_EQ_RISING()) return;
                }
                break;
            }
        }

#undef CMP_OUT_EQ_RISING

        /* Zero-cross confirmed — record timing and arm commutation timer */
        __disable_irq();
        maskPhaseInterrupts();
        isr_hot.lastzctime = isr_hot.thiszctime;
        isr_hot.thiszctime = INTERVAL_TIMER_COUNT;
        SET_INTERVAL_TIMER_COUNT(0);
        SET_AND_ENABLE_COM_INT(isr_hot.waitTime + 1);
        __enable_irq();
    }
}

/**
 * @brief This function handles TIM6 global and DAC underrun error interrupts.
 */
void TIMER13_IRQHandler(void)
{
    TIMER_INTF(TIMER13) = ~(uint32_t)TIMER_INT_FLAG_UP;  // Direct clear, saves 8-12 cycles
    tenKhzRoutine();
}

/**
 * @brief This function handles TIM14 global interrupt.
 */
void TIMER15_IRQHandler(void)
{
    interrupt_time = TIMER_CNT(UTILITY_TIMER);
    TIMER_INTF(TIMER15) = ~(uint32_t)TIMER_INT_FLAG_UP;  // Direct clear

    // Inline PeriodElapsedCallback — only called from here, saves 20-30 cycle call overhead (2.3)
    DISABLE_COM_TIMER_INT(); // disable commutation timer interrupt
    commutate();
    isr_hot.commutation_interval = ((isr_hot.commutation_interval) + ((isr_hot.lastzctime + isr_hot.thiszctime) >> 1)) >> 1;
    if (!ec_auto_advance) { /* Refactor 3.8: cached eepromBuffer.auto_advance */
        isr_hot.advance = (isr_hot.commutation_interval * temp_advance) >> 6;
    } else {
        isr_hot.advance = (isr_hot.commutation_interval * auto_advance_level) >> 6;
    }
    isr_hot.waitTime = (isr_hot.commutation_interval >> 1) - isr_hot.advance;
    if (!old_routine) {
        enableCompInterrupts();
    }
    if (zero_crosses < 10000) {
        zero_crosses++;
    }

    interrupt_time = ((uint16_t)TIMER_CNT(UTILITY_TIMER)) - interrupt_time;
}

void TIMER14_IRQHandler(void) { TIMER_INTF(TIMER14) = ~(uint32_t)TIMER_FLAG_UP; }  // Direct clear

/**
 * @brief This function handles USART1 global interrupt / USART1 wake-up
 * interrupt through EXTI line 25.
 */
void USART1_IRQHandler(void) { }

void TIMER2_IRQHandler(void)
{
    // sendDshotDma();
}

void EXTI4_15_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_15)) {
        exti_flag_clear(EXTI_15);
        processDshot();
    }
}

