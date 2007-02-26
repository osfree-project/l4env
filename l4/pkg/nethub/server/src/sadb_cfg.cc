/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nethub/nh-cfg-server.h>
#include <l4/nethub/base-types.h>
#include <l4/nethub/cfg-types.h>
#include "sadb.h"
#include "sadb_cfg.h"
#include "ip.h"
#include "ip_sec.h"
#include "ike_connector.h"

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

extern SADB_env env;

static u32 p2m(u32 p)
{
  u32 m;
  // special case for IA32 BUG
  if (p==0)
    return 0;

  m = (-1UL) << (32-p);
  return Net::h_to_n(m);
}

int Nh_cfg_listen_component(CORBA_Object o, l4_snd_fpage_t *msg_ring,
                            CORBA_Server_Environment *v)
{
  Ike_notify_buffer *b = env.sadb->ike()->buffer();
  msg_ring->snd_base = 0;
  msg_ring->fpage.fp.grant = 0;
  msg_ring->fpage.fp.write = 1;
  msg_ring->fpage.fp.size = 12; // 4k
  msg_ring->fpage.fp.page = (l4_umword_t)b >> 12;
  return NH_OK;
}

int
Nh_cfg_del_sa_component(CORBA_Object o,
                        const struct Nh_sadb_sa_key *key,
                        CORBA_Server_Environment *_dice_corba_env)
{
  // L4::cout << "del SA from " << *o << '\n';
  env.rtab->void_all(key->satype & ~NH_SA_TYPE_TUN_MASK, 
                     key->spi, key->dst_ip);
  switch (env.sadb->del(key->satype, key->spi, key->dst_ip ))
    {
    case SADB::EOK: return NH_OK;
    case SADB::ENOTFND: return NH_ENOENT;
    case SADB::EINVAL: 
    default: return NH_EINVAL;
    }
}

int
Nh_cfg_add_sa_component(CORBA_Object o,
                        const char *msg,
			unsigned size,
                        CORBA_Server_Environment *_dice_corba_env)
{
  Nh_sadb_sa_msg *m = (Nh_sadb_sa_msg*)(msg);
  
  // L4::cout << "add SA from " << *o << '\n';
  if (m->data.state != NH_SA_STATE_MATURE)
    return NH_EINVAL;

  if (m->data.dst_if >= env.ifl->num_ifs())
    return NH_EINVAL;
  
  char *enc_key = 0;
  char *auth_key = 0;
  
  if (m->data.auth)
    auth_key = (char*)(m + 1);

  if (m->data.encrypt)
    {
      if (auth_key)
	enc_key = auth_key + (m->data.auth_key_bits / 8);
      else
	enc_key = (char*)(m + 1);
    }
  
  Auth_algo *a = 0;
  Crypt_algo *c = 0;
  Ip_packet *i = 0;
  
  if (auth_key)
    a = Auth_algo::get( m->data.auth, m->data.auth_key_bits, auth_key );
  
  if (enc_key)
    c = Crypt_algo::get( m->data.encrypt, m->data.encrypt_key_bits, enc_key );
  
  if (m->key.satype & NH_SA_TYPE_TUN_MASK) 
    i = new Ip_packet( 4,             /* version */
	               5,             /* head length in 32 bit words */
		       0,             /* tos */
		       0,             /* len */
		       0,             /* id */
		       0,             /* frag_off */
		       64,            /* ttl */
		       Ip_packet::IP_IP_PROTO,
		       m->data.src_ip, m->key.dst_ip);
  
  SA *sa;
  SA::Lifetime hard(m->hard.bytes, m->hard.addtime*1000, m->hard.usetime*1000);
  SA::Lifetime soft(m->soft.bytes, m->soft.addtime*1000, m->soft.usetime*1000); 

  int ret = NH_EINVAL;
  
  switch (m->key.satype)
    {
    case NH_SA_TYPE_TUN_ESP:
      if (!i)
	goto fail_exit;
      
    case NH_SA_TYPE_ESP:
      if (!a && !c)
	goto fail_exit;

    case NH_SA_TYPE_PAS:
      sa = new SA(env.ifl->get(m->data.dst_if), m->key.satype, 
	          m->key.spi, m->key.dst_ip, a, c, i, soft, hard);
      switch (env.sadb->add(sa))
	{
	case SADB::EOK: env.rtab->update_sa(sa); return NH_OK;
	case SADB::EEXIST: ret = NH_EEXISTS; goto fail_exit;
	case SADB::EINVAL: 
	default: goto fail_exit;
	}
    default:
      goto fail_exit;
    }

  return NH_OK;
  
fail_exit:
  if (a) delete a;
  if (c) delete c;
  if (i) delete i;
  return ret;
}

