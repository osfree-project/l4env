#ifndef __DICE_L4_V2_H__
#define __DICE_L4_V2_H__

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include "dice/dice-common.h"
#include "dice/dice-env_functions.h"

static inline
void* malloc_warning(unsigned long size)
{
  enter_kdebug("malloc");
  return 0;
};

static inline
void free_warning(void* addr)
{
  enter_kdebug("free");
};

#define dice_default_environment \
  { CORBA_NO_EXCEPTION, 0, 0, L4_IPC_NEVER_INITIALIZER, \
    { fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, 0, 0, \
    malloc_warning, free_warning }
#define dice_default_server_environment \
  { CORBA_NO_EXCEPTION, 0, 0, L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0), \
    { fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, 0, 0, \
    malloc_warning, free_warning }

/*
 * defines for L4 types for test-environment
 * L4 types are l4_strdope_t and l4_fpage_t
 */
#define random_l4_strdope_t (l4_strdope_t){ 0, 0, 0, 0 }
#define random_l4_fpage_t (l4_fpage_t)({ fpage: 0 })

/*
 * defines some values and macros for the default function
 */
#define DICE_REPLY    1
#define DICE_NO_REPLY 2

#define DICE_GET_DWORD(buf, pos)   *((l4_umword_t*)(&((buf)->_bytes[pos*4])))
#define DICE_GET_STRING(buf, pos)  ((buf)->_strings[pos]).rcv_str
#define DICE_GET_STRSIZE(buf, pos) ((buf)->_strings[pos]).rcv_size

#define DICE_UNMARSHAL_DWORD(buf, var, pos)  var=DICE_GET_DWORD(buf, pos)
#define DICE_MARSHAL_DWORD(buf, var, pos)    DICE_GET_DWORD(buf, pos)=var

#define DICE_UNMARSHAL_STRING(buf, ptr, size, pos) ptr=DICE_GET_STRING(buf, pos); \
                                                   size=DICE_GET_STRSIZE(buf, pos)
#define DICE_MARSHAL_STRING(buf, ptr, size, pos)   ((buf)->_strings[pos]).snd_str=ptr; \
                                                   ((buf)->_strings[pos]).snd_size=size

#define DICE_UNMARSHAL_FPAGE(buf, snd_fpage, pos) (snd_fpage).snd_base=DICE_GET_DWORD(buf,pos); \
                                                  (snd_fpage).fpage=DICE_GET_DWORD(buf,pos+1)
#define DICE_MARSHAL_FPAGE(buf, snd_fpage, pos)   DICE_GET_DWORD(buf,pos)=(l4_umword_t)(snd_fpage).snd_base; \
                                                  DICE_GET_DWORD(buf,pos+1)=(l4_umword_t)(snd_fpage).fpage.fpage
#define DICE_MARSHAL_ZERO_FPAGE(buf, pos)         DICE_GET_DWORD(buf,pos)=0; \
					          DICE_GET_DWORD(buf,pos+1)=0

#define DICE_GET_DWORD_COUNT(buf)                 ((buf)->send.md.dwords)
#define DICE_GET_STRING_COUNT(buf)                ((buf)->send.md.strings)
#define DICE_SET_ZERO_COUNT(buf)                  (buf)->send.md.dwords = 0; \
						  (buf)->send.md.strings = 0; \
						  (buf)->send.md.fpage_received = 0
#define DICE_SET_DWORD_COUNT(buf, count)          (buf)->send.md.dwords = count
#define DICE_SET_STRING_COUNT(buf, count)         (buf)->send.md.strings = count
#define DICE_SET_SHORTIPC_COUNT(buf)              DICE_SET_DWORD_COUNT(buf,2); \
                                                  DICE_SET_STRING_COUNT(buf,0); \
						  (buf)->send.md.fpage_received = 0
#define DICE_SET_SEND_FPAGE(buf)                  (buf)->send.md.fpage_received = 1					     

#endif // __DICE_L4_V2_H__
