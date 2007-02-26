/* -*- c++ -*- */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_IDS_H__
#define L4_IDS_H__

#include <l4/sys/types.h>

class L4_threadid
{
public:
  L4_threadid( l4_threadid_t const &id = L4_INVALID_ID )
  : id(id)
  {}

  bool is_valid() const
  { return l4_is_invalid_id(id); }

  operator l4_threadid_t & () { return id; }

  bool operator == (L4_threadid const &o)
  { return id.id.task == o.id.id.task && id.id.lthread == o.id.id.lthread; }

private:
  l4_threadid_t id;
};


class L4_uid
{
public:
  L4_uid( l4_threadid_t id ) : _id(id.id.task)
  {}

  L4_uid( unsigned id = 0) : _id(id)
  {}

  bool operator == ( L4_uid const &o ) const
  { return _id == o._id; }

  bool operator != ( L4_uid const &o ) const
  { return _id != o._id; }

  bool is_valid() const
  { return _id != 0; }

  L4_uid &operator = ( L4_uid const &o )
  { _id = o._id; return *this; }

  operator unsigned ()
  { return _id; }

private:
  unsigned _id;

};



#endif // L4_IDS_H__
