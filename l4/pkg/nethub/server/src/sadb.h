/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_SADB_H__
#define L4_NH_SADB_H__

#include <l4/cxx/iostream.h>

#include "routing.h"
#include "ip.h"
#include "ip_sec.h"

class SADB;
class Ike_connector;

class Base_sa : public Routing_entry
{
public:
  /**
   * \brief Type and mode constants for SA type.
   */
  enum Type 
  {
    esp = 3,       ///< SA type ID for ESP.
    ah  = 2,       ///< SA type ID for AH.
    pas = 1,       ///< SA type ID for pass (no IPSec).
    tun = 4,       ///< SA tunnel mode bit.
    mode_mask = 4, ///< Mask for mode flag.
  };
  
  virtual bool is_void() const = 0;

  /// Print SA (key values) to stream.
  virtual void print( L4::BasicOStream &o ) const = 0;
  
  Base_sa(u32 satype, u32 dst) : _type(satype), _dst(dst) 
  {}
  
  /// Get the destination IP address.
  u32 dst() const 
  { return _dst; }
  
  /// Get the SA-type (esp, ah, or pas).
  u32 type() const 
  { return _type & ~mode_mask; }

  /// Get the SA-mode (tunnel if ret!=0).
  u32 mode() const 
  { return _type & mode_mask; }
  
  void mode(u32 mode) 
  { _type = (_type & ~mode_mask) | (mode & mode_mask); }

  u32 satype() const 
  { return _type; }

private:
  u32 _type;
  u32 _dst;
};

/// Security Association
class SA : public Base_sa
{
public:
  enum Expire
  {
    no_lt   = 0,
    soft_lt = 1,
    hard_lt = 2,
  };
  
  enum State 
  {
    larval = 0,
    mature = 1,
    dying  = 2,
    dead   = 3
  };
  
  struct Lifetime
  {
    Lifetime(u64 bytes, u64 addtime, u64 usetime)
      : bytes(bytes), addtime(addtime), usetime(usetime)
    {}
    
    u64 bytes;
    u64 addtime;
    u64 usetime;

    bool is_valid() const { return bytes || addtime || usetime; }
  };
 

  /**
   * \brief Direction flags.
   */
  enum Direction 
  {
    dir_none = 2, ///< No direction assigned yet.
    dir_in = 0,   ///< SA for inbound packets.
    dir_out = 1,  ///< SA for outbound packets.
  };
  
  /**
   * \brief Create SA.
   * \param tx_if transmit interface for packets handled by this SA.
   * \param type  type and mode (see SA::Type).
   */
  SA(Tx_iface *tx_if, u32 type, u32 spi, u32 dst, 
     Auth_algo *a, Crypt_algo *c, Ip_packet *i, 
     Lifetime const &s, Lifetime const &h);
      
  SA(u32 type, u32 spi, u32 dst)
    : Base_sa(type, dst), _iface(0), _spi(spi),
      _state(larval), _current(0,0,0), _soft(0,0,0), _hard(0,0,0),
      seq(0), c(0), a(0), i(0), direction(dir_none), _chain(0), _c_chain(0)
  {}


  /**
   * \brief Destroy SA.
   */
  ~SA();

  /// Get the security policy index.
  u32 spi() const { return _spi; }
  
  void spi(u32 spi) { _spi = spi; }
  
  /// Get next SA of bundle.
  SA *chain() const { return _chain; }

  /// Set next SA in bundle.
  void chain( SA *c ) { _chain = c; }

  /// Compare with given key values.
  bool equals( u32 _type, u32 spi, u32 _dst ) const
  { return type() == _type && _spi == spi && dst() == _dst; }
  
  /// Compare key values from other SA.
  bool operator == (SA const &o) const 
  { return satype() == o.satype() && _spi == o._spi && dst() == o.dst(); }

  /// Print SA (key values) to stream.
  void print( L4::BasicOStream &o ) const;

  /// Print full SA information (algorithms and key values).
  void print_full( L4::BasicOStream &o ) const;
  
  /// Get pointer to collision chain pointer (for hashtable).
  SA **c_chain() const 
  { return &_c_chain; }

  
  Lifetime const &current() const
  { return _current; }
  
  Lifetime const &soft() const 
  { return _soft; }
  
  Lifetime const &hard() const 
  { return _hard; }

  void soft(Lifetime const &s)
  { _soft = s; }
  
  void hard(Lifetime const &h)
  { _hard = h; }

  inline Expire send_bytes(unsigned n); 

  void set_ike(Ike_connector *i) 
  { _ike_c = i; }

