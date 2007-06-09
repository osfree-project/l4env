/* $Id$ */
/*****************************************************************************/
/**
 * \file   pingpong/server/src/worker_ipc.h
 */
/*****************************************************************************/

#include "string.h"

/* which test */
#define CALL_ONLY    0

/* Don't enable this because it is unfair. We normally call the server
 * and the server replies by reply_and_wait using send timeout Zero. */
#if CALL_ONLY

/* IPC operation to use in ping thread - short call, timeout never */
#define WHATTODO_SHORT_PING \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in pong thread - short call, timeout never */
#define WHATTODO_SHORT_PONG \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

#else

/* IPC operation to use in ping thread - short call
 * send timeout never, receive timeout never */
#define WHATTODO_SHORT_PING \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in pong thread - short reply
 * send timeout 0, receive timeout never */
#define WHATTODO_SHORT_PONG \
  "mov  $1,%%ebp     \n\t"  \
  "mov  $0x04000000,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

#endif /* !CALL_ONLY */

/* IPC operating to use in ping thread - send timeout 0, receive with (long) timeout */
#define WHATTODO_SHORT_TO_PING \
  "sub  %%ebp,%%ebp  \n\t"  \
  "mov  $0x04007cff,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in pong thread - short reply
 * send timeout 0, receive with (long) timeout */
#define WHATTODO_SHORT_TO_PONG \
  "mov  $1,%%ebp     \n\t"  \
  "mov  $0x04007cff,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in ping thread - short send, receive long
 * send timeout never, receive timeout never */
#define WHATTODO_LONG_PING \
  "sub  %%eax,%%eax  \n\t" \
  "sub  %%ecx,%%ecx  \n\t" \
  "push %%ebp        \n\t" \
  IPC_SYSENTER             \
  "pop  %%ebp        \n\t" \
  "add  $(" L4_stringify(NR_BYTES) "),%%ebp \n\t"

/* IPC operation to use in pong thread - reply long
 * send timeout 0, receive timeout never */
#define WHATTODO_LONG_PONG \
  "mov  $1,%%ebp     \n\t" \
  "mov  $0x04000000,%%ecx  \n\t" \
  "push %%eax        \n\t" \
  IPC_SYSENTER             \
  "pop  %%eax        \n\t" \
  "add  $(" L4_stringify(NR_BYTES) "),%%eax \n\t"

/* IPC operation to use in ping thread - short send, receive long
 * send timeout never, receive timeout never */
#define WHATTODO_INDIRECT_PING \
  "sub  %%eax,%%eax  \n\t" \
  "sub  %%ecx,%%ecx  \n\t" \
  "push %%ebp        \n\t" \
  IPC_SYSENTER             \
  "pop  %%ebp        \n\t" \
  "add  $128,%%ebp   \n\t"

/* IPC operation to use in pong thread - reply long
 * send timeout 0, receive timeout never */
#define WHATTODO_INDIRECT_PONG \
  "mov  $1,%%ebp     \n\t" \
  "mov  $0x04000000,%%ecx  \n\t" \
  "push %%eax        \n\t" \
  IPC_SYSENTER             \
  "pop  %%eax        \n\t" \
  "add  $128,%%eax   \n\t"

/* IPC operation to use in ping thread - short send, receive fpage
 * send timeout never, receive timeout never */
#define WHATTODO_FPAGE_PING \
  "mov  $0x82,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in send thread - short send fpage. We don't need
 * to save the two dwords describing the flexpage since the recevie thread
 * replies both values immediatly to the sender
 * send timeout 0, receive timeout never */
#define WHATTODO_FPAGE_PONG \
  "push %%eax        \n\t"  \
  "mov  $1,%%ebp     \n\t"  \
  "mov  $0x04000000,%%ecx  \n\t"  \
  "mov  $2,%%eax     \n\t"  \
  IPC_SYSENTER              \
  "pop  %%eax        \n\t"  \
  "add  %%eax,%%edx  \n\t"  \
  "add  %%eax,%%ebx  \n\t"

#define WHATTODO_SYSCALL_L4_MYSELF \
  "xorl %%esi,%%esi  \n\t"         \
  L4_SYSCALL(id_nearest)

#define WHATTODO_SYSCALL_L4_NCHIEF \
  "mov  %%ebx,%%esi  \n\t"         \
  L4_SYSCALL(id_nearest)


