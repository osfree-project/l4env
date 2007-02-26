/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_SADB_CFG_H__
#define L4_NH_SADB_CFG_H__

class Nh_sadb_msg;
class SADB;
class Eroute_table;
class Iface_vector;

class SADB_env
{
public:
  SADB_env() : ifl(0), rtab(0), sadb(0) {}
  SADB_env( Iface_vector *ifl, Eroute_table *rtab, SADB *sadb)
    : ifl(ifl), rtab(rtab), sadb(sadb)
  {}

  Iface_vector *ifl;
  Eroute_table *rtab;
  SADB *sadb;
};

int sadb_handle_msg( SADB_env const &env, unsigned cmd, void *msg );

#endif // L4_NH_SADB_CFG_H__

