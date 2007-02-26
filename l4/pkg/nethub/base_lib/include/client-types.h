/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_CLIENT_TYPES_H__
#define L4_NH_CLIENT_TYPES_H__

#include <l4/nethub/iface.h>

/**
 * \brief Return values of library functions.
 */
enum Nh_return_values {
  L4_NH_OK       = 0, ///< Well done, OK.
  L4_NH_EAGAIN   = 1, ///< Try a again please.
  L4_NH_ERR      = 2, ///< Error during the operation.
  L4_NH_EREALLOC = 3, ///< Please allocate a larger receive buffer.
};

struct Nh_iface 
{
  l4_threadid_t out;
  l4_threadid_t in;
  l4_threadid_t mapper;
  l4_threadid_t rcv_thread;
  l4_threadid_t txe_irq;
  void *shared_mem_start;
  void *shared_mem_end;
  struct Nh_packet_ring *out_ring;
};

#endif // L4_NH_CLIENT_H__
