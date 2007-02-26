#ifndef __GLOBAL_H
#define __GLOBAL_H

#ifdef FIASCO_UX
#ifndef LOW_MEM
#define LOW_MEM
#endif
#endif

#ifdef LOW_MEM
#define SCRATCH_MEM_SIZE	(16*1024*1024)
#else
#define SCRATCH_MEM_SIZE	(32*1024*1024)
#endif

#define STACKSIZE		8192
#define DO_DEBUG		0

/* threads */
#define PAGER_THREAD  1
#define PING_THREAD   2
#define PONG_THREAD   3
#define MEMCPY_THREAD 4

/* number of test rounds */
#ifdef FIASCO_UX
#define ROUNDS 1250
#else
#define ROUNDS 12500
#endif

#define NR_MSG        8
#define NR_DWORDS     4093  /* whole message descriptor occupies 4 pages */
#define NR_STRINGS    4

#define SMAS_SIZE     16

#define timeout_10s	L4_IPC_TIMEOUT(0,0,152,7,0,0)
#define timeout_20s	L4_IPC_TIMEOUT(0,0,76,6,0,0)

#if defined(L4_API_L4X0) || defined(L4API_l4x0)
#define REGISTER_DWORDS 3
#else
#define REGISTER_DWORDS 2
#endif

#define INTRA		0
#define INTER		1

extern void create_thread(l4_threadid_t id, l4_umword_t eip, 
			  l4_umword_t esp, l4_threadid_t new_pager);
extern void test_mem_bandwidth(void);
extern void flooder(void);
extern void test_flooder(void);

extern void dummy_exception13_handler(void);
extern void exception6_handler(void);
extern void exception6_c_handler(void);

extern void* memchr(const void *s, int c, unsigned n);

extern l4_threadid_t main_id;
extern l4_threadid_t pager_id;
extern l4_threadid_t ping_id;
extern l4_threadid_t pong_id;
extern l4_threadid_t memcpy_id;
extern l4_umword_t scratch_mem, fpagesize;
extern l4_umword_t strsize, strnum, rounds;
extern l4_umword_t mhz;
extern l4_uint64_t flooder_costs;
extern int use_superpages;
extern int sysenter;

extern char _stext, _etext, _edata, _end;

#endif
