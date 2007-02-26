/* 
 * $Id$
 */

#ifndef __L4_IPC_H__ 
#define __L4_IPC_H__ 

/*
 * L4 ipc
 */

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

/*
 * IPC parameters
 */


/* 
 * Structure used to describe destination and true source if a chief
 * wants to deceit 
 */

typedef struct {
  l4_threadid_t dest, true_src;
} l4_ipc_deceit_ids_t;



/* 
 * Defines used for Parameters 
 */

#define L4_IPC_SHORT_MSG 	0

/*
 * Defines used to build Parameters
 */

#define L4_IPC_STRING_SHIFT 8
#define L4_IPC_DWORD_SHIFT 13
#define L4_IPC_SHORT_FPAGE ((void *)2)

#define L4_IPC_DOPE(dwords, strings) \
( (l4_msgdope_t) {md: {0, 0, 0, 0, 0, 0, strings, dwords }})


#define L4_IPC_TIMEOUT(snd_man, snd_exp, rcv_man, rcv_exp, snd_pflt, rcv_pflt)\
     ( (l4_timeout_t) \
       {to: { rcv_exp, snd_exp, rcv_pflt, snd_pflt, snd_man, rcv_man } } )

#define L4_IPC_NEVER ((l4_timeout_t) {timeout: 0})
#define L4_IPC_MAPMSG(address, size)  \
     ((void *)(l4_umword_t)( ((address) & L4_PAGEMASK) | ((size) << 2) \
			 | (unsigned long)L4_IPC_SHORT_FPAGE)) 

#define L4_IPC_IOMAPMSG(port, iosize)  \
     ((void *)(l4_umword_t)( 0xf0000000 | ((port) << 12) | ((iosize) << 2) \
			 | (unsigned long)L4_IPC_SHORT_FPAGE)) 

/* 
 * Some macros to make result checking easier
 */

#define L4_IPC_ERROR_MASK 	0xF0
#define L4_IPC_DECEIT_MASK	0x01
#define L4_IPC_FPAGE_MASK	0x02
#define L4_IPC_REDIRECT_MASK	0x04
#define L4_IPC_SRC_MASK		0x08
#define L4_IPC_SND_ERR_MASK	0x10

#define L4_IPC_IS_ERROR(x)		(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_MSG_DECEITED(x) 		(((x).msgdope) & L4_IPC_DECEIT_MASK)
#define L4_IPC_MSG_REDIRECTED(x)	(((x).msgdope) & L4_IPC_REDIRECT_MASK)
#define L4_IPC_SRC_INSIDE(x)		(((x).msgdope) & L4_IPC_SRC_MASK)
#define L4_IPC_SND_ERROR(x)		(((x).msgdope) & L4_IPC_SND_ERR_MASK)
#define L4_IPC_MSG_TRANSFER_STARTED \
				((((x).msgdope) & L4_IPC_ERROR_MASK) < 5)

/*
 * Prototypes
 */