  Ike_connector *ike() const 
  { return _ike_c; }
  
  void expired(Expire e) const;

  Tx_iface *iface() const 
  { return _iface; }

  void iface(Tx_iface *ifa) 
  { _iface = ifa; }
  
  /// Get destination interface.
  Tx_iface *destination() const;

  bool is_void() const 
  { return false; };

  unsigned state() const 
  { return _state; }

  void state(unsigned st) 
  { _state = st; }

  
private:
  Tx_iface *_iface;
  u32 _spi;

  unsigned _state;
  Lifetime _current;
  Lifetime _soft;
  Lifetime _hard;

public:
  mutable u32 seq;
  Crypt_algo *c;
  Auth_algo *a;
  Ip_packet *i;
  unsigned direction;

private:
  mutable SA *_chain;
  mutable SA *_c_chain;
  Ike_connector *_ike_c;
};

class Void_sa : public Base_sa
{
public:
  /// Get destination interface.
  Tx_iface *destination() const;
  
  bool is_void() const 
  { return true; };

  Void_sa(u32 satype, u32 dst) : Base_sa(satype, dst)
  {}

  Void_sa(Base_sa const *sa) : Base_sa(*sa)
  {}

  /// Print SA (key values) to stream.
  void print( L4::BasicOStream &o ) const;
};


/// Security Policy
class Eroute
{
public:
  
  Eroute(u32 src_if = (u32)-1, u32 d_mask=0, u32 d_addr=0, 
         u32 s_mask=0, u32 s_addr=0, 
	 u64 sel_mask=0, u64 selector=0,
	 u8 protocol=0,
	 Base_sa *sa = 0)
    : src_if(src_if), d_mask(d_mask), d_addr(d_addr), 
      s_mask(s_mask), s_addr(s_addr), 
      sel_mask(sel_mask), selector(selector),
      protocol(protocol), _sa(sa), c_chain(0)
  {}

  ~Eroute()
  {
    if (re() && re()->is_void())
      delete re();
  }

  bool operator == (Eroute const &o) const
  {
    return src_if == o.src_if && 
           d_addr == o.d_addr && d_mask == o.d_mask && 
	   s_addr == o.s_addr && s_mask == o.s_mask &&
	   sel_mask == o.sel_mask && selector == o.selector &&
	   protocol == o.protocol;
  }

  bool equals(u32 _src_if, u32 dmask, u32 daddr, u32 smask, u32 saddr, 
              u64 _sel_mask, u64 _selector, u8 _protocol )
  {
    return src_if == _src_if && d_addr == daddr && d_mask == dmask && 
           s_addr == saddr && s_mask == smask &&
	   sel_mask == _sel_mask && selector == _selector &&
	   protocol == _protocol;
  }

  bool matches(u32 sif, u32 daddr, u32 saddr, 
               u64 _selector, u8 _protocol ) const
  { 
    return (src_if==(u32)-1 || src_if == sif) &&
           !((d_addr ^ daddr) & d_mask) && 
           !((s_addr ^ saddr) & s_mask) && 
	   !((selector ^ _selector) & sel_mask) &&
	   (protocol==0 || protocol == _protocol);
  }

  void print(L4::BasicOStream &s) const;

  Eroute **chain() 
  { return &c_chain; }

  void re(Base_sa *sa)
  { _sa = sa; }
  
  Base_sa *re() const
  { return _sa; }
  
  SA *sa() const
  { return static_cast<SA*>(_sa); }

public:
  u32 src_if;
  u32 d_mask;
  u32 d_addr;
  u32 s_mask;
  u32 s_addr;
  u64 sel_mask;
  u64 selector;
  u8  protocol;
  
private:
  Base_sa *_sa;
  Eroute *c_chain;

};

template< typename Hash >
class Basic_SADB
{
public:
  enum 
  {
    EOK       = 0,
    EEXIST    = 1,
    EINVAL    = 2,
    ENOTFND   = 3,
  };
  
  Basic_SADB(Ike_connector *i) 
    : ike_connector(i)
  {}

  SA *get_spi(u32 type, u32 dst, u32 *min_spi, u32 max_spi);
  
  unsigned add(SA *sa);
  unsigned update(Tx_iface *tx_if, u32 type, u32 spi, 
                  u32 dst, Auth_algo *a, Crypt_algo *c,
		  Ip_packet *i, SA::Lifetime const &s,
		  SA::Lifetime const &h);

  SA *get(u32 type, u32 spi, u32 dst);
  SA *find_first(u32 type, u32 dst);
  
