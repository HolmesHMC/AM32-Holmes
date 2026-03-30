/*
 * phaseouts.c
 *
 *  Created on: Apr 22, 2020
 *      Author: Alka
 */
#include "phaseouts.h"
#include "isr_hotdata.h"  /* Refactor 3.9: HOT_ISR_FN macro */

#include "targets.h"

extern char prop_brake_active;

#ifndef PWM_ENABLE_BRIDGE

#ifdef USE_INVERTED_LOW
#pragma message("using inverted low side output")
#define LOW_BITREG_ON BRR
#define LOW_BITREG_OFF BSRR
#else
#define LOW_BITREG_ON BSRR
#define LOW_BITREG_OFF BRR
#endif

#ifdef USE_INVERTED_HIGH
#pragma message("using inverted high side output")
// #define HIGH_BITREG_ON  BRR
#define HIGH_BITREG_OFF BSRR
#else
// #define HIGH_BITREG_ON  BSRR
#define HIGH_BITREG_OFF BRR
#endif

/* Always inlined: called 3x per commutation, saves 30-48 cycles total */
__attribute__((always_inline)) static inline void gpio_mode_QUICK(
    uint32_t gpio_periph, uint32_t mode, uint32_t pull_up_down, uint32_t pin)
{
    GPIO_CTL(gpio_periph) = ((((((GPIO_CTL(gpio_periph)))) & (~(((pin * pin) * (0x3UL << (0U)))))) | (((pin * pin) * mode))));
}

void proportionalBrake()
{ // alternate all channels between braking (ABC LOW)
    // and coasting (ABC float) put lower channel into
    // alternate mode and turn upper OFF for each
    // channel
    // turn all HIGH channels off for ABC

    //	gpio_mode_QUICK(PHASE_A_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT,
    // GPIO_PUPD_NONE, PHASE_A_GPIO_HIGH);
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_A_GPIO_HIGH);
    GPIO_BC(PHASE_A_GPIO_PORT_HIGH) = PHASE_A_GPIO_HIGH;

    gpio_mode_QUICK(PHASE_B_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_B_GPIO_HIGH);
    GPIO_BC(PHASE_B_GPIO_PORT_HIGH) = PHASE_B_GPIO_HIGH;

    gpio_mode_QUICK(PHASE_C_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_C_GPIO_HIGH);
    GPIO_BC(PHASE_C_GPIO_PORT_HIGH) = PHASE_C_GPIO_HIGH;

    // set low channel to PWM, duty cycle will now control braking
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_A_GPIO_LOW);
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_B_GPIO_LOW);
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_C_GPIO_LOW);
}

void phaseBPWM()
{
    if (!eepromBuffer.comp_pwm) { // for future
        gpio_mode_QUICK(PHASE_B_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
            PHASE_B_GPIO_LOW);
        GPIO_BC(PHASE_B_GPIO_PORT_LOW) = PHASE_B_GPIO_LOW;
    } else {
        gpio_mode_QUICK(PHASE_B_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
            PHASE_B_GPIO_LOW); // low
        //		GPIO_CTL(PHASE_B_GPIO_PORT_LOW) |= (2 <<
        //(PHASE_B_GPIO_LOW
        //<<2)); 		GPIO_CTL(PHASE_B_GPIO_PORT_LOW) =
        //((((((GPIO_CTL(PHASE_B_GPIO_PORT_LOW)))) & (~(((PHASE_B_GPIO_LOW *
        // PHASE_B_GPIO_LOW) * (0x3UL << (0U)))))) | (((PHASE_B_GPIO_LOW *
        // PHASE_B_GPIO_LOW) * 2))));
    }
    gpio_mode_QUICK(
        PHASE_B_GPIO_PORT_HIGH, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_B_GPIO_HIGH); // high
                            //	  GPIO_CTL(PHASE_B_GPIO_PORT_HIGH) =
    //((((((GPIO_CTL(PHASE_B_GPIO_PORT_HIGH)))) & (~(((PHASE_B_GPIO_HIGH *
    // PHASE_B_GPIO_HIGH) * (0x3UL << (0U)))))) | (((PHASE_B_GPIO_HIGH *
    // PHASE_B_GPIO_HIGH) * 2))));
}

void phaseBFLOAT()
{
    gpio_mode_QUICK(PHASE_B_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_B_GPIO_LOW);
    GPIO_BC(PHASE_B_GPIO_PORT_LOW) = PHASE_B_GPIO_LOW;
    gpio_mode_QUICK(PHASE_B_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_B_GPIO_HIGH);
    GPIO_BC(PHASE_B_GPIO_PORT_HIGH) = PHASE_B_GPIO_HIGH;
}

void phaseBLOW()
{
    // low mosfet on
    gpio_mode_QUICK(PHASE_B_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_B_GPIO_LOW);
    GPIO_BOP(PHASE_B_GPIO_PORT_LOW) = PHASE_B_GPIO_LOW;
    gpio_mode_QUICK(PHASE_B_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_B_GPIO_HIGH);
    GPIO_BC(PHASE_B_GPIO_PORT_HIGH) = PHASE_B_GPIO_HIGH;
}

