/**
 * \file interface.h
 * Definition of network interfaces of the Viaduct.
 */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef L4_NH_INTERFACE_H__
#define L4_NH_INTERFACE_H__

#include <l4/nethub/client-types.h>
#include "ip.h"
#include "region.h"
#include "config.h"

class Iface;
class Routing_entry;

/**
 * Pointer to entry in incoming ring buffer.
 */
class Rx_pkt
{
public:
  /**
   * Create a pointer to a packet descriptor in a packet ring.
   * 
   * The pointer also carries an offset to calculate the local (Viaduct's)
   * address of the described IP packet.
   * 
   * \param e local address of the packet ring entry.
   * \param ofs offset to calculate local from remote addresses.
   */
  Rx_pkt(Nh_packet_ring_entry *e = 0, signed long ofs = 0)
    : e(e), o(ofs)
  {}

  /**
   * Get Ip_packet that is described.
   * 
   * Offset calculation is done to have a valid address in the Viaduct's
   * address space.
   * 
   * \return The local address of the described IP packet.
   */
  Ip_packet *packet() const
  { return (Ip_packet*)((char*)e->pkt + o); }

  /**
   * Clear packet flags (mark as done).
   */
  void clear_flags() const
  { e->flags = 0; }

  /**
   * Get the validity of this pointer.
   *
   * \return 0 if the pointer is invalid, !0 else.
   */
  unsigned valid() const 
  { return (unsigned)e; }

  /**
   * Make this pointer invalid.
   */
  void invalidate() 
  { e=0; o=0; }
  
private:
  Nh_packet_ring_entry *e;
  signed long o;
};


/**
 * Transmission interface.
 *
 * A transmission interface represetns a peer thread which routed 
 * packets are transmitted to.
 */
class Tx_iface
{
public:
  /**
   * Create a new TX interface.
   */
  Tx_iface() 
  { _peer = L4_INVALID_ID; }

  /**
   * Set the peer thread for this interface.
   * 
   * \param peer the thread ID of the peer thread for this interface.
   *
   * The peer is the thread which handles the transmission of packets
   * sent through this interface. The peer thread must handle the 
   * communication with the client.
   */
  void peer(l4_threadid_t const &p) 
  { _peer = p; }

  /**
   * Get the peer thread of this interface.
   * 
   * \return The thread ID of the peer thread.
   * \see peer(l4_threadid_t const &p)
   */
  l4_threadid_t peer() const 
  { return _peer; }

  /**
   * Transmit packet via this interface.
   * 
   * \param i the incoming interface.
   * \param slot the slot in the incoming interface that contains 
   *        the packet
   */
  void xmit(Iface *i, unsigned slot);

private:
  l4_threadid_t _peer;
};

/**
 * Structure for a bidirectional interface.
 *
 * This class keeps a referece to the client's packet ring, for incmoing 
 * packets. It also manages the memory region for the shared memory used to 
 * send and receive packets.
 */
class Iface : public Tx_iface
{
#ifdef NH_CONFIG_IFACE_STATS
private:
  u32 rx_count;
  u32 tx_count;
  u32 rx_drop_count;
  u32 tx_drop_count;
public:
  void reset_stats() 
  { 
    tx_count = 0; rx_count = 0; 
    tx_drop_count = 0; rx_drop_count = 0; 
  }
  
  void inc_rx_count() 
  { ++rx_count; }
  
  void inc_tx_count() 
  { ++tx_count; }
  
  void inc_rx_drop_count() 
  { ++rx_drop_count; }
  
  void inc_tx_drop_count() 
  { ++tx_drop_count; }
  
  void get_stats(unsigned &_rx_count, unsigned &_rx_drop_count,
                 unsigned &_tx_count, unsigned &_tx_drop_count) const
  {
    _tx_count = tx_count; _tx_drop_count = tx_drop_count;
    _rx_count = rx_count; _rx_drop_count = rx_drop_count;
  }
  
#else
public:
  void reset_stats() const {} 
  void inc_rx_count() const {}
  void inc_tx_count() const {}
  void inc_rx_drop_count() const {}
  void inc_tx_drop_count() const {}
  void get_stats(unsigned &, unsigned &, unsigned &, unsigned &) const
  {}
#endif

public:

  /**
   * Create a pointer to an incoming interface pcket ring.
   * 
   * This pointer also carries the offset for calculating local
   * (Viadtuct) from remote (client) addresses.
   *
   * \param r remote address of the packet ring structure.
   * \param ofs offset to calculate local addresses.
   */
  Iface()
    : reg(), r(0)
  { txe_irq = L4_INVALID_ID; /* I hate GCC 2.95*/ }
  
  /**
   * Set the pointer to the given packet ring.
   *
   * \param ri remote address of the packet ring structure.
   * \param ofs offset for calculating local addresses.
   */
  unsigned set_iface(Nh_packet_ring *ri, unsigned long start,
                     unsigned long end, unsigned long offset,
		     l4_threadid_t mapper, Region_handler *rh,
		     l4_threadid_t _txe_irq)
  {
    txe_irq = _txe_irq;
    reg = Region(start, end, offset, mapper, rh, this);
    r = reg.localize(ri);
    if (!r)
      {
	reg = Region();
        return 0;
      }
    reset_stats();
    return 1;
  }

