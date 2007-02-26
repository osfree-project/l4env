/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_HUB_REGION_H__
#define L4_HUB_REGION_H__

#include <l4/sys/types.h>

class Region_handler
{
public:
  virtual void unresolved_fault( void *priv ) = 0;
};

class Region 
{
public:
  Region() 
    // gcc 2.95 sucks (_mapper(L4_INVALID_ID))
    : _start(0), _end(0), _offset(0), /*_mapper(L4_INVALID_ID),*/
      _handler(0), last_request((unsigned long)-1)
  {_mapper = L4_INVALID_ID;}

  Region( unsigned long start, unsigned long end, 
          unsigned long offset, l4_threadid_t mapper, 
	  Region_handler *h, void *priv )
    : _start(start), _end(end), _offset(offset), _mapper(mapper), 
      _handler(h), _priv(priv), last_request((unsigned long)-1)
  {}

  void invalidate()
  { mapper( L4_INVALID_ID); }

  bool in_region( unsigned long address ) const
  { return address >=_start && address<_end; }

  bool is_empty() const
  { return _start == _end; }

  bool is_vlaid() const
  { return !l4_is_invalid_id(_mapper); }

  bool operator == (Region const &o) const
  { return _start == o._start && _end == o._end; }

  int request_mapping( unsigned long address ) const;

  unsigned long start() const { return _start; }
  unsigned long end() const { return _end; }
  unsigned long offset() const { return _offset; }
  void offset( unsigned long o ) { _offset = o; }
  void mapper( l4_threadid_t m ) { _mapper = m; }

  unsigned long localize( unsigned long address ) const
  { 
    // XXX: only object up to 64k are allowed and the start offset must
    //      be at least 64k before the _end.
    if (is_empty())
      return 0;
    if (address >= _offset && (address - _offset) < (_end - 0xffff))
      return address - _offset + _start;
    return 0;
  }

  template< typename Cl >
  Cl *localize( Cl *o ) const
  { return (Cl*)localize((unsigned long)o); }

  Region_handler *handler() const { return _handler; }

  void unresolved_fault() const
  {
    handler()->unresolved_fault(_priv);
  }
  
private:
  unsigned long _start, _end;
  unsigned long _offset;
  l4_threadid_t _mapper;
  Region_handler *_handler;
  void *_priv;
  mutable unsigned long last_request;
};

namespace L4 
{
  class BasicOStream;
};

L4::BasicOStream &operator << (L4::BasicOStream &o, Region const &r);

#endif // L4_HUB_REGION_H__

