#ifndef __DICE_L4_V4_H__
#define __DICE_L4_V4_H__

#include <l4/types.h>
#include <l4/ipc.h>
#include <l4/message.h>
#include <l4/kdebug.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#define dice_default_environment \
  { CORBA_NO_EXCEPTION, 0, { param: 0 }, L4_Never, \
    L4_CompleteAddressSpace, \
    malloc_warning, free_warning }
#define dice_default_server_environment \
  { CORBA_NO_EXCEPTION, 0, { param: 0 }, L4_ZeroTime, \
    L4_CompleteAddressSpace, \
    malloc_warning, free_warning, \
    0, { 0,0,0,0,0, 0,0,0,0,0}, 0 }

#ifdef __cplusplus
namespace dice
{
    extern inline
    CORBA_Environment::CORBA_Environment()
    {
	major = 0;
	repos_id = 0;
	_p.param = 0;
	timeout = L4_Never;
	rcv_fpage = L4_CompleteAddressSpace;
	malloc = malloc_warning;
	free = free_warning;
    }

    extern inline
    CORBA_Server_Environment::CORBA_Server_Environment()
    {
	major = 0;
	repos_id = 0;
	_p.param = 0;
	timeout = L4_Never;
	rcv_fpage = L4_CompleteAddressSpace;
	malloc = malloc_warning;
	free = free_warning;
	user_data = 0;
	for (int i=0; i < DICE_PTRS_MAX; i++)
	    ptrs[i] = 0;
	ptrs_cur = 0;
    }
}
#endif

/*
 * defines for L4 types for test-environment
 * L4 types are l4_strdope_t and l4_fpage_t
 */
#define random_l4_strdope_t (L4_StringItem_t){ raw: { 0, 0 } }
#define random_l4_fpage_t L4_Nilpage

#endif // __DICE_L4_V2_H__
