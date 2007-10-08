/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/ipc.h
 * \brief   L4 IPC System Calls, x86
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_IPC_H__
#define __L4_IPC_H__

#include <l4/sys/types.h>

#define L4_IPC_IOMAPMSG_BASE 0xf0000000   ///< I/O mapmsg base

#include_next <l4/sys/ipc.h>

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

#include <l4/sys/rt_sched-proto.h>
#include <l4/sys/ipc-invoke.h>

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)  ///< GCC in a single figure (e.g. 402 for gcc-4.2)

#ifdef PROFILE
#  include "ipc-l42-profile.h"
#else
#  if GCC_VERSION < 302
#    error gcc >= 3.0.2 required
#  else
#    ifdef __PIC__
#      include "ipc-l42-gcc3-pic.h"
#    else
#      include "ipc-l42-gcc3-nopic.h"
#    endif
#  endif
#endif

#endif /* !__L4_IPC_H__ */