extern int PREFIX(l4_ipc_call_asm) (
			l4_threadid_t dest,
			const void *snd_msg,
			l4_umword_t snd_dword0,
			l4_umword_t snd_dword1,
			void *rcv_msg,
			l4_umword_t *rcv_dword0,
			l4_umword_t *rcv_dword1,
			l4_timeout_t timeout,
			l4_msgdope_t *result)
			__attribute__((regparm(3)));
extern int PREFIX(l4_ipc_reply_and_wait_asm) (
			l4_threadid_t dest,
			const void *snd_msg,
			l4_umword_t snd_dword0,
			l4_umword_t snd_dword1,
			l4_threadid_t *src,
			void *rcv_msg,
			l4_umword_t *rcv_dword0,
			l4_umword_t *rcv_dword1,
			l4_timeout_t timeout,
			l4_msgdope_t *result)
			__attribute__((regparm(3)));

extern int dont_do_cold;


/* ------------------------------------------------------------------------- */
/* Short IPC in Shortcut (Timout Never, Timeout 0) (warm/cold)               */
/* ------------------------------------------------------------------------- */

/** Pong (reply) thread for short IPCs. */
void __attribute__((noreturn))
PREFIX(pong_short_thread)(void)
{
  register l4_umword_t ping_id32  = ping_id.raw;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 "pop   %%ebp    \n\t"
	 :
	 : "S" (ping_id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	 );
    }
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t id32 = pong_id.raw;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 "pop   %%ebp    \n\t"
	 :
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" :
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds,
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t id32 = pong_id.raw;
  const l4_umword_t rounds = 10;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      tsc -= l4_rdtsc();
      flooder();
      tsc += l4_rdtsc();
      asm volatile 
	(
	 "push  %%ebp   	\n\t"
	 WHATTODO_SHORT_PING
	 "pop   %%ebp    	\n\t"
	 :
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/cold: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Short IPC, not Shortcut (Timeout 1ms) (warm/cold)                         */
/* ------------------------------------------------------------------------- */

/** Pong (reply) thread for short IPCs. */
void __attribute__((noreturn))
PREFIX(pong_short_to_thread)(void)
{
  l4_umword_t ping_id32 = ping_id.raw;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 WHATTODO_SHORT_TO_PONG
	 "pop   %%ebp    \n\t"
	 :
	 : "S" (ping_id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	 );
    }
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_to_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t id32 = pong_id.raw;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 WHATTODO_SHORT_TO_PING
	 "pop   %%ebp    \n\t"
	 :
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);      
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds, 
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_to_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t id32 = pong_id.raw;
  const l4_umword_t rounds = 10;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      tsc -= l4_rdtsc();
      flooder();
      tsc += l4_rdtsc();
      asm volatile 
	(
	 "push  %%ebp   	\n\t"
	 WHATTODO_SHORT_TO_PING
	 "pop   %%ebp		\n\t"
	 :
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);      
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/cold: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Short IPC with deceit bit                                                 */
/* ------------------------------------------------------------------------- */

