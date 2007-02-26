/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_CFG_TYPES_H__
#define L4_NH_CFG_TYPES_H__

#include <l4/sys/types.h>

/**
 * \brief SA key structure.
 *
 * The key uniquely identifies an SA.
 */
struct Nh_sadb_sa_key
{
  l4_uint32_t satype; ///< Type and mode (esp, ah, pas and tunnel or transport)
  l4_uint32_t spi;    ///< SPI
  l4_uint32_t dst_ip; ///< Destination IP
};

/**
 * \brief Data for a new SA.
 */
struct Nh_sadb_sa_data
{
  l4_uint8_t replay;            ///< Size of the replay window.
  l4_uint8_t state;             ///< State of the SA (see Nh_sadb_sa_state)
  l4_uint8_t auth;              ///< Auth alogorithm.
  l4_uint8_t encrypt;           ///< Encryption algorithm.
  l4_uint32_t dst_if;           ///< Destination interface for this SA.
  l4_uint32_t src_ip;           ///< Source IP for tunnel mode.
  l4_uint16_t auth_key_bits;    ///< Length of the auth key.
  l4_uint16_t encrypt_key_bits; ///< Length of the encrypt key.
};

/**
 * \brief Lifetime of a SA.
 */
struct Nh_sadb_sa_lifetime
{
  l4_uint64_t bytes;       ///< Bytes transmitted by this SA.
  l4_uint64_t addtime;     ///< Time since instanciation.
  l4_uint64_t usetime;     ///< Time since first use.
  l4_uint32_t allocations; ///< Number of allocations (unused).
};

/**
 * \brief Message for setting a SA.
 */
struct Nh_sadb_sa_msg
{
  struct Nh_sadb_sa_key key;       ///< SA key structure.
  struct Nh_sadb_sa_lifetime soft; ///< Soft lifetime.
  struct Nh_sadb_sa_lifetime hard; ///< Hard lifetime.
  struct Nh_sadb_sa_data data;     ///< SA data structure.
  /* followed by the keys */
};

struct Nh_sp_address
{
  l4_uint8_t  proto; 
  l4_uint8_t  prefix_len;
  l4_uint16_t port;
  l4_uint32_t address;
};

struct Nh_sadb_notify_head
{
  l4_uint16_t type;
  l4_uint16_t reserved;
};

struct Nh_sadb_acquire_msg
{
  struct Nh_sadb_notify_head type;
  l4_uint32_t satype;
  struct Nh_sp_address src;
  struct Nh_sp_address dst;
};

struct Nh_sadb_expire_msg
{
  struct Nh_sadb_notify_head type;
  struct Nh_sadb_sa_key      sa;
  struct Nh_sadb_sa_lifetime curr;
  l4_uint8_t                 expired;
  l4_uint8_t                 reserved1;
  l4_uint16_t                reserved2;
};

enum Nh_sadb_notify_type
{
  NH_SADB_NOTIFY_EMPTY   = 0,
  NH_SADB_NOTIFY_EXPIRE  = 1,
  NH_SADB_NOTIFY_ACQUIRE = 3,
  NH_SADB_NOTIFY_COMMITED= 1,
  NH_SADB_NOTIFY_WIP     = 4,
};

enum Nh_sadb_notify_size
{
  NH_SADB_NOTIFY_SZ_EXPIRE  = 48,
  NH_SADB_NOTIFY_SZ_ACQUIRE = 24,
  NH_SADB_NOTIFY_SZ_BUFFER  = 4080,
  NH_SADB_NOTIFY_BUFFER_ALIGN = 4096,
};

/// Interface direction values.
enum Nh_sadb_direcion {
  NH_SADB_IFACE_INBOUND  = 1, ///< Process inbound and outbound packets.
  NH_SADB_IFACE_OUTBOUND = 0, ///< Process only outbound packets (SPDB).
};

/// States of an SA.
enum Nh_sadb_sa_state {
  NH_SA_STATE_LARVAL = 0, ///< SPI allocated, but nothing else
  NH_SA_STATE_MATURE = 1, ///< Mature SA
  NH_SA_STATE_DYING  = 2, ///< SA with exceeded soft lifetime
  NH_SA_STATE_DEAD   = 3, ///< SA with exceeded hard lifetime
};

/// Types and modes of an SA.
enum Nh_sadb_sa_type {
  NH_SA_TYPE_UNSPEC   = 0, ///< Reserved.
  NH_SA_TYPE_PAS      = 1, ///< Pass packets plain
  NH_SA_TYPE_AH       = 2, ///< Do AH
  NH_SA_TYPE_ESP      = 3, ///< Do ESP
  NH_SA_TYPE_TUN_MASK = 4, ///< Mask for mode bit
  NH_SA_TYPE_MASK     = 3, ///< Mask for type bits
  NH_SA_TYPE_TUN_AH   = 6, ///< Do AH in tunnel mode
  NH_SA_TYPE_TUN_ESP  = 7, ///< Do ESP in tunnel mode
};

enum Nh_sadb_aalg {
  NH_AALG_NONE    = 0,
  NH_AALG_MD5HMAC = 1,
};

enum Nh_sadb_ealg {
  NH_EALG_NONE    = 0,
  NH_EALG_3DESCBC = 3,
};

#endif 

