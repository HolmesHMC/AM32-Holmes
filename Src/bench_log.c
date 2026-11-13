/*
 * bench_log.c - bench recorder
 *
 * Keeps the most recent drive-state samples in RAM that survives a reset
 * and writes them to one flash page: after a watchdog reset, or when asked.
 * A programmer reads the page through the bootloader. Built only with
 * BENCH_LOG.
 */
#include "targets.h"

#ifdef BENCH_LOG

#include "bench_log.h"
#include "eeprom.h"
#include "isr_hotdata.h"

#define BENCH_LOG_ADDRESS 0x08007000          /* written on request */
#define BENCH_LOG_WATCHDOG_ADDRESS 0x08007400 /* written after a watchdog reset */
#define BENCH_LOG_SAMPLES 40
#define BENCH_LOG_PERIOD_MS 10
#define BENCH_LOG_MAGIC 0x33474C42 /* "BLG3" */

extern volatile uint16_t newinput;
extern char stepper_sine;
extern char running;
extern char old_routine;
extern volatile char armed;
extern volatile uint32_t zero_crosses;
extern uint16_t step_delay;

typedef struct {
    uint16_t interval; /* six-step: commutation interval, sine: step delay */
    uint8_t input;     /* newinput / 8 */
    uint8_t flags;     /* b0 running, b1 sine, b2 polling, b3 armed, b4-7 zero crosses (max 15) */
    uint8_t loops;     /* main loop passes since the previous sample (max 255) */
    uint8_t step;      /* commutation step */
} bench_sample_t;

typedef struct {
    uint32_t magic;
    uint8_t count;       /* valid samples */
    uint8_t period_ms;
    uint8_t reset_flags; /* RCC_CSR bits 31..24 as found at boot */
    uint8_t head;        /* index of the oldest sample once the ring is full */
    bench_sample_t sample[BENCH_LOG_SAMPLES];
    uint32_t marks;      /* routine marks as found at boot */
} bench_log_t;

/* fixed RAM above .bss and below the stack; not cleared at start-up */
#define bench_log (*(bench_log_t*)0x20000A00)

static uint8_t bench_divider;
static uint8_t bench_last_sine;
static uint8_t bench_after; /* samples still to take after a handoff; 0xFF = no handoff seen yet */
static volatile uint8_t bench_loops;

void bench_log_loop(void)
{
    BENCH_MARK(0x10);
    if (bench_loops < 255) {
        bench_loops++;
    }
}

void bench_log_save(void)
{
    save_flash_nolib((uint8_t*)&bench_log, sizeof(bench_log), BENCH_LOG_ADDRESS);
}

void bench_log_boot(void)
{
    uint8_t flags = (uint8_t)(RCC->CSR >> 24);
    RCC->CSR |= RCC_CSR_RMVF;
    /* watchdog reset with samples still in RAM: keep them */
    if ((flags & 0x20) && bench_log.magic == BENCH_LOG_MAGIC && bench_log.count > 0) {
        bench_log.reset_flags = flags;
        bench_log.marks = BENCH_MARKS;
        save_flash_nolib((uint8_t*)&bench_log, sizeof(bench_log), BENCH_LOG_WATCHDOG_ADDRESS);
    }
    bench_log.magic = BENCH_LOG_MAGIC;
    bench_log.count = 0;
    bench_log.head = 0;
    bench_log.period_ms = BENCH_LOG_PERIOD_MS;
    bench_log.reset_flags = flags;
    bench_after = 0xFF;
}

void bench_log_tick(void)
{
    bench_sample_t* s;
    uint32_t zc;
    /* recording runs while armed */
    if (!armed) {
        return;
    }
    /* the ring freezes 30 samples after the first change from sine to six-step */
    if (bench_after == 0) {
        return;
    }
    if (bench_last_sine && !stepper_sine && bench_after == 0xFF) {
        bench_after = 30;
    }
    bench_last_sine = (uint8_t)stepper_sine;
    if (++bench_divider < BENCH_LOG_PERIOD_MS) {
        return;
    }
    bench_divider = 0;
    if (bench_after != 0xFF) {
        bench_after--;
    }
    if (bench_log.count < BENCH_LOG_SAMPLES) {
        s = &bench_log.sample[bench_log.count++];
    } else {
        s = &bench_log.sample[bench_log.head];
        bench_log.head = (uint8_t)((bench_log.head + 1) % BENCH_LOG_SAMPLES);
    }
    zc = zero_crosses;
    s->interval = stepper_sine ? step_delay : (uint16_t)isr_hot.commutation_interval;
    s->input = (uint8_t)(newinput >> 3);
    s->flags = (running ? 1 : 0) | (stepper_sine ? 2 : 0) | (old_routine ? 4 : 0) | 8
        | ((zc > 15 ? 15 : zc) << 4);
    s->loops = bench_loops;
    s->step = (uint8_t)isr_hot.step;
    bench_loops = 0;
}

#endif /* BENCH_LOG */