//////////////////////////////PHASE
/// 2//////////////////////////////////////////////////

void phaseCPWM()
{
    if (!eepromBuffer.comp_pwm) {
        gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
            PHASE_C_GPIO_LOW);
        GPIO_BC(PHASE_C_GPIO_PORT_LOW) = PHASE_C_GPIO_LOW;
    } else {
        gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
            PHASE_C_GPIO_LOW);
        ;
    }
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_HIGH, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_C_GPIO_HIGH);
}

void phaseCFLOAT()
{
    // floating
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_C_GPIO_LOW);
    GPIO_BC(PHASE_C_GPIO_PORT_LOW) = PHASE_C_GPIO_LOW;
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_C_GPIO_HIGH);
    GPIO_BC(PHASE_C_GPIO_PORT_HIGH) = PHASE_C_GPIO_HIGH;
}

void phaseCLOW()
{
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_C_GPIO_LOW);
    GPIO_BOP(PHASE_C_GPIO_PORT_LOW) = PHASE_C_GPIO_LOW;
    gpio_mode_QUICK(PHASE_C_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_C_GPIO_HIGH);
    GPIO_BC(PHASE_C_GPIO_PORT_HIGH) = PHASE_C_GPIO_HIGH;
}

///////////////////////////////////////////////PHASE 3
////////////////////////////////////////////////////

void phaseAPWM()
{
    if (!eepromBuffer.comp_pwm) {
        gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
            PHASE_A_GPIO_LOW);
        GPIO_BC(PHASE_A_GPIO_PORT_LOW) = PHASE_A_GPIO_LOW;
    } else {
        gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_AF, GPIO_PUPD_NONE,
            PHASE_A_GPIO_LOW);
    }
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_HIGH, GPIO_MODE_AF, GPIO_PUPD_NONE,
        PHASE_A_GPIO_HIGH);
}

void phaseAFLOAT()
{
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_A_GPIO_LOW);
    GPIO_BC(PHASE_A_GPIO_PORT_LOW) = PHASE_A_GPIO_LOW;
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_A_GPIO_HIGH);
    GPIO_BC(PHASE_A_GPIO_PORT_HIGH) = PHASE_A_GPIO_HIGH;
}

void phaseALOW()
{
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_A_GPIO_LOW);
    GPIO_BOP(PHASE_A_GPIO_PORT_LOW) = PHASE_A_GPIO_LOW;
    gpio_mode_QUICK(PHASE_A_GPIO_PORT_HIGH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        PHASE_A_GPIO_HIGH);
    GPIO_BC(PHASE_A_GPIO_PORT_HIGH) = PHASE_A_GPIO_HIGH;
}

#else

//////////////////////////////////PHASE 1//////////////////////
void phaseBPWM()
{
    if (!eepromBuffer.comp_pwm) { // for future
                     // gpio_mode_QUICK(PHASE_B_GPIO_PORT_LOW, GPIO_MODE_OUTPUT,
                     // GPIO_PUPD_NONE, PHASE_B_GPIO_LOW);
                     // GPIO_BC(PHASE_B_GPIO_PORT_LOW) = PHASE_B_GPIO_LOW;
    } else {
        LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_ENABLE, PHASE_B_GPIO_ENABLE,
            LL_GPIO_MODE_OUTPUT); // enable on
        PHASE_B_GPIO_PORT_ENABLE->BSRR = PHASE_B_GPIO_ENABLE;
    }
    LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_PWM, PHASE_B_GPIO_PWM,
        LL_GPIO_MODE_ALTERNATE); // high pwm
}

void phaseBFLOAT()
{
    LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_ENABLE, PHASE_B_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable off
    PHASE_B_GPIO_PORT_ENABLE->BRR = PHASE_B_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_PWM, PHASE_B_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT); // pwm off
    PHASE_B_GPIO_PORT_PWM->BRR = PHASE_B_GPIO_PWM;
}

void phaseBLOW()
{
    // low mosfet on
    LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_ENABLE, PHASE_B_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable on
    PHASE_B_GPIO_PORT_ENABLE->BSRR = PHASE_B_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_B_GPIO_PORT_PWM, PHASE_B_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT); // pwm off
    PHASE_B_GPIO_PORT_PWM->BRR = PHASE_B_GPIO_PWM;
}

//////////////////////////////PHASE
/// 2//////////////////////////////////////////////////

void phaseCPWM()
{
    if (!eepromBuffer.comp_pwm) {
        //	gpio_mode_QUICK(PHASE_C_GPIO_PORT_LOW, GPIO_MODE_OUTPUT,
        // GPIO_PUPD_NONE,
        // PHASE_C_GPIO_LOW); GPIO_BC(PHASE_C_GPIO_PORT_LOW) = PHASE_C_GPIO_LOW;
    } else {
        LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_ENABLE, PHASE_C_GPIO_ENABLE,
            LL_GPIO_MODE_OUTPUT); // enable on
        PHASE_C_GPIO_PORT_ENABLE->BSRR = PHASE_C_GPIO_ENABLE;
    }
    LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_PWM, PHASE_C_GPIO_PWM,
        LL_GPIO_MODE_ALTERNATE);
}

