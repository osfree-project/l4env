/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_CLIENT_H__
#define L4_NH_CLIENT_H__

#include <l4/nethub/iface.h>
#include <l4/nethub/base.h>
#include <l4/nethub/client-types.h>
#include <l4/nethub/nh-client-client.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Connect to a interface in the Viaduct.
 * \param hub        L4 thread ID of the Viaduct's management thread (use
 *                   names).
 * \param niface     the interface number that should be connected.
 * \param iface      the interface descriptor.
 */
extern inline
int
nh_if_open(Hub hub, unsigned niface, struct Nh_iface *iface);

extern inline
int
nh_if_close(Hub hub, unsigned niface);

/** Get the statistics of the specified interface.
 * \param hub    L4 thread ID of the Viaduct's management thread.
 * \param iface  interface number.
 * \param tx     if != NULL then set to the number of packets transmitted via
 *               the interface.
 * \param txd    if != NULL then set to the number of packets received on
 *               the interface, but dropped after routing.
 * \param rx     if != NULL then set to the number of packets that are
 *               received on the interface.
 * \param rxd    if != NULL then set to the number of packets that are droppped
 *               due to a missing route.
 */
extern inline
int
nh_if_stats(Hub hub, unsigned niface,
            unsigned *tx, unsigned *txd,
	    unsigned *rx, unsigned *rxd);

/** Mapper loop.
 * \param start  local start address of the shared memory.
 * \param end    local end address of the shared memory.
 * \return Sould never return!
 *
 * This function must be started in an extra L4 thread and handles the
 * mapping of memory to the Viaduct.
 */
int nh_region_mapper( void const *start, void const *end );

/** Receive a packet from Viaduct to the given buffer.
 * \param iface   Viaduct's interface descriptor opened with nh_open_iface.
 * \param buffer  local pointer to the receive buffer.
 * \param len     the length of the buffer, or on NH_EREALLOC the size needed
 *                for the packet.
 *
 * This function blocks until a pakcet is received or a bigger buffer
 * must be allocated.
 */
int nh_recv( struct Nh_iface *iface, void *buffer, unsigned long *len );

/** Queues a packet into the send ring and notifies the Viaduct.
 * \param iface   the Viaduct's interface descriptor opened with
 *                nh_open_iface.
 * \param buffer  pointer to the packet that is to be transmitted.
 * \param priv    pointer to private data associated with the packet
 *                (not interpreted).
 */
int nh_send( struct Nh_iface *iface, void *buffer, void *priv );

/**
 * \brief Function type taken by nh_for_each_empty_slot.
 */
typedef void (Nh_slot_func)(void **);

/** Calls a function for each empty slot in a packet ring.
 * \param r  the packet ring.
 * \param f  the function to be called.
 *
 * Empty are slots that have flags set to 0.
 */
void nh_for_each_empty_slot( struct Nh_packet_ring *r, Nh_slot_func *f );

//-----------------------------------IMPL-------------------------------------

extern inline
int
nh_if_open(Hub hub, unsigned niface, struct Nh_iface *iface)
{
  DICE_DECLARE_ENV(env);
  return Nh_client_open_call(&hub, niface, iface, &env);
}

extern inline
int
nh_if_close(Hub hub, unsigned niface)
{
  DICE_DECLARE_ENV(env);
  return Nh_client_close_call(&hub, niface, &env);
}

extern inline
int
nh_if_stats(Hub hub, unsigned niface,
            unsigned *tx, unsigned *txd,
	    unsigned *rx, unsigned *rxd)
{
  DICE_DECLARE_ENV(env);
  return Nh_client_stats_call(&hub, niface, tx, txd, rx, rxd, &env);
}

#ifdef __cplusplus
}
#endif

#endif // L4_NH_CLIENT_H__