  unsigned del(u32 type, u32 spi, u32 dst);

  unsigned del(SA *sa) 
  {
    if (!sa) return EINVAL;
    return del(sa->type(), sa->spi(), sa->dst());
  }

  Ike_connector *ike() const
  { return ike_connector; }

  void flush();

  void print(L4::BasicOStream &o) const;
  
private:
  SA *buckets[Hash::num_buckets];
  Ike_connector *ike_connector;
  static SA **c_chain_find( SA **c, u32 type, u32 spi, u32 dst );
};

class Simple_hash
{
public:
  enum { num_buckets = 256, };
  static unsigned index(u32 type, u32 a, u32 b) 
  { return (b | a) % num_buckets; }

};

class SADB : public Basic_SADB<Simple_hash>
{
public:
  SADB(Ike_connector *i)
  : Basic_SADB<Simple_hash>(i)
    {}
};

class Eroute_table : public Routing_table
{
public:
  enum 
  {
    EOK     = 0,
    EEXIST  = 1,
    EINVAL  = 2,
    ENOTFND = 3,
  };
  Eroute_table(SADB *sadb, Ike_connector *i) 
    : _if_mode(0), sadb(sadb), first(0), ike_connector(i) 
  {}

  void if_mode(unsigned iface, bool mode) 
  {
    if (mode)
      _if_mode |= (1<<iface);
    else
      _if_mode &= ~(1<<iface);
  }
  
  unsigned add(Eroute *r);

  unsigned void_all();
  unsigned void_all(u32 type, u32 spi, u32 dst);
  unsigned update_sa(SA *new_sa);

  unsigned del(u32 src_if, u32 dmask, u32 daddr, u32 smask, u32 saddr,
               u64 sel_mask, u64 selector, u8 proto);
  unsigned del(Eroute *r)
  {
    if (!r) return EINVAL;
    return del(r->src_if, r->d_mask, r->d_addr, r->s_mask, r->s_addr, 
	       r->sel_mask, r->selector, r->protocol);
  }
  
  Eroute *get(unsigned src_if, Ip_packet *p, u64 selector) const 
  { 
    if (p)
      return get(src_if, p->daddr(), p->saddr(), selector, p->protocol());
    else
      return 0;
  }

  Routing_entry *route(unsigned iface, Ip_packet *p) const;
  
  Eroute *get(u32 src_if, u32 daddr, u32 saddr, u64 selector, u8 proto) const;
  Eroute *get(u32 src_if, u32 dmask, u32 daddr, u32 smask, u32 saddr,
              u64 _sel_mask, u64 _selector, u8 _proto) const;

  void flush();

  void print(L4::BasicOStream &o) const;

private:
  u32 _if_mode;
  SADB *sadb;
  Eroute *first;
  Ike_connector *ike_connector;
};

//-------------IMPL-----------------------------------------------------------

#include "time.h"
inline
SA::SA(Tx_iface *tx_if, u32 type, u32 spi, u32 dst, 
       Auth_algo *a, Crypt_algo *c, Ip_packet *i, 
       Lifetime const &s, Lifetime const &h)

: Base_sa(type, dst), _iface(tx_if), _spi(spi),
  _state(mature), _current(0,Time::get_time(),0), _soft(s), _hard(h),
  seq(0), c(c), a(a), i(i), direction(dir_none), _chain(0), _c_chain(0)
{}

inline SA::Expire SA::send_bytes(unsigned n) 
{ 
  u64 t = Time::get_time();
  if (_current.bytes == 0)
    _current.usetime = t;
    
#if 0
  L4::cout << Time::get_time() << ": send " << n << " bytes\n";
  L4::cout << L4::dec << "send " << n << " bytes via ";
  print(L4::cout);
  L4::cout << '\n';

  L4::cout << L4::dec << "soft " << (int)_soft.bytes << "\n";
#endif
  
  _current.bytes += n;

  if (  ((_hard.bytes + 1) && _current.bytes >= _hard.bytes)
      ||(_hard.addtime && _hard.addtime + _current.addtime < t)
      ||(_hard.usetime && _hard.usetime + _current.usetime < t))
    {
      _state = dead;
      return hard_lt;
    }
  
  if (  ((_soft.bytes + 1) && _current.bytes >= _soft.bytes )
      ||(_soft.addtime && _soft.addtime + _current.addtime < t)
      ||(_soft.usetime && _soft.usetime + _current.usetime < t))
    {
      _state = dying;
      return soft_lt;
    }
  
  return no_lt;
}

  
template< typename Hash >
SA **Basic_SADB<Hash>::c_chain_find(SA **c, u32 type, u32 spi, u32 dst)
{
  while (   *c && (((*c)->type() != (type & ~SA::mode_mask))
	 || (*c)->spi() != spi 
	 || (*c)->dst() != dst))
    c = (*c)->c_chain();
  
  return c;
}

