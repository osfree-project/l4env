#ifndef L4_SYSCALLS_EXTENSIONS_H
#define L4_SYSCALLS_EXTENSIONS_H

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#define RT_ADD_TIMESLICE	1
#define RT_REM_TIMESLICES	2
#define RT_SET_PERIOD		3
#define RT_BEGIN_PERIODIC	4
#define RT_BEGIN_PERIODIC_NS	5
#define RT_END_PERIODIC		6

#define ABS_RECV_TIMEOUT	0x1
#define ABS_SEND_TIMEOUT	0x2
#define ABS_RECV_CLOCK		0x4
#define ABS_SEND_CLOCK		0x8
#define NEXT_PERIOD		0x10
#define PREEMPTION_ID		0x20

L4_INLINE void
l4_abs_recv_timeout (unsigned long long clock,
                     enum l4_timeout_abs_validity validity,
                     l4_timeout_t *timeout, l4_threadid_t *id);

L4_INLINE void
l4_abs_send_timeout (unsigned long long clock,
                     enum l4_timeout_abs_validity validity,
                     l4_timeout_t *timeout, l4_threadid_t *id);

L4_INLINE
void
l4_abs_recv_timeout (unsigned long long clock,
                     enum l4_timeout_abs_validity validity,
                     l4_timeout_t *timeout, l4_threadid_t *id)
{
  unsigned m = clock >> validity;
  
  if (m >> 8 & 1)
    id->id.chief |= ABS_RECV_CLOCK;
  else
    id->id.chief &= ~ABS_RECV_CLOCK;
  
  id->id.chief |= ABS_RECV_TIMEOUT;

  timeout->to.rcv_exp = 15 - validity;
  timeout->to.rcv_man = m & 0xff;
}

L4_INLINE
void
l4_abs_send_timeout (unsigned long long clock,
                     enum l4_timeout_abs_validity validity,
                     l4_timeout_t *timeout, l4_threadid_t *id)
{
  unsigned m = clock >> validity;
  
  if (m >> 8 & 1)
    id->id.chief |= ABS_SEND_CLOCK;
  else
    id->id.chief &= ~ABS_SEND_CLOCK;
  
  id->id.chief |= ABS_SEND_TIMEOUT;

  timeout->to.snd_exp = 15 - validity;
  timeout->to.snd_man = m & 0xff;
}

#endif