/** Pong (reply) thread for short IPCs. */
void __attribute__((noreturn))
PREFIX(pong_short_dc_thread)(void)
{
  l4_umword_t id32 = main_id.raw;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(send)(main_id);

  asm volatile 
    (
     "push %%ebp       \n\t"
     "mov  $1,%%ebp    \n\t"
     "sub  %%ecx,%%ecx \n\t"
     "or   $-1,%%eax   \n\t"
     IPC_SYSENTER
     "pop  %%ebp       \n\t"
     :
     : "S" (id32)
     : "eax", "ebx", "ecx", "edx", "edi"
    );

  for (;;)
    PREFIX(send)(main_id);
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_dc_thread)(void)
{
  int i;
  l4_umword_t dummy1, dummy2 __attribute__((unused));
  l4_cpu_time_t tsc;
  register l4_umword_t id32   = pong_id.raw;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(main_id);

  tsc = l4_rdtsc();
  for (i=0; i<200; i++)
    {
      asm volatile 
	(
	 "push %%ebp       \n\t"
	 "or   $-1,%%ebp   \n\t"
	 "sub  %%ecx,%%ecx \n\t"
	 "mov  $1,%%eax    \n\t"
	 IPC_SYSENTER
	 "pop   %%ebp      \n\t"
	 : "=S"(dummy1)
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);
      id32  += 0x20000;
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/nosw: %8u cycles / %6u rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, 200, 
	 (l4_uint32_t)(tsc/200));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Pong (reply) thread for short IPCs. */
void __attribute__((noreturn))
PREFIX(pong_short_ndc_thread)(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(send)(main_id);

  asm volatile 
    (
     "push %%ebp       \n\t"
     "mov  $1,%%ebp    \n\t"
     "sub  %%ecx,%%ecx \n\t"
     "or   $-1,%%eax   \n\t"
     IPC_SYSENTER
     "or   $-1,%%ebp   \n\t"
     "sub  %%ecx,%%ecx \n\t"
     "sub  %%eax,%%eax \n\t"
     IPC_SYSENTER
     "pop  %%ebp       \n\t"
     :
     :
     : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );

  for (;;)
    PREFIX(send)(main_id);
}

/** Ping (send) thread */
void __attribute__((noreturn))
PREFIX(ping_short_ndc_thread)(void)
{
  int i;
  l4_umword_t dummy1, dummy2 __attribute__((unused));
  l4_cpu_time_t tsc;
  register l4_umword_t id32   = pong_id.raw;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(main_id);

  tsc = l4_rdtsc();
  for (i=0; i<200; i++)
    {
      asm volatile 
	(
	 "push %%ebp       \n\t"
	 "mov  $1,%%ebp    \n\t"
	 "sub  %%ecx,%%ecx \n\t"
	 "sub  %%eax,%%eax \n\t"
	 IPC_SYSENTER
	 "pop   %%ebp      \n\t"
	 : "=S"(dummy1)
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
	);
      id32  += 0x20000;
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/call: %8u cycles / %6u rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, 200, 
	 (l4_uint32_t)(tsc/200));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/* ------------------------------------------------------------------------- */
/* Short IPC (warm/cold) using C-Bindings                                    */
/* ------------------------------------------------------------------------- */

/** Pong (reply) thread for short IPCs. */
void  __attribute__((noreturn))
PREFIX(pong_short_c_thread)(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;
      l4_threadid_t src;

      l4_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 2, 1,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_SEND_TIMEOUT_0, &result);
      l4_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 4, 3,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_SEND_TIMEOUT_0, &result);
      l4_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 6, 5,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_SEND_TIMEOUT_0, &result);
      l4_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 8, 7,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_SEND_TIMEOUT_0, &result);
    }
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_c_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = global_rounds*2; i; i--)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;

      l4_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 1, 2,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 3, 4,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 5, 6,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 7, 8,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds,
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_c_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  l4_umword_t rounds = 10;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;

      tsc -= l4_rdtsc();
      flooder();
      tsc += l4_rdtsc();
      l4_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 1, 2,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/cold: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Short IPC (warm/cold) using Assembler library functions (not inline)      */
/* ------------------------------------------------------------------------- */