template< typename Hash >
SA *Basic_SADB<Hash>::get(u32 type, u32 spi, u32 dst)
{
  unsigned index = Hash::index(type, spi, dst);
  SA **sa = &buckets[index];
  if (!sa)
    return 0;

  return *c_chain_find(sa, type, spi, dst);
}

template< typename Hash >
SA *Basic_SADB<Hash>::find_first(u32 type, u32 dst)
{
  for (unsigned i = 0; i<Hash::num_buckets; ++i)
    {
      SA *b = buckets[i];
      while (b)
	{
	  if (b->satype() == type && dst == b->dst())
	    return b;
        
	  b = *b->c_chain();
	}
    }

  return 0;
}



template< typename Hash >
SA *Basic_SADB<Hash>::get_spi(u32 type, u32 dst, u32 *min_spi, 
                                   u32 max_spi)
{
  SA *sa = new SA(type, 0, dst);
  
  while(1)
    {
      sa->spi(*min_spi);
      if (add(sa) == EEXIST)
	{
	  if (*min_spi >= max_spi)
	    {
	      delete sa;
	      return 0;
	    }
	  else
	    (*min_spi)++;
	}
      else
	return sa;
    }
}

template< typename Hash >
unsigned Basic_SADB<Hash>::update(Tx_iface *tx_if, u32 type, u32 spi, 
                                  u32 dst, Auth_algo *a, Crypt_algo *cr,
				  Ip_packet *i, SA::Lifetime const &s,
				  SA::Lifetime const &h)
{
  unsigned mode = type & SA::mode_mask;
  
  unsigned index = Hash::index(type & ~SA::mode_mask, spi, dst);
  SA **b = &buckets[index];
  SA **c = c_chain_find(b, type & ~SA::mode_mask, spi, dst);
  if (!*c)
    return ENOTFND;

  u32 state = (*c)->state();

  if (state == SA::mature || state == SA::dead)
    return EINVAL;

  if ((a || cr || i) && state != SA::larval)
    return EINVAL;

  if (state = SA::larval && !(a && cr && (i || mode != SA::tun) && tx_if))
    return EINVAL;

  (*c)->state(SA::mature);
  if (a) (*c)->a = a;
  if (cr) (*c)->c = cr;
  if (i) (*c)->i = i;
  if (s.is_valid()) (*c)->soft(s);
  if (h.is_valid()) (*c)->hard(h);
  if (tx_if) (*c)->iface(tx_if);
  
  return EOK;
}

template< typename Hash >
unsigned Basic_SADB<Hash>::add(SA *sa)
{
  if (!sa)
    return EINVAL;
  
  u32 spi = sa->spi();
  u32 dst = sa->dst();
  u32 type = sa->type();
  unsigned index = Hash::index(type, spi, dst);
  SA **b = &buckets[index];
  SA **c = c_chain_find(b, type, spi, dst);
  if (*c)
    return EEXIST;

  sa->set_ike(ike_connector);
  *sa->c_chain() = buckets[index];
  buckets[index] = sa;
  
  return 0;
}

template< typename Hash >
unsigned Basic_SADB<Hash>::del(u32 type, u32 spi, u32 dst)
{
  type &= ~SA::mode_mask;
  unsigned index = Hash::index(type, spi, dst);
  SA **b = &buckets[index];
  SA **c = c_chain_find(b, type, spi, dst);
  if (!*c)
    return ENOTFND;

  SA *next = *(*c)->c_chain();
  delete *c;
  
  *c = next;
  return 0;
}

template< typename Hash >
void Basic_SADB<Hash>::flush()
{
  for (unsigned i = 0; i<Hash::num_buckets; ++i)
    {
      SA *b = buckets[i];
      buckets[i] = 0;
      while (b)
	{
          SA *c = *b->c_chain();
	  delete b;
	  b = c;
	}
    }
}

template< typename Hash >
void Basic_SADB<Hash>::print(L4::BasicOStream &o) const
{
  for (unsigned i = 0; i<Hash::num_buckets; ++i)
    {
      SA *sa = buckets[i];
      while (sa)
	{
	  sa->print_full(o); o << '\n';
	  sa = *sa->c_chain();
	}
    }
}

#endif // L4_NH_SADB_H__

