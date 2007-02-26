/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_IFACE_H__
#define L4_NH_IFACE_H__

#include <l4/sys/types.h>

/**
 * \brief Entry in a clients send packet ring.
 */
struct Nh_packet_ring_entry
{
  void *pkt;            ///< Pointer to the packet.
  l4_uint32_t flags;    ///< Packet flags (set to 1 for valid packet).
  l4_uint64_t selector; ///< Opaque selector value (used for SPD).
  void *priv;           ///< Private data untouched by the Viduct.
};

/**
 * \brief Send packet ring.
 *
 * A packet ring must be located inside the shared memory region.
 */
struct Nh_packet_ring 
{
  char empty;                           ///< Flags an empty ring
  unsigned next;                        ///< Next slot for client.
  unsigned next_to_send;                ///< Next slot read by the Viaduct.
  struct Nh_packet_ring_entry ring[32]; ///< Entries (slots).
};

#endif // L4_NH_IFACE_H__

