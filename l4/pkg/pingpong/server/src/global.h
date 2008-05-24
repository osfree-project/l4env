#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <string.h>

#define STACKSIZE		8192
#define SCRATCH_MEM_SIZE	(32 << 20)

/* threads */
#define PAGER_THREAD		1
#define PING_THREAD		2
#define PONG_THREAD		3
#define MEMCPY_THREAD		4

#define NR_MSG        8
#define NR_DWORDS     4093  /* whole message descriptor occupies 4 pages */
#define NR_STRINGS    4
#define NR_BYTES      ((NR_DWORDS + 3) * 4)

#define SMAS_SIZE     16

#define timeout_10s	l4_ipc_timeout(0,0,610,14)
#define timeout_20s	l4_ipc_timeout(0,0,610,15)
#define timeout_50s	l4_ipc_timeout(0,0,762,16)
#define timeout_2m	l4_ipc_timeout(0,0,458,18)

#define REGISTER_DWORDS 2

enum test_type { INTER, INTRA, SINGLE, };

enum callmodes { INT30, SYSENTER, KIPCALL, NR_MODES };


#include <l4/sys/kernel.h>

extern l4_kernel_info_t *kip;

#include <l4/sys/cache.h>
static inline l4_cpu_time_t get_clocks_kip(void)
{
  l4_sys_cache_clean_range((unsigned long)&kip->clock, 8);
  return kip->clock;
}

#ifdef ARCH_x86
#include <l4/util/rdtsc.h>
static inline l4_cpu_time_t get_clocks(void) { return l4_rdtsc(); }
static inline l4_uint64_t clocks_to_us(l4_cpu_time_t clocks)
{ return l4_tsc_to_us(clocks); }
#else
static inline l4_cpu_time_t get_clocks(void) { return get_clocks_kip(); }
static inline l4_uint64_t clocks_to_us(l4_cpu_time_t clocks)
{ return clocks; }
#endif

#ifndef ARCH_x86
static inline void fiasco_timer_enable(void) {}
static inline void fiasco_timer_disable(void) {}
static inline void fiasco_watchdog_enable(void) {}
static inline void fiasco_watchdog_disable(void) {}
#endif

#define BENCH_END	                                    \
  do { if (fiasco_running) { if (!ux_running)               \
                               fiasco_watchdog_enable();    \
                             fiasco_timer_enable(); }       \
  } while (0)
#define BENCH_BEGIN	                                    \
  do { if (fiasco_running) { fiasco_timer_disable();        \
                             if (!ux_running)               \
                               fiasco_watchdog_disable(); } \
  } while (0)

extern void create_thread(l4_threadid_t id, l4_umword_t eip,
			  l4_umword_t esp, l4_threadid_t new_pager);
extern void test_memory_bandwidth(int nr);
extern void test_instruction_cycles(int nr);
extern void test_cache_tlb(int nr);
extern void flooder(void);
extern void test_flooder(void);
extern void touch_pages(void);

extern void exception6_handler(void);
extern void exception6_c_handler(void);

extern void *memchr(const void *s, int c, size_t n);

extern l4_threadid_t main_id;
extern l4_threadid_t pager_id;
extern l4_threadid_t ping_id;
extern l4_threadid_t pong_id;
extern l4_threadid_t memcpy_id;
extern l4_threadid_t getchar_id;
extern l4_threadid_t intra_ping, inter_ping;
extern l4_threadid_t intra_pong, inter_pong;
extern l4_umword_t scratch_mem, fpagesize;
extern l4_umword_t strsize, strnum, rounds;
extern l4_uint32_t mhz;
extern l4_umword_t global_rounds;
extern int use_superpages;
extern int deceit;
extern int callmode;
extern int ux_running;
extern int fiasco_running;
extern int dont_do_cold;

extern char _stext, _etext, _edata, _end;

extern unsigned char pong_stack[STACKSIZE] __attribute__((aligned(2048)));

#endif
