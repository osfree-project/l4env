#ifndef __DICE_L4_X0_H__
#define __DICE_L4_X0_H__

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include "dice/dice-l4_common.h"

#ifndef CORBA_Object_typedef
#define CORBA_Object_typedef
typedef l4_threadid_t CORBA_Object;
#endif

#include "dice/dice-env_types.h"

#ifndef CORBA_Environment_typedef
#define CORBA_Environment_typedef
typedef struct CORBA_Environment
{
  COMMON_ENVIRONMENT;
  l4_timeout_t timeout;
  l4_fpage_t rcv_fpage;
  l4_uint32_t ipc_error;
  void* user_data;
  void* (*malloc)(unsigned long);
} CORBA_Environment;
#endif

#include "dice/dice-env_functions.h"

#define dice_default_environment { CORBA_NO_EXCEPTION, 0, 0, L4_IPC_NEVER, \
      { fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, 0, 0, \
      0 }

/*
 * defines for L4 types for test-environment
 * L4 types are l4_strdope_t and l4_fpage_t
 */
#define random_l4_strdope_t (l4_strdope_t){ 0, 0, 0, 0 }
#define random_l4_fpage_t (l4_fpage_t)({ fpage: 0 })

/*
 * defines the maximum number of elements an array might have; this are elements, _NOT_ bytes
 * Thus CORBA_alloc should be able to allocate not only this amount of bytes, but also a 
 * multiple of this
 */
#define MAX_ARRAY_SIZE 512
/*
 * defines the maximum string length for indirect strings, This value is used if a server
 * loop has to allocate memory for a receive buffer itself.
 */
#define MAX_STRING_SIZE 1024

/*
 * defines some values and macros for the default function
 */
#define REPLY    1
#define NO_REPLY 2

#define GET_DWORD(buf, pos)   *((l4_umword_t*)(&(buf->_bytes[pos*4])))
#define GET_STRING(buf, pos)  (buf->_strings[pos]).rcv_str

#define UNMARSHAL_DWORD(buf, var, pos)  var=GET_DWORD(buf, pos)
#define MARSHAL_DWORD(buf, var, pos)    GET_DWORD(buf, pos)=var

#define UNMARSHAL_STRING(buf, ptr, pos) ptr=GET_STRING(buf, pos)
#define MARSHAL_STRING(buf, ptr, pos)   (buf->_strings[pos]).snd_str=ptr; \
                                        (buf->_strings[pos]).snd_size=strlen(ptr)

#define UNMARSHAL_FPAGE(buf, snd_fpage, pos) (snd_fpage).snd_base=GET_DWORD(buf,pos); \
                                             (snd_fpage).fpage=GET_DWORD(buf,pos+1)
#define MARSHAL_FPAGE(buf, snd_fpage, pos)   GET_DWORD(buf,pos)=(l4_umword_t)(snd_fpage).snd_base; \
                                             GET_DWORD(buf,pos+1)=(l4_umword_t)(snd_fpage).fpage.fpage
#define MARSHAL_ZERO_FPAGE(buf, pos)         GET_DWORD(buf,pos)=0; \
					     GET_DWORD(buf,pos+1)=0

#define GET_DWORD_COUNT(buf)          (buf->send.md.dwords)
#define GET_STRING_COUNT(buf)         (buf->send.md.dwords)
#define SET_DWORD_COUNT(buf, count)   buf->send.md.dwords = count
#define SET_STRING_COUNT(buf, count)  buf->send.md.strings = count
#define SET_SHORTIPC_COUNT(buf)       SET_DWORD_COUNT(buf,2); SET_STRING_COUNT(buf,0)
#define SET_SEND_FPAGE(buf)           buf->send.md.fpage_received = 1					     

#endif // __DICE_L4_X0_H__