int
Nh_cfg_update_sa_component(CORBA_Object o,
                           const char *msg,
		   	   unsigned size,
                           CORBA_Server_Environment *_dice_corba_env)
{
  Nh_sadb_sa_msg *m = (Nh_sadb_sa_msg*)(msg);
  
  // L4::cout << "add SA from " << *o << '\n';
  if (m->data.state != NH_SA_STATE_MATURE)
    return NH_EINVAL;

  if (m->data.dst_if >= env.ifl->num_ifs())
    return NH_EINVAL;
  
  char *enc_key = 0;
  char *auth_key = 0;
  
  if (m->data.auth)
    auth_key = (char*)(m + 1);

  if (m->data.encrypt)
    {
      if (auth_key)
	enc_key = auth_key + (m->data.auth_key_bits / 8);
      else
	enc_key = (char*)(m + 1);
    }
  
  Auth_algo *a = 0;
  Crypt_algo *c = 0;
  Ip_packet *i = 0;
  
  if (auth_key)
    a = Auth_algo::get( m->data.auth, m->data.auth_key_bits, auth_key );
  
  if (enc_key)
    c = Crypt_algo::get( m->data.encrypt, m->data.encrypt_key_bits, enc_key );
  
  if (m->key.satype & NH_SA_TYPE_TUN_MASK) 
    i = new Ip_packet( 4,             /* version */
	               5,             /* head length in 32 bit words */
		       0,             /* tos */
		       0,             /* len */
		       0,             /* id */
		       0,             /* frag_off */
		       64,            /* ttl */
		       Ip_packet::IP_IP_PROTO,
		       m->data.src_ip, m->key.dst_ip);
  
  SA::Lifetime hard(m->hard.bytes, m->hard.addtime, m->hard.usetime);
  SA::Lifetime soft(m->soft.bytes, m->soft.addtime, m->soft.usetime); 

  int ret = NH_EINVAL;
  
  switch (m->key.satype)
    {
    case NH_SA_TYPE_TUN_ESP:
    case NH_SA_TYPE_ESP:
    case NH_SA_TYPE_PAS:
      switch (env.sadb->update(env.ifl->get(m->data.dst_if), m->key.satype, 
 	                       m->key.spi, m->key.dst_ip, a, c, i, soft, hard))
	{
	case SADB::EOK: return NH_OK;
	case SADB::ENOTFND: ret = NH_ENOENT; goto fail_exit;
	case SADB::EINVAL: 
	default: goto fail_exit;
	}
    default:
      goto fail_exit;
    }
  return NH_OK;
  
fail_exit:
  if (a) delete a;
  if (c) delete c;
  if (i) delete i;
  return ret;
}

int
Nh_cfg_get_spi_component(CORBA_Object _dice_corba_obj,
                         l4_uint32_t satype,
                         l4_uint32_t dst_ip,
                         l4_uint32_t *min_spi,
                         l4_uint32_t max_spi,
                         CORBA_Server_Environment *_dice_corba_env)
{
  SA *sa = env.sadb->get_spi(satype, dst_ip, min_spi, max_spi);
  if (sa)
    {
      env.rtab->update_sa(sa);
      return NH_OK;
    }

  return NH_EINVAL;
}


#if 0
int
Nh_cfg_get_sa_component(CORBA_Object o,
    			const struct Nh_sadb_sa_key *key,
                        l4_uint8_t *msg,
			unsigned *size,
                        CORBA_Server_Environment *_dice_corba_env)
{
  return NH_EINVAL;
}
#endif

int
Nh_cfg_del_sp_component(CORBA_Object o,
                        CORBA_unsigned_int src_if,
                        const struct Nh_sp_address *src,
                        const struct Nh_sp_address *dst,
                        CORBA_Server_Environment *_dice_corba_env)
{
  switch (env.rtab->del(src_if, p2m(dst->prefix_len), dst->address, 
	                p2m(src->prefix_len), src->address,
		        0, 0 /*m->selector_mask, m->selector*/, src->proto))
    {
    case Eroute_table::EOK: return NH_OK;
    case Eroute_table::ENOTFND: return NH_ENOENT;
    default: return NH_EINVAL;
    }

}

int
Nh_cfg_add_sp_component(CORBA_Object o,
                        unsigned src_if,
                        const struct Nh_sp_address *src,
                        const struct Nh_sp_address *dst,
                        const struct Nh_sadb_sa_key *sak,
                        CORBA_Server_Environment *_dice_corba_env)
{
  Base_sa *sa;
  if (sak->spi)
    sa = env.sadb->get(sak->satype, sak->spi, sak->dst_ip);
  else
    sa = env.sadb->find_first(sak->satype, sak->dst_ip);

  if (!sa)
    sa = new Void_sa(sak->satype, sak->dst_ip);

  // L4::cout << "ADD SP using SA: "; sa->print(L4::cout); L4::cout << '\n';
  
  if (!sa)
    return NH_EINVAL;

  Eroute *e = new Eroute(src_if, p2m(dst->prefix_len), dst->address,
	                 p2m(src->prefix_len), src->address, 
			 0,0 /*m->selector_mask, m->selector*/, src->proto, sa);
  if (env.rtab->add(e))
    return NH_OK;
  else
    {
      delete e;
      return NH_EEXISTS;
    }
}

int
Nh_cfg_flush_sa_component(CORBA_Object o,
                          CORBA_Server_Environment *_dice_corba_env)
{
  env.rtab->void_all();
  env.sadb->flush();
  return NH_OK;
}

int
Nh_cfg_flush_sp_component(CORBA_Object o,
                          CORBA_Server_Environment *_dice_corba_env)
{
  env.rtab->flush();
  return NH_OK;
}

int
Nh_cfg_dump_sa_component(CORBA_Object o,
                         CORBA_Server_Environment *_dice_corba_env)
{
  L4::cout << "SADB dump\n";
  env.sadb->print(L4::cout);
  return NH_OK;
}

int
Nh_cfg_dump_sp_component(CORBA_Object o,
                         CORBA_Server_Environment *_dice_corba_env)
{
  L4::cout << "SPD dump\n";
  env.rtab->print(L4::cout);
  return NH_OK;

}

int
Nh_cfg_chg_iface_component(CORBA_Object _dice_corba_obj,
                           CORBA_unsigned_int niface,
                           CORBA_unsigned_int direction,
                           CORBA_Server_Environment *_dice_corba_env)
{
  env.rtab->if_mode(niface, direction);
  return NH_OK;
}

