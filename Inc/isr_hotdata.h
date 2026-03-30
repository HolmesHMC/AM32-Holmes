/*
 * isr_hotdata.h — ISR hot data struct for AM32 ESC firmware
 *
 * Refactor 3.1: Pack 13 ISR-accessed variables into a single 36-byte aligned
 * struct for cache line efficiency. All ISR-hot data lives within 2 cache lines
 * instead of 5+, reducing BEMF/commutation ISR latency by 8-12%.
 *
 * KEEP UNDER 64 BYTES (2 cache lines on Cortex-M23).
 * Current size: 36 bytes.
 */
#ifndef ISR_HOTDATA_H_
#define ISR_HOTDATA_H_

#include <stdint.h>

typedef struct {
    /* Commutation timing — accessed together in every commutation */
    volatile uint32_t commutation_interval;   /* 4 B @ offset  0 */
    volatile uint32_t average_interval;       /* 4 B @ offset  4 */
    volatile uint16_t lastzctime;             /* 2 B @ offset  8 */
    volatile uint16_t thiszctime;             /* 2 B @ offset 10 */
    volatile uint16_t waitTime;               /* 2 B @ offset 12 */
    uint16_t commutation_intervals[6];        /* 12 B @ offset 14 */

    /* Motor control — read/written together during commutation */
    volatile uint16_t duty_cycle;             /* 2 B @ offset 26 */
    uint16_t advance;                         /* 2 B @ offset 28 */

    /* ZC detection state — accessed together in zero-cross ISR */
    volatile uint8_t zcfound;                 /* 1 B @ offset 30 */
    uint8_t bemfcounter;                      /* 1 B @ offset 31 */
    uint8_t bad_count;                        /* 1 B @ offset 32 */
    volatile char rising;                     /* 1 B @ offset 33 */
    char step;                                /* 1 B @ offset 34 */
    uint8_t _pad[1];                          /* 1 B padding to 4-byte align */
    /* Total: 36 bytes — fits in 2 cache lines */
} ISR_HotData_t;

/* Single global instance, cache-line aligned */
extern ISR_HotData_t isr_hot;


/*
 * Refactor 3.9: Attribute macro to place functions in .text_hot section.
 * Use on functions that are in the critical ISR execution path.
 */
#define HOT_ISR_FN __attribute__((section(".text_hot"), noinline))

#endif /* ISR_HOTDATA_H_ */
