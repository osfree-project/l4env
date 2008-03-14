#ifndef __DICE_L4_V4_H__
#define __DICE_L4_V4_H__

#include <l4/types.h>
#include <l4/ipc.h>
#include <l4/message.h>
#include <l4/kdebug.h>

#ifdef __cplusplus
extern "C" {
#endif

DICE_INLINE
void* malloc_warning(unsigned long size)
{
    L4_KDB_Enter("malloc");
    return 0;
};

DICE_INLINE
void free_warning(void* addr)
{
    L4_KDB_Enter("free");
};

#ifdef __cplusplus
}
#endif

#define dice_default_environment \
  { { _corba: { major: CORBA_NO_EXCEPTION, repos_id: 0} }, \
      { param: 0 }, L4_Never, L4_CompleteAddressSpace, \
      malloc_warning, free_warning }
#define dice_default_server_environment \
  { { _corba: { major: CORBA_NO_EXCEPTION, repos_id: 0} }, \
      { param: 0 }, L4_ZeroTime, L4_CompleteAddressSpace, \
      malloc_warning, free_warning, L4_anythread, \
	  0, { 0,0,0,0,0, 0,0,0,0,0}, 0 }

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
	timeout = L4_Never;
	rcv_fpage = L4_CompleteAddressSpace;
    }

    DICE_EXTERN_INLINE
    CORBA_Server_Environment::CORBA_Server_Environment()
    : _exception(),
      _p(),
      timeout(),
      rcv_fpage(),
      malloc(malloc_warning),
      free(free_warning),
      partner(),
      user_data(0),
      ptrs_cur(0)
    {
	_exception._corba.major = CORBA_NO_EXCEPTION;
	_exception._corba.repos_id = CORBA_DICE_EXCEPTION_NONE;
	_p.param = 0;
	timeout = L4_ZeroTime;
	rcv_fpage = L4_CompleteAddressSpace;
	partner = L4_anythread;
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = 0;
    }
}
#endif

/*
 * defines for L4 types for test-environment
 * L4 types are l4_strdope_t and l4_fpage_t
 */
#define random_l4_strdope_t (L4_StringItem_t){ raw: { 0, 0 } }
#define random_l4_fpage_t L4_Nilpage

/*
 * defines some values and macros for the default function
 */

#define DICE_GET_DWORD(buf, pos)   ((L4_Msg_t*)buf)->msg[pos + 1]
#define DICE_GET_DWORD_COUNT(buf)  ((L4_Msg_t*)buf)->tag.X.u
#define DICE_SET_DWORD_COUNT(buf, cnt)     ((L4_Msg_t*)buf)->tag.X.u = cnt
#define DICE_SET_STRING_COUNT(buf, cnt)    ((L4_Msg_t*)buf)->tag.X.t = cnt

#endif // __DICE_L4_V4_H__
