/*
 * bench_log.h - bench recorder (bench builds only, enabled by BENCH_LOG)
 */
#ifndef BENCH_LOG_H_
#define BENCH_LOG_H_

#ifdef BENCH_LOG
/* last four routine marks, newest in the low byte; kept across a reset */
#define BENCH_MARKS (*(volatile unsigned long*)0x20000AFC)
#define BENCH_MARK(n) (BENCH_MARKS = (BENCH_MARKS << 8) | (n))
void bench_log_boot(void);   /* call once at start-up */
void bench_log_tick(void);   /* call at 1 kHz */
void bench_log_loop(void);   /* call once per main loop pass */
void bench_log_save(void);   /* write the samples to flash */
#else
#define BENCH_MARK(n)
#define bench_log_boot()
#define bench_log_tick()
#define bench_log_loop()
#define bench_log_save()
#endif

#endif