  /**
   * Is the interface valid.
   */
  unsigned valid() const 
  { return (unsigned)r; }

  /**
   * Inavlidate this interface.
   */
  void invalidate() 
  { txe_irq = L4_INVALID_ID; r = 0; reg = Region(); }

  /**
   * Get next descriptor.
   * 
   * Get pointer to descriptor for the next packet to handle and 
   * mark it with the work-in-progress flag.
   */
  Rx_pkt next_to_handle(unsigned &slot)
  {
    unsigned i = r->next_to_send;
    // TODO must do compare-and-swap for multi-threaded router
    while (r->ring[i].flags == 3)
      i = (i+1) % 32;
    
    slot = i;

    if (r->ring[i].flags == 1) 
      {
        inc_rx_count();
        r->ring[i].flags = 3;
	r->next_to_send = (i+1) % 32;
	if (reg.localize(r->ring[i].pkt))
          return Rx_pkt(&r->ring[i], reg.start()-reg.offset());
	else
	  reg.unresolved_fault();
      }
    return Rx_pkt();
  }

  /**
   * Get the packet from the givel slot.
   * 
   * \param slot the slot number which should be returned.
   * \return A packet pointer to the packet in the given slot.
   */
  Rx_pkt packet(unsigned slot)
  {
    if (reg.localize( r->ring[slot].pkt))
      return Rx_pkt( &r->ring[slot], reg.start()-reg.offset() );
    else
      reg.unresolved_fault();

    return Rx_pkt();
  }

  /**
   * Set the route for the packet in the given slot.
   * 
   * \param slot the slot for the routed packet.
   * \param re the routing-table entry that was used to route the packet.
   */
  void set_route(unsigned slot, Routing_entry *re)
  { _routes[slot] = re; }

  /**
   * Get the route of the packet in the given slot.
   * 
   * \param slot the slot number.
   * \return The routing-table entry for the given slot (formerly 
   *         set by set_route).
   */
  Routing_entry *get_route(unsigned slot)
  { return _routes[slot]; }

  /**
   * Get state of this interface. 
   * 
   * \return 0 if there is no packet to handle, !0 else.
   */
  unsigned active() 
  { return r->ring[r->next_to_send].flags == 1; }

  /**
   * Set the empty flag for this interface.
   */
  void set_empty()
  { r->empty = 1; }

  /**
   * Clear the empty flag for this interface.
   */
  void clear_empty() 
  { r->empty = 0; }

  /**
   * Get the memory region for the client of this interface.
   * 
   * \return Memory region for the client of this interface.
   */
  Region *region() 
  { return &reg; } 

  void tx_empty();

private:
  l4_threadid_t txe_irq;
  Region reg;
  Nh_packet_ring *r;
  Routing_entry *_routes[32]; 
};

template< typename Cl >
class Vector
{
public:
  Vector(unsigned size);
  ~Vector();

  unsigned size() const 
  { return _size; }
 
  Cl &operator [] (unsigned index)
  { return v[index]; }
  
  Cl const &operator [] (unsigned index) const 
  { return v[index]; }

private:
  unsigned _size;
  Cl *v;
};
template< typename Cl >
Vector<Cl>::Vector( unsigned s )
: _size(s), v(new Cl[s])
{}

template< typename Cl >
Vector<Cl>::~Vector()
{ if (v) delete [] v; }

/**
 * List of input interfaces.
 */
class Iface_vector : public Vector<Iface>
{
public:

  /**
   * Create a new interface list.
   */
  Iface_vector( unsigned nifs ) 
    : Vector<Iface>(nifs), current_if(0)
  {}

  /**
   * Get a pointer to the next active interface.
   * 
   * \param num is set to the number of this interface in the list.
   * \return A pointer to the next active interface, or 0 if there is none.
   *
   * The next active interface is the next interface with a pending packet.
   */
  Iface *next_active(unsigned &num) 
  { 
    unsigned i=current_if;
    do
      {
        if (get(i)->valid() && get(i)->active())
	  {
	    current_if = (i+1) % size();
	    num = i;
	    return get(i);
	  }
	i = (i+1) % size();
      }
    while (i!=current_if); 

    return 0;
  }

  /**
   * The interface with the given index.
   * 
   * \param index the index of the requested interface.
   */
  Iface *get(unsigned index) 
  { return &operator [] (index); }

  /**
   * Set the parameters of the given interface.
   * 
   * \param index the index of the interface.
   * \param r the remote pointer to the receiving packet ring.
   * \param ofs the offset of the remote region.
   * \param mapper the remote mapper thread that sends mappings from the 
   *        remote address space.
   * \param rh the handler for unresolved faults in the interface region.
   */
  unsigned set(unsigned index, Nh_packet_ring *r, unsigned long ofs,
               l4_threadid_t mapper, Region_handler *rh, l4_threadid_t txe_irq) 
  {
    unsigned long base = index * 0x8000000;
    return get(index)->set_iface(r,0x10000000+base,0x18000000+base,
	ofs,mapper,rh,txe_irq);
  }

  /**
   * Get the total number of interfaces in this list.
   * 
   * \return The total number of interfaces in this list.
   */
  unsigned const num_ifs() const 
  { return size(); }
  
private:
  unsigned current_if;
};

#endif // L4_NH_INTERFACE_H__