L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, 
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   l4_umword_t snd_dword0, 
			   l4_umword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, 
			   l4_umword_t *rcv_dword0, 
			   l4_umword_t *rcv_dword1, 
			   l4_timeout_t timeout, 
			   l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_reply_deceiting_and_wait(const l4_ipc_deceit_ids_t ids,
				     const void *snd_msg, 
				     l4_umword_t snd_dword0, 
				     l4_umword_t snd_dword1,
				     l4_threadid_t *src,
				     void *rcv_msg, 
				     l4_umword_t *rcv_dword0, 
				     l4_umword_t *rcv_dword1, 
				     l4_timeout_t timeout, 
				     l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg,
		 l4_umword_t snd_dword0, 
		 l4_umword_t snd_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_send_deceiting(l4_ipc_deceit_ids_t ids,
			   const void *snd_msg, 
			   l4_umword_t snd_dword0, 
			   l4_umword_t snd_dword1, 
			   l4_timeout_t timeout, 
			   l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 l4_umword_t *rcv_dword0, 
		 l4_umword_t *rcv_dword1, 
		 l4_timeout_t timeout, 
		 l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, 
		    l4_umword_t *rcv_dword0, 
		    l4_umword_t *rcv_dword1, 
		    l4_timeout_t timeout, 
		    l4_msgdope_t *result);


L4_INLINE int l4_ipc_fpage_received(l4_msgdope_t msgdope);
L4_INLINE int l4_ipc_is_fpage_granted(l4_fpage_t fp);
L4_INLINE int l4_ipc_is_fpage_writable(l4_fpage_t fp);

/* IPC bindings for chiefs */

L4_INLINE int
l4_i386_ipc_chief_wait(l4_threadid_t *src, 
		       l4_threadid_t *real_dst,
		       void *rcv_msg, 
		       l4_umword_t *rcv_dword0, 
		       l4_umword_t *rcv_dword1, 
		       l4_timeout_t timeout, 
		       l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_receive(l4_threadid_t src, 
			  l4_threadid_t *real_dst,
			  void *rcv_msg, 
			  l4_umword_t *rcv_dword0, 
			  l4_umword_t *rcv_dword1, 
			  l4_timeout_t timeout, 
			  l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_call(l4_threadid_t dest, 
		       l4_threadid_t fake_src,
		       const void *snd_msg, 
		       l4_umword_t snd_dword0, 
		       l4_umword_t snd_dword1, 
		       l4_threadid_t *real_dst,
		       void *rcv_msg, 
		       l4_umword_t *rcv_dword0, 
		       l4_umword_t *rcv_dword1, 
		       l4_timeout_t timeout, 
		       l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_reply_and_wait(l4_threadid_t dest, 
				 l4_threadid_t fake_src,
				 const void *snd_msg, 
				 l4_umword_t snd_dword0, 
				 l4_umword_t snd_dword1, 
				 l4_threadid_t *src,
				 l4_threadid_t *real_dst,
				 void *rcv_msg, 
				 l4_umword_t *rcv_dword0, 
				 l4_umword_t *rcv_dword1, 
				 l4_timeout_t timeout, 
				 l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_send(l4_threadid_t dest, 
		       l4_threadid_t fake_src,
		       const void *snd_msg, 
		       l4_umword_t snd_dword0, 
		       l4_umword_t snd_dword1, 
		       l4_timeout_t timeout, 
		       l4_msgdope_t *result);





/*
 *
 */

L4_INLINE int l4_ipc_fpage_received(l4_msgdope_t msgdope)
{
  return msgdope.md.fpage_received != 0;
}
L4_INLINE int l4_ipc_is_fpage_granted(l4_fpage_t fp)
{
  return fp.fp.grant != 0;
}
L4_INLINE int l4_ipc_is_fpage_writable(l4_fpage_t fp)
{
  return fp.fp.write != 0;
}

/*
 * IPC results
 */

#define L4_IPC_ERROR(x)			(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_ENOT_EXISTENT		0x10
#define L4_IPC_RETIMEOUT		0x20
#define L4_IPC_SETIMEOUT		0x30
#define L4_IPC_RECANCELED		0x40
#define L4_IPC_SECANCELED		0x50
#define L4_IPC_REMAPFAILED		0x60
#define L4_IPC_SEMAPFAILED		0x70
#define L4_IPC_RESNDPFTO		0x80
#define L4_IPC_SESNDPFTO		0x90
#define L4_IPC_RERCVPFTO		0xA0
#define L4_IPC_SERCVPFTO		0xB0
#define L4_IPC_REABORTED		0xC0
#define L4_IPC_SEABORTED		0xD0
#define L4_IPC_REMSGCUT			0xE0
#define L4_IPC_SEMSGCUT			0xF0


/*
 * Internal defines used to build IPC parameters for the L4 kernel
 */

#define L4_IPC_NIL_DESCRIPTOR 	(-1)
#define L4_IPC_DECEIT 		1
#define L4_IPC_OPEN_IPC 	1


/*
 * Implementation
 */

#ifndef CONFIG_L4_CALL_SYSCALLS

# ifndef L4X0_IPC_SYSENTER
# define IPC_SYSENTER \
  "int    $0x30          \n\t"
# else
# define IPC_SYSENTER \
  "push   %%ecx          \n\t" \
  "push   %%ebp          \n\t" \
  "push   $0x1b          \n\t" \
  "push   $0f            \n\t" \
  "mov    %%esp,%%ecx    \n\t" \
  "sysenter              \n\t" \
  "mov    %%ebp,%%edx    \n\t" \
  "0:                    \n\t"
# endif

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#   define IPC_SYSENTER \
  "call __L4_ipc_direct  \n\t"
# else
#   define IPC_SYSENTER \
  "call *__L4_ipc        \n\t"
# endif

#endif

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)

#  if GCC_VERSION < 295
#    error gcc >= 2.95 required
#  elif GCC_VERSION < 302
#    ifdef __PIC__
#      include "ipc-l4x0adapt-gcc295-pic.h"
#    else
#      ifdef PROFILE
#        error "PROFILE support for X.0 API not supported"
#      else
#        include "ipc-l4x0adapt-gcc295-nopic.h"
#      endif
#    endif
#  else
#    ifdef __PIC__
#      include "ipc-l4x0adapt-gcc295-pic.h"
#    else
#      ifdef PROFILE
#        error "PROFILE support for X.0 API not supported"
#      else
#        include "ipc-l4x0adapt-gcc3-nopic.h"
#      endif
#    endif
#  endif

#endif