void phaseCFLOAT()
{
    // floating
    LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_ENABLE, PHASE_C_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable off
    PHASE_C_GPIO_PORT_ENABLE->BRR = PHASE_C_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_PWM, PHASE_C_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT);
    PHASE_C_GPIO_PORT_PWM->BRR = PHASE_C_GPIO_PWM;
}

void phaseCLOW()
{
    LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_ENABLE, PHASE_C_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable on
    PHASE_C_GPIO_PORT_ENABLE->BSRR = PHASE_C_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_C_GPIO_PORT_PWM, PHASE_C_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT);
    PHASE_C_GPIO_PORT_PWM->BRR = PHASE_C_GPIO_PWM;
}

///////////////////////////////////////////////PHASE 3
////////////////////////////////////////////////////

void phaseAPWM()
{
    if (!eepromBuffer.comp_pwm) {
        //	gpio_mode_QUICK(PHASE_A_GPIO_PORT_LOW, GPIO_MODE_OUTPUT,
        // GPIO_PUPD_NONE,
        // PHASE_A_GPIO_LOW); GPIO_BC(PHASE_A_GPIO_PORT_LOW) = PHASE_A_GPIO_LOW;
    } else {
        LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_ENABLE, PHASE_A_GPIO_ENABLE,
            LL_GPIO_MODE_OUTPUT); // enable on
        PHASE_A_GPIO_PORT_ENABLE->BSRR = PHASE_A_GPIO_ENABLE;
    }
    LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_PWM, PHASE_A_GPIO_PWM,
        LL_GPIO_MODE_ALTERNATE);
}

void phaseAFLOAT()
{
    LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_ENABLE, PHASE_A_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable on
    PHASE_A_GPIO_PORT_ENABLE->BRR = PHASE_A_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_PWM, PHASE_A_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT);
    PHASE_A_GPIO_PORT_PWM->BRR = PHASE_A_GPIO_PWM;
}

void phaseALOW()
{
    LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_ENABLE, PHASE_A_GPIO_ENABLE,
        LL_GPIO_MODE_OUTPUT); // enable on
    PHASE_A_GPIO_PORT_ENABLE->BSRR = PHASE_A_GPIO_ENABLE;
    LL_GPIO_SetPinMode(PHASE_A_GPIO_PORT_PWM, PHASE_A_GPIO_PWM,
        LL_GPIO_MODE_OUTPUT);
    PHASE_A_GPIO_PORT_PWM->BRR = PHASE_A_GPIO_PWM;
}

#endif

void allOff()
{
    phaseAFLOAT();
    phaseBFLOAT();
    phaseCFLOAT();
}

/*
 * Refactor 3.3: comStep() implemented as function pointer table.
 * Replaces 6-case switch with a single indexed indirect call.
 * Table indexed 1..6 (index 0 unused). Eliminates branch chain and
 * allows the CPU to load the target address from a predictable table
 * location, improving instruction cache utilization.
 */
typedef void (*step_fn_t)(void);

static void do_step1(void) { phaseCFLOAT(); phaseBLOW();  phaseAPWM(); } /* A-B */
static void do_step2(void) { phaseAFLOAT(); phaseBLOW();  phaseCPWM(); } /* C-B */
static void do_step3(void) { phaseBFLOAT(); phaseALOW();  phaseCPWM(); } /* C-A */
static void do_step4(void) { phaseCFLOAT(); phaseALOW();  phaseBPWM(); } /* B-A */
static void do_step5(void) { phaseAFLOAT(); phaseCLOW();  phaseBPWM(); } /* B-C */
static void do_step6(void) { phaseBFLOAT(); phaseCLOW();  phaseAPWM(); } /* A-C */

static const step_fn_t comstep_table[7] = {
    (step_fn_t)0, /* [0] unused — step is 1-based */
    do_step1, do_step2, do_step3, do_step4, do_step5, do_step6
};

HOT_ISR_FN void comStep(int newStep)
{
    /* Bounds-guarded indirect call — replaces 6-case switch (Refactor 3.3) */
    if (__builtin_expect((unsigned int)newStep < 7u, 1)) {
        comstep_table[newStep]();
    }
}

void fullBrake()
{ // full braking shorting all low sides
    phaseALOW();
    phaseBLOW();
    phaseCLOW();
}

void allpwm()
{ // for stepper_sine
    phaseAPWM();
    phaseBPWM();
    phaseCPWM();
}

void twoChannelForward()
{
    phaseAPWM();
    phaseBLOW();
    phaseCPWM();
}

void twoChannelReverse()
{
    phaseALOW();
    phaseBPWM();
    phaseCLOW();
}
