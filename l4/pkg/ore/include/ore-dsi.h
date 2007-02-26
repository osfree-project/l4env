/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __ORE_DSI_H
#define __ORE_DSI_H

#ifdef ORE_DSI
#include <l4/dsi/dsi.h>
#else
typedef struct dsi_socket_ref
{
  /* socket descriptor */
  l4_int32_t socket;   //!< reference to socket

  /* thread ids */
  l4_threadid_t  work_th;  //!< work thread
  l4_threadid_t  sync_th;  //!< synchronisation thread
  l4_threadid_t  event_th; ///< event signalling thread
} dsi_socket_ref_t;

struct dsi_socket {};
struct dsi_packet {};

typedef struct dsi_socket dsi_socket_t;
typedef struct dsi_packet dsi_packet_t;
#endif
#endif
