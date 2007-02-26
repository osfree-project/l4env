/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_CFG_H__
#define L4_NH_CFG_H__

#include <l4/sys/types.h>
#include <l4/nethub/base.h>
#include <l4/nethub/cfg-types.h>
#include <l4/nethub/nh-cfg-client.h>

#ifdef __cplusplus
extern "C" {
#endif

int nh_sadb_map_notify_buffer(Hub hub, struct Nh_sadb_notify_head *buffer);

extern inline
int
nh_sadb_del_sa(Hub hub, const struct Nh_sadb_sa_key *key);

extern inline
int
nh_sadb_add_sa(Hub hub, const struct Nh_sadb_sa_msg *msg, unsigned size);

extern inline
int
nh_sadb_update_sa(Hub hub, const struct Nh_sadb_sa_msg *msg, unsigned size);

extern inline
int
nh_sadb_del_sp(Hub hub, unsigned src_if,
               const struct Nh_sp_address *src,
	       const struct Nh_sp_address *dst);

extern inline
int
nh_sadb_add_sp(Hub hub, unsigned src_if,
               const struct Nh_sp_address *src,
	       const struct Nh_sp_address *dst,
	       const struct Nh_sadb_sa_key *sa);

extern inline
int
nh_sadb_get_spi(Hub hub, l4_uint32_t satype, l4_uint32_t dst_ip,
		l4_uint32_t *min_spi, l4_uint32_t max_spi);

extern inline
int
nh_sadb_flush_sa(Hub hub);

extern inline
int
nh_sadb_dump_sa(Hub hub);

extern inline
int
nh_sadb_flush_sp(Hub hub);

extern inline
int
nh_sadb_dump_sp(Hub hub);

extern inline
int
nh_sadb_chg_iface(Hub hub, unsigned niface, unsigned direction);

struct Nh_sadb_notify_head *
nh_sadb_next_notification(struct Nh_sadb_notify_head *last);

void
nh_sadb_clear_notification(struct Nh_sadb_notify_head *nfy);


//-----------------------IMPL-------------------------------------------------

extern inline
int
nh_sadb_get_spi(Hub hub, l4_uint32_t satype, l4_uint32_t dst_ip,
		l4_uint32_t *min_spi, l4_uint32_t max_spi)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_get_spi_call(&hub, satype, dst_ip, min_spi, max_spi, &env);

}

extern inline
int
nh_sadb_del_sa(Hub hub, const struct Nh_sadb_sa_key *key)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_del_sa_call(&hub, key, &env);
}

extern inline
int
nh_sadb_add_sa(Hub hub, const struct Nh_sadb_sa_msg *msg, unsigned size)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_add_sa_call(&hub, (char*)msg, size, &env);
}

extern inline
int
nh_sadb_update_sa(Hub hub, const struct Nh_sadb_sa_msg *msg, unsigned size)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_update_sa_call(&hub, (char*)msg, size, &env);
}

extern inline
int
nh_sadb_del_sp(Hub hub, unsigned src_if,
               const struct Nh_sp_address *src,
	       const struct Nh_sp_address *dst)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_del_sp_call(&hub, src_if, src, dst, &env);
}

extern inline
int
nh_sadb_add_sp(Hub hub, unsigned src_if,
               const struct Nh_sp_address *src,
	       const struct Nh_sp_address *dst,
	       const struct Nh_sadb_sa_key *sa)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_add_sp_call(&hub, src_if, src, dst, sa, &env);
}

extern inline
int
nh_sadb_flush_sa(Hub hub)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_flush_sa_call(&hub, &env);
}

extern inline
int
nh_sadb_dump_sa(Hub hub)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_dump_sa_call(&hub, &env);
}

extern inline
int
nh_sadb_flush_sp(Hub hub)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_flush_sp_call(&hub, &env);
}

extern inline
int
nh_sadb_dump_sp(Hub hub)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_dump_sp_call(&hub, &env);
}

extern inline
int
nh_sadb_chg_iface(Hub hub, unsigned niface, unsigned direction)
{
  DICE_DECLARE_ENV(env);
  return Nh_cfg_chg_iface_call(&hub, niface, direction, &env);
}

#ifdef __cplusplus
}
#endif

#endif
