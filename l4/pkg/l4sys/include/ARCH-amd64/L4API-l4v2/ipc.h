/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-amd64/L4API-l4v2/ipc.h
 * \brief   L4 IPC System Calls
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_IPC_H__
#define __L4_IPC_H__

#include <l4/sys/types.h>

#define L4_IPC_IOMAPMSG_BASE 0xfffffffff0000000 ///< I/O mapmsg base

#include_next <l4/sys/ipc.h>

#include <l4/sys/ipc-invoke.h>

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)  ///< GCC in a single figure (e.g. 402 for gcc-4.2)

#ifdef PROFILE
#  include "ipc-l42-profile.h"
#else
#  if GCC_VERSION < 303
#    error gcc >= 3.3 required
#  else
#    include "ipc-l42-gcc3.h"
#  endif
#endif

#endif /* !__L4_IPC_H__ */