/** Pong (reply) thread for short IPCs. */
void __attribute__((noreturn))
PREFIX(pong_short_asm_thread)(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;
      l4_threadid_t src;

      PREFIX(l4_ipc_reply_and_wait_asm)(ping_id, L4_IPC_SHORT_MSG, 2, 1,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_SEND_TIMEOUT_0, &result);
      PREFIX(l4_ipc_reply_and_wait_asm)(ping_id, L4_IPC_SHORT_MSG, 4, 3,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_SEND_TIMEOUT_0, &result);
      PREFIX(l4_ipc_reply_and_wait_asm)(ping_id, L4_IPC_SHORT_MSG, 6, 5,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_SEND_TIMEOUT_0, &result);
      PREFIX(l4_ipc_reply_and_wait_asm)(ping_id, L4_IPC_SHORT_MSG, 8, 7,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_SEND_TIMEOUT_0, &result);
    }
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_asm_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = global_rounds*2; i; i--)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;

      PREFIX(l4_ipc_call_asm)(pong_id,
			   L4_IPC_SHORT_MSG, 1, 2,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      PREFIX(l4_ipc_call_asm)(pong_id,
			   L4_IPC_SHORT_MSG, 3, 4,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      PREFIX(l4_ipc_call_asm)(pong_id,
			   L4_IPC_SHORT_MSG, 5, 6,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      PREFIX(l4_ipc_call_asm)(pong_id,
			   L4_IPC_SHORT_MSG, 7, 8,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds, 
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_short_asm_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  l4_umword_t rounds = 10;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      l4_umword_t dummy1, dummy2;
      l4_msgdope_t result;

      tsc -= l4_rdtsc();
      flooder();
      tsc += l4_rdtsc();
      PREFIX(l4_ipc_call_asm)(pong_id,
			   L4_IPC_SHORT_MSG, 1, 2,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
    }
  tsc = l4_rdtsc() - tsc;

  printf("  %s/cold: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" : 
	 (callmode == 2) ? "kipcalls" : "   int30",
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Pong (reply) thread. */
void __attribute__((noreturn))
PREFIX(pong_long_thread)(void)
{
  l4_umword_t ping_id32 = ping_id.raw;
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<8; i++)
    {
      long_send_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_send_msg[i].send_dope = L4_IPC_DOPE(strsize, 0);
      memset(&long_send_msg[i].dw[0], 0x67, 4*NR_DWORDS);
    }

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      l4_umword_t dummy;
      asm volatile 
	(
	 "push %%ebp      \n\t"
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 "pop   %%ebp    \n\t"
	 : "=a" (dummy)
	 : "a" (long_send_msg), "S" (ping_id32)
	 : "ebx", "ecx", "edx", "edi"
	 );
    }
}


/* ------------------------------------------------------------------------- */
/* Long IPC (warm/cold)                                                      */
/* ------------------------------------------------------------------------- */

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_long_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x66, 4*NR_DWORDS);
    }

    {
      register l4_umword_t id32 = pong_id.raw;

      PREFIX(call)(pong_id);

      tsc = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     "pop   %%ebp      \n\t"
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  for (i=0; i<NR_MSG; i++)
    {
      if (memchr(&long_recv_msg[i].dw[REGISTER_DWORDS], 
		 0x66, 4*(strsize-REGISTER_DWORDS)))
	{
	  printf("Test failed (found 0x66 in %08lx-%08lx)!\n",
	      (unsigned long)&long_recv_msg[i].dw[REGISTER_DWORDS],
	      (unsigned long)&long_recv_msg[i].dw[REGISTER_DWORDS]
			+4*(strsize-REGISTER_DWORDS));
	  enter_kdebug("stop");
	  break;
	}
    }

  printf("  %s %4ld dwords (%5ldB): %10u cycles / %6lu rounds >> %5u <<\n",
	 dont_do_cold ? "" : "warm",
         strsize, strsize*4,
	 (l4_uint32_t)tsc, 8*rounds, (l4_uint32_t)(tsc/(8*rounds)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_long_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x66, 4*NR_DWORDS);
    }

    {
      register l4_umword_t id32 = pong_id.raw;
      PREFIX(call)(pong_id);

      rounds = 10;
      tsc = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  tsc -= l4_rdtsc();
	  flooder();
	  tsc += l4_rdtsc();
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_LONG_PING
	     "pop   %%ebp      \n\t"
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  printf("  cold %4ld dwords (%5ldB): %10u cycles / %6lu rounds >> %5u <<\n",
         strsize, strsize*4,
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Pong (reply) thread. */
void __attribute__((noreturn))
PREFIX(pong_indirect_thread)(void)
{
  l4_umword_t ping_id32 = ping_id.raw;
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<8; i++)
    {
      int j;

      indirect_send_msg[i].size_dope = L4_IPC_DOPE(4, strnum);
      indirect_send_msg[i].send_dope = L4_IPC_DOPE(2, strnum);
      indirect_send_msg[i].dw[0] = 0x10101010;
      indirect_send_msg[i].dw[1] = 0x11111111;
      indirect_send_msg[i].dw[2] = 0x12121212;
      indirect_send_msg[i].dw[3] = 0x13131313;
      for (j=0; j<strnum; j++)
	{
	  l4_umword_t str = (l4_umword_t)&indirect_send_str
			  + i*NR_STRINGS*(NR_DWORDS+3)*4
			  + j*strsize*4;
	  indirect_send_msg[i].str[j].snd_size = strsize*4;
	  indirect_send_msg[i].str[j].snd_str  = str;
	  indirect_send_msg[i].str[j].rcv_size = 0;
	  indirect_send_msg[i].str[j].rcv_str  = 0;
	  memset((void*)str, 's', strsize*4);
	}
    }

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  while (1)
    {
      l4_umword_t dummy;
      asm volatile 
	(
	 "push %%ebp      \n\t"
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 "pop   %%ebp    \n\t"
	 : "=a" (dummy)
	 : "a" (indirect_send_msg), "S" (ping_id32)
	 : "ebx", "ecx", "edx", "edi"
	 );
    }
}


/* ------------------------------------------------------------------------- */
/* Indirect IPC (warm/cold)                                                  */
/* ------------------------------------------------------------------------- */

/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_indirect_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      int j;

      indirect_recv_msg[i].size_dope = L4_IPC_DOPE(4, strnum);
      indirect_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&indirect_recv_msg[i].dw[0], 0x66, 4*4);
      for (j=0; j<strnum; j++)
	{
	  l4_umword_t str = (l4_umword_t)&indirect_recv_str
			  + i*NR_STRINGS*(NR_DWORDS+3)*4
			  + j*strsize*4;
	  indirect_recv_msg[i].str[j].snd_size = 0;
	  indirect_recv_msg[i].str[j].snd_str  = 0;
	  indirect_recv_msg[i].str[j].rcv_size = strsize*4;
	  indirect_recv_msg[i].str[j].rcv_str  = str;
	  memset((void*)str, 'r', strsize*4);
	}
    }

    {
      register l4_umword_t id32 = pong_id.raw;

      PREFIX(call)(pong_id);

      tsc = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     "pop   %%ebp      \n\t"
	     : "=b" (dummy)
	     :  "b" (&indirect_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  printf("  %s %ldx%5ld dwords (%5ldB): %10u cycles / %6lu rounds >> %7u <<\n",
	 dont_do_cold ? "" : "warm",
         strnum, strsize, strnum*strsize*4,
	 (l4_uint32_t)tsc, rounds*8, (l4_uint32_t)(tsc/(rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/** Ping (send) thread. */
void __attribute__((noreturn))
PREFIX(ping_indirect_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      int j;

      indirect_recv_msg[i].size_dope = L4_IPC_DOPE(4, strnum);
      indirect_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&indirect_recv_msg[i].dw[0], 0x66, 4*4);
      for (j=0; j<strnum; j++)
	{
	  l4_umword_t str = (l4_umword_t)&indirect_recv_str
			  + i*NR_STRINGS*(NR_DWORDS+3)*4
			  + j*strsize*4;
	  indirect_recv_msg[i].str[j].snd_size = 0;
	  indirect_recv_msg[i].str[j].snd_str  = 0;
	  indirect_recv_msg[i].str[j].rcv_size = strsize*4;
	  indirect_recv_msg[i].str[j].rcv_str  = str;
	  memset((void*)str, 'r', strsize*4);
	}
    }

    {
      register l4_umword_t id32 = pong_id.raw;

      PREFIX(call)(pong_id);

      rounds = 10;
      tsc = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  tsc -= l4_rdtsc();
	  flooder();
	  tsc += l4_rdtsc();
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_INDIRECT_PING
	     "pop   %%ebp      \n\t"
	     : "=b" (dummy)
	     :  "b" (&indirect_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  printf("  cold %ldx%5ld dwords (%5ldB): %10u cycles / %6lu rounds >> %7u <<\n",
         strnum, strsize, strnum*strsize*4,
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/rounds));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Fpage IPC (warm/cold)                                                     */
/* ------------------------------------------------------------------------- */

/** Receive fpage thread. */
void __attribute__((noreturn))
PREFIX(pong_fpage_thread)(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

    {
      register l4_umword_t ping_id32 = ping_id.raw;
      register l4_umword_t dw0 = scratch_mem;
      register l4_umword_t dw1 = l4_fpage(scratch_mem, l4util_log2(fpagesize),
					  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
      while (1)
	{
	  asm volatile
	    (
	     "push  %%ebp   \n\t"
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     "pop   %%ebp    \n\t"
	     : "=d" (dw0), "=b" (dw1)
	     : "S" (ping_id32), "d" (dw0), "b" (dw1), "a" (fpagesize)
	     : "ecx", "edi"
	    );
	}
    }
}

/** Send short fpages. */
void __attribute__((noreturn))
PREFIX(ping_fpage_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t id32 = pong_id.raw;
  register l4_umword_t dw0 = 0, dw1 = 0;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      asm volatile
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 "pop  %%ebp    \n\t"
	 :  "=d" (dw0), "=b" (dw1)
	 :  "S" (id32),  "d" (dw0),  "b" (dw1), "D" (0)
	 : "eax", "ecx"
	);
    }
  tsc = l4_rdtsc() - tsc;
  
  printf("  %4ldkB: %10u cycles / %5lu rounds >> %8u <<\n",
         fpagesize/1024,
	 (l4_uint32_t)tsc, rounds*8, (l4_uint32_t)(tsc/(rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Long Fpage IPC (warm/cold)                                                */
/* ------------------------------------------------------------------------- */

/** Pong thread replying long fpage messages. */
void __attribute__((noreturn))
PREFIX(pong_long_fpage_thread)(void)
{
  l4_umword_t ping_id32 = ping_id.raw;
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<rounds; i++)
    {
      /* map 1024 flexpages each round */
      l4_umword_t mem = scratch_mem + 1024*L4_PAGESIZE*i;
      int j;

      long_send_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_send_msg[i].send_dope = L4_IPC_DOPE(2048, 0);
      long_send_msg[i].dw[ 0] = mem;
      long_send_msg[i].dw[ 1] = l4_fpage(mem, L4_LOG2_PAGESIZE,
					 L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
      for (j=1; j<1024; j++)
	{
	  long_send_msg[i].dw[2*j  ] = long_send_msg[i].dw[2*j-2] + L4_PAGESIZE;
	  long_send_msg[i].dw[2*j+1] = long_send_msg[i].dw[2*j-1] + L4_PAGESIZE;
	}

      long_send_msg[i].dw[2*j  ] = 0; /* terminate */
      long_send_msg[i].dw[2*j+1] = 0; /* terminate */
    }

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  /* wait for first request from ping thread */
  PREFIX(recv)(ping_id);
  PREFIX(call)(ping_id);

  for (i=0; ; i++)
    {
      l4_umword_t dummy;
      asm volatile
	(
	 "push %%ebp        \n\t"
	 "mov  $1,%%ebp     \n\t"
	 "sub  %%ecx,%%ecx \n\t"
	 IPC_SYSENTER
	 "pop   %%ebp       \n\t"
	 : "=a" (dummy), "=b" (dummy), "=d" (dummy)
	 : "a" ((l4_umword_t)&long_send_msg[i] | 2),
	   "d" (long_send_msg[i].dw[0]), "b" (long_send_msg[i].dw[1]),
	   "D" (0), "S" (ping_id32)
	 : "ecx"
	 );
    }
}

/** Ping (send) thread expecting long fpage messages. */
void __attribute__((noreturn))
PREFIX(ping_long_fpage_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<rounds; i++)
    {
      long_recv_msg[i].fp = l4_fpage(0, L4_WHOLE_ADDRESS_SPACE,
				     L4_FPAGE_RW, L4_FPAGE_MAP);
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x55, NR_DWORDS*4);
    }

    {
      register l4_umword_t id32 = pong_id.raw;

      PREFIX(call)(pong_id);

      tsc = l4_rdtsc();
      for (i=0; i<rounds; i++)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push %%ebp        \n\t"
	     "mov  %%ebx,%%ebp  \n\t"
	     "sub  %%eax,%%eax  \n\t"
	     "sub  %%ecx,%%ecx  \n\t"
	     IPC_SYSENTER
	     "pop  %%ebp        \n\t"
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg[i]), "S" (id32), "D" (0)
	     : "eax", "ecx", "edx"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  printf("  %s %d fps (%d MB): %u cycles / %lu rounds a 1024 fpages "
         ">> %u/fp <<\n",
	 dont_do_cold ? "" : "warm",
         SCRATCH_MEM_SIZE/L4_PAGESIZE, SCRATCH_MEM_SIZE/(1024*1024),
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/(rounds*1024)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Ping (send) thread expecting long fpage messages. */
void __attribute__((noreturn))
PREFIX(ping_long_fpage_cold_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<rounds; i++)
    {
      long_recv_msg[i].fp = l4_fpage(0, L4_WHOLE_ADDRESS_SPACE,
				     L4_FPAGE_RW, L4_FPAGE_MAP);
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x55, NR_DWORDS*4);
    }

    {
      register l4_umword_t id32 = pong_id.raw;

      PREFIX(call)(pong_id);

      tsc = l4_rdtsc();
      for (i=0; i<rounds; i++)
	{
	  l4_umword_t dummy;
	  tsc -= l4_rdtsc();
	  flooder();
	  tsc += l4_rdtsc();
	  asm volatile 
	    (
	     "push %%ebp        \n\t"
	     "mov  %%ebx,%%ebp  \n\t"
	     "sub  %%eax,%%eax  \n\t"
	     "sub  %%ecx,%%ecx  \n\t"
	     IPC_SYSENTER
	     "pop  %%ebp        \n\t"
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg[i]), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
	    );
	}
      tsc = l4_rdtsc() - tsc;
    }

  printf("  cold %d fps (%d MB): %u cycles / %lu rounds a 1024 fpages "
         ">> %u/fp <<\n",
         SCRATCH_MEM_SIZE/L4_PAGESIZE, SCRATCH_MEM_SIZE/(1024*1024),
	 (l4_uint32_t)tsc, rounds, (l4_uint32_t)(tsc/(rounds*1024)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


/* ------------------------------------------------------------------------- */
/* Pagefault IPC (warm/cold)                                                 */
/* ------------------------------------------------------------------------- */

/** pager thread. */
void __attribute__((noreturn))
PREFIX(pong_pagefault_thread)(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  PREFIX(call)(main_id);

  PREFIX(recv)(ping_id);

  while (1)
    {
      register l4_umword_t dw0, dw1;
      register l4_umword_t ping_id32 = ping_id.raw;
      register l4_umword_t fp = l4util_log2(fpagesize) << 2;

      asm volatile
	(
	 "push %%ebp              \n\t"
	 "xorl %%eax,%%eax        \n\t"
	 "mov  $0x801,%%ecx        \n\t"
	 "mov  $1,%%ebp           \n\t"
	 IPC_SYSENTER
	 "pop  %%ebp              \n\t"
	 : "=d" (dw0), "=b" (dw1), "=S" (ping_id32)
	 : "S" (ping_id32), "D" (0)
	 : "eax", "ecx"
	 );

      while (1)
	{
	  dw0 &= ~(fpagesize-1);
	  dw1  = dw0 | fp;
	  asm volatile
	    (
	     "push %%ebp        \n\t"
	     "movl $2,%%eax     \n\t"
	     "mov  $0x801,%%ecx  \n\t"
	     "mov  $1,%%ebp     \n\t"
	     IPC_SYSENTER
	     "pop  %%ebp        \n\t"
	     : "=d" (dw0), "=b" (dw1), "=S" (ping_id32)
	     :  "d" (dw0),  "b" (dw1),  "S" (ping_id32), "D" (0)
	     : "eax", "ecx"
	     );
	}
    }
}

/** Raise pagefaults. */
void __attribute__((noreturn))
PREFIX(ping_pagefault_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  register l4_umword_t dw0 = scratch_mem;
  l4_umword_t dummy;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

    {
      /* change my pager to pong_pagefault_thread */
      l4_threadid_t preempter = L4_INVALID_ID, pager = pong_id;
      l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
			&preempter, &pager, &dummy, &dummy, &dummy);
    }

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      asm volatile
	(
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 : "=a" (dw0), "=c" (dummy)
	 :  "a" (dw0),  "c" (fpagesize)
	 : "ebx", "edx"
	);
    }
  tsc = l4_rdtsc() - tsc;
  
  printf("  %d%cB => %ld%cB: %10u cycles / %5lu rounds >> %6u <<\n",
         use_superpages ? L4_SUPERPAGESIZE/(1024*1024) : L4_PAGESIZE / 1024,
	 use_superpages ? 'M' : 'k',
         fpagesize > 1024*1024 ? fpagesize/(1024*1024) : fpagesize / 1024,
	 fpagesize > 1024*1024 ? 'M' : 'k',
	 (l4_uint32_t)tsc, rounds*8, (l4_uint32_t)(tsc/(rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/*
 * Syscall performance
 */

void __attribute__((noreturn))
PREFIX(syscall_id_nearest_thread)(void)
{
  int i;
  l4_cpu_time_t tsc;
  l4_threadid_t me;
  l4_umword_t dummy;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(call)(pong_id);

  tsc = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 WHATTODO_SYSCALL_L4_MYSELF
	 "pop   %%ebp    \n\t"
	 :
	 :
	 : "eax", "ebx", "ecx", "edx", "edi", "esi"
	);
    }
  tsc = l4_rdtsc() - tsc;

  printf(" l4_myself() %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" :
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds,
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  me = l4_myself();

  tsc = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 WHATTODO_SYSCALL_L4_NCHIEF
	 "pop   %%ebp    \n\t"
	 : "=b" (dummy)
	 : "b" (me.raw)
	 : "eax", "ecx", "edx", "edi", "esi"
	);
    }
  tsc = l4_rdtsc() - tsc;

  printf(" l4_nchief() %s%s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" :
	 (callmode == 2) ? "kipcalls" : "   int30",
	 dont_do_cold ? "" : "/warm",
	 (l4_uint32_t)tsc, 8*global_rounds,
	 (l4_uint32_t)(tsc/(8*global_rounds)));

  /* finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);
  l4_sleep_forever();
}


#include <l4/rmgr/librmgr.h>

/* Normal code/data distribution */
static char task_stack_normal[1024];
static void syscall_task_new_test_task_normal(void)
{
  PREFIX(send)(ping_id);
  l4_sleep_forever();
}

extern char PREFIX(task_stack_compact)[1024];
extern l4_threadid_t PREFIX(task_ping_id_compact);
extern char PREFIX(task_new_page_start)[];
extern char PREFIX(task_new_page_end)[];
void PREFIX(syscall_task_new_test_task_compact)(void);

static void print_result(char *testname, unsigned long rounds,
                         l4_cpu_time_t tsc)
{
  printf(" %s  %s: %10u cycles / %6lu rounds >> %5u <<\n",
         (callmode == 1) ? "sysenter" :
	 (callmode == 2) ? "kipcalls" : "   int30",
	 testname,
	 (l4_uint32_t)tsc, rounds,
	 (l4_uint32_t)(tsc/rounds));
}

void __attribute__((noreturn))
PREFIX(syscall_task_new_thread)(void)
{
  l4_threadid_t task = L4_NIL_ID;
  l4_threadid_t me = l4_myself();
  l4_umword_t stack;
  l4_cpu_time_t tsc, tsc_sum;
  const l4_umword_t rounds = 1024;
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  PREFIX(task_ping_id_compact) = ping_id;

  task.id.lthread = 0;
  task.id.task    = 38;
  if (rmgr_get_task(task.id.task))
    printf("rmgr_get_task(%d) failed\n", task.id.task);

  stack = (l4_umword_t)task_stack_normal + sizeof(task_stack_normal);

  /* First check setup */
  task = l4_task_new(task, 255, stack,
                     (l4_umword_t)syscall_task_new_test_task_normal, pager_id);
  PREFIX(recv)(task);
  if (l4_is_nil_id(task))
    printf("Error creating task\n");

  /* check that compact code is really within one page */
  if (PREFIX(task_new_page_end) - PREFIX(task_new_page_start) > L4_PAGESIZE)
    printf("compact task_new run exceeds one page!\n");



  /* First test: normal */
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)syscall_task_new_test_task_normal, pager_id);

      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;

      l4_task_new(task, (unsigned)me.raw, 0, 0, L4_NIL_ID);
    }
  print_result("normal  up-only", rounds, tsc_sum);

  /* normal also with task destruction */
  l4_task_new(task, 255, stack,
              (l4_umword_t)syscall_task_new_test_task_normal, pager_id);
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)syscall_task_new_test_task_normal, pager_id);

      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;
    }
  print_result("normal  up+down", rounds, tsc_sum);

  /* Next test: compact */
  stack = (l4_umword_t)PREFIX(task_stack_compact)
          + sizeof(PREFIX(task_stack_compact));
  /* Pager maps writable upon first PF so that we don't catch a second
   * when we write the stack */
  pager_always_map_writable = 1;
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact),
                  pager_id);

      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;

      l4_task_new(task, (unsigned)me.raw, 0, 0, L4_NIL_ID);
    }
  pager_always_map_writable = 0;
  print_result("compact up-only", rounds, tsc_sum);

  /* Next test: compact with task destruction */
   l4_task_new(task, 255, stack,
               (l4_umword_t)PREFIX(syscall_task_new_test_task_compact),
               pager_id);
  pager_always_map_writable = 1;
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact),
                  pager_id);

      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;
    }
  pager_always_map_writable = 0;
  print_result("compact up+down", rounds, tsc_sum);



  /* Next test: start with exception */
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255 | L4_TASK_NEW_RAISE_EXCEPTION, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;

      l4_task_new(task, (unsigned)me.raw, 0, 0, L4_NIL_ID);
    }
  print_result("raise   up-only", rounds, tsc_sum);

  /* Next test: start with exception and destruction */
  l4_task_new(task, 255 | L4_TASK_NEW_RAISE_EXCEPTION, stack,
              (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255 | L4_TASK_NEW_RAISE_EXCEPTION, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
      PREFIX(recv)(task);
      tsc_sum += l4_rdtsc() - tsc;
    }
  print_result("raise   up+down", rounds, tsc_sum);

  /* Next test: pure without interaction of new task */
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
      tsc_sum += l4_rdtsc() - tsc;

      l4_task_new(task, (unsigned)me.raw, 0, 0, L4_NIL_ID);
    }
  print_result("pure    up-only", rounds, tsc_sum);

  /* Next test: pure without interaction of new task and with desctruction */
  l4_task_new(task, 255, stack,
              (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
  tsc_sum = 0;
  for (i = rounds; i; i--)
    {
      tsc = l4_rdtsc();
      l4_task_new(task, 255, stack,
                  (l4_umword_t)PREFIX(syscall_task_new_test_task_compact), me);
      tsc_sum += l4_rdtsc() - tsc;
    }
  print_result("pure    up+down", rounds, tsc_sum);



  /* Done */
  if (rmgr_free_task(task.id.task))
    printf("rmgr_free_task(%d) failed\n", task.id.task);

  /* finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);
  l4_sleep_forever();
}
