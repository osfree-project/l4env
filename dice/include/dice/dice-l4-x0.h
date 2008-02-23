#ifndef __DICE_L4_X0_H__
#define __DICE_L4_X0_H__

#include <l4/l4.h>
#if defined(ARCH_x86)
#include <l4/x86/kdebug.h>
#else
#error unsupported architecture
#endif

#if defined(DICE_TRACE_SERVER) || defined(DICE_TRACE_CLIENT) || defined(DICE_TRACE_MSGBUF)
#include <l4io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

DICE_INLINE
void* malloc_warning(unsigned long size)
{
  enter_kdebug("malloc");
  return 0;
};

DICE_INLINE
void free_warning(void* addr)
{
  enter_kdebug("free");
};

#ifdef __cplusplus
}
#endif

#ifndef L4_IPC_DOPE
#define L4_IPC_DOPE(dwords, strings) \
	( (l4_msgdope_t) {md: {0, 0, 0, 0, 0, strings, dwords }})
#endif

#ifndef L4_IPC_SHORT_FPAGE
#define L4_IPC_SHORT_FPAGE			((void*)2)
#endif

#ifndef L4_IPC_SHORT_MSG
#define L4_IPC_SHORT_MSG			((void*)0)
#endif

/* error codes */
#ifndef L4_IPC_SECANCELED
#define L4_IPC_SECANCELED   0x50
#endif
#ifndef L4_IPC_SEABORTED
#define L4_IPC_SEABORTED	0xB0
#endif

#ifdef __cplusplus
#define dice_default_environment CORBA_Environment()
#define dice_default_server_environment CORBA_Server_Environment()
#else
#define dice_default_environment \
    { { _corba: { major: CORBA_NO_EXCEPTION, repos_id: 0} }, \
	{ param: 0 }, L4_IPC_NEVER, \
	{ fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, \
	malloc_warning, free_warning }
#define dice_default_server_environment \
    { { _corba: { major: CORBA_NO_EXCEPTION, repos_id: 0} }, \
	{ param: 0 }, L4_IPC_TIMEOUT(0,1,0,0,0,0), \
	{ fp: { 1, 1, L4_WHOLE_ADDRESS_SPACE, 0, 0 } }, \
	malloc_warning, free_warning, L4_INVALID_ID_INIT, \
	    0, { 0,0,0,0,0, 0,0,0,0,0}, 0 }
#endif

#ifdef __cplusplus
namespace dice
{
    DICE_EXTERN_INLINE
    CORBA_Environment::CORBA_Environment()
    : _exception(),
      _p(),
      timeout(),
      rcv_fpage(),
      malloc(malloc_warning),
      free(free_warning)
    {
	_exception._corba.major = CORBA_NO_EXCEPTION;
	_exception._corba.repos_id = CORBA_DICE_EXCEPTION_NONE;
	_p.param = 0;
	timeout = L4_IPC_NEVER;
	rcv_fpage.fp.grant = 1;
    	rcv_fpage.fp.write = 1;
	rcv_fpage.fp.size = L4_WHOLE_ADDRESS_SPACE;
	rcv_fpage.fp.zero = 0;
	rcv_fpage.fp.page = 0;
    }
    
    DICE_EXTERN_INLINE
    CORBA_Server_Environment::CORBA_Server_Environment()
    : _exception(),
      _p(),
      timeout(),
      rcv_fpage(),
      malloc(malloc_warning),
      free(free_warning),
      partner(L4_INVALID_ID),
      user_data(0),
      ptrs_cur(0)
    {
	_exception._corba.major = CORBA_NO_EXCEPTION;
	_exception._corba.repos_id = CORBA_DICE_EXCEPTION_NONE;
	_p.param = 0;
	timeout = L4_IPC_TIMEOUT(0,1,0,0,0,0);
	rcv_fpage.fp.grant = 1;
	rcv_fpage.fp.write = 1;
	rcv_fpage.fp.size = L4_WHOLE_ADDRESS_SPACE;
	rcv_fpage.fp.zero = 0;
	rcv_fpage.fp.page = 0;
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = 0;
    }

    DICE_EXTERN_INLINE
    CORBA_Server_Environment::CORBA_Server_Environment(const CORBA_Server_Environment& src)
    : _exception(src._exception),
	_p(src._p),
	timeout(src.timeout),
	rcv_fpage(src.rcv_fpage),
	malloc(src.malloc),
	free(src.free),
	partner(src.partner),
	user_data(src.user_data),
	ptrs_cur(src.ptrs_cur)
    {
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = src.ptrs[i];
    }

    DICE_EXTERN_INLINE CORBA_Server_Environment&
    CORBA_Server_Environment::operator=(const CORBA_Server_Environment& src)
    {
	_exception = src._exception;
	_p = src._p;
	timeout = src.timeout;
	rcv_fpage = src.rcv_fpage;
	malloc = src.malloc;
	free = src.free;
	partner = src.partner;
	user_data = src.user_data;
	ptrs_cur = src.ptrs_cur;
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = src.ptrs[i];
	return *this;
    }
}
#endif

/*
 * defines for L4 types for test-environment
 * L4 types are l4_strdope_t and l4_fpage_t
 */
#define random_l4_strdope_t (l4_strdope_t){ 0, 0, 0, 0 }
#define random_l4_fpage_t (l4_fpage_t)({ fpage: 0 })

/*
 * defines some values and macros for the default function
 */

#define DICE_GET_DWORD(buf, pos)   ((buf)->_word._word[pos])
#define DICE_GET_STRING(buf, pos)  ((buf)->_word._strings[pos]).rcv_str
#define DICE_GET_STRSIZE(buf, pos) ((buf)->_word._strings[pos]).rcv_size

#define DICE_UNMARSHAL_DWORD(buf, var, pos)  var=DICE_GET_DWORD(buf, pos)
#define DICE_MARSHAL_DWORD(buf, var, pos)    DICE_GET_DWORD(buf, pos)=var

#define DICE_UNMARSHAL_STRING(buf, ptr, size, pos) do { \
    ptr=DICE_GET_STRING(buf, pos); \
    size=DICE_GET_STRSIZE(buf, pos); \
    } while (0)
#define DICE_MARSHAL_STRING(buf, ptr, size, pos)   do { \
    ((buf)->_word._strings[pos]).snd_str=ptr; \
    ((buf)->_word._strings[pos]).snd_size=size; \
    } while (0)

#define DICE_UNMARSHAL_FPAGE(buf, snd_fpage, pos) do { \
    (snd_fpage).snd_base=DICE_GET_DWORD(buf,pos); \
    (snd_fpage).fpage.raw=DICE_GET_DWORD(buf,pos+1); \
    } while (0)
#define DICE_MARSHAL_FPAGE(buf, snd_fpage, pos)   do { \
    DICE_GET_DWORD(buf,pos)=(l4_umword_t)(snd_fpage).snd_base; \
    DICE_GET_DWORD(buf,pos+1)=(l4_umword_t)(snd_fpage).fpage.fpage; \
    } while (0)
#define DICE_MARSHAL_ZERO_FPAGE(buf, pos)         do { \
    DICE_GET_DWORD(buf,pos)=0; \
    DICE_GET_DWORD(buf,pos+1)=0; \
    } while (0)

#define DICE_SEND_DOPE(buf)             (buf)->_word._dice_send_dope
#define DICE_SIZE_DOPE(buf)             (buf)->_word._dice_size_dope
#define DICE_GET_DWORD_COUNT(buf)       (DICE_SEND_DOPE(buf).md.dwords)
#define DICE_GET_STRING_COUNT(buf)      (DICE_SEND_DOPE(buf).md.strings)
#define DICE_SET_ZERO_COUNT(buf)        do { \
    DICE_SEND_DOPE(buf).md.dwords = 0; \
    DICE_SEND_DOPE(buf).md.strings = 0; \
    DICE_SEND_DOPE(buf).md.fpage_received = 0; \
    } while (0)
#define DICE_SET_DWORD_COUNT(buf, cnt)     DICE_SEND_DOPE(buf).md.dwords = cnt
#define DICE_SET_STRING_COUNT(buf, cnt)    DICE_SEND_DOPE(buf).md.strings = cnt
#define DICE_SET_SHORTIPC_COUNT(buf)     do { \
    DICE_SET_DWORD_COUNT(buf,2); \
    DICE_SET_STRING_COUNT(buf,0); \
    DICE_SEND_DOPE(buf).md.fpage_received = 0; \
    } while (0)
#define DICE_SET_SEND_FPAGE(buf)     DICE_SEND_DOPE(buf).md.fpage_received = 1					     

#endif // __DICE_L4_X0_H__
