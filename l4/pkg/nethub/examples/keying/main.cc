/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#include <l4/cxx/main_thread.h>
#include <l4/cxx/thread.h>
#include <l4/cxx/l4types.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/nethub/client.h>
#include <l4/nethub/cfg.h>

#include <stddef.h>

char LOG_tag[9]=LOG_TAG;

class Main : public L4::MainThread
{
public:
  void run();
};

// espenckey=0x54f11985_0594ceea_3d37893a_6be34b67_f31eab0d_86e4f0dc
static char des_cbc_key[] =
{ 0x54, 0xf1, 0x19, 0x85,
  0x05, 0x94, 0xce, 0xea,
  0x3d, 0x37, 0x89, 0x3a,
  0x6b, 0xe3, 0x4b, 0x67,
  0xf3, 0x1e, 0xab, 0x0d,
  0x86, 0xe4, 0xf0, 0xdc };

// espauthkey=0xc7b5fa7a_132f0868_d9dd39cb_6607fe35
static char hmac_md5_key[] = 
{ 0xc7, 0xb5, 0xfa, 0x7a,
  0x13, 0x2f, 0x08, 0x68,
  0xd9, 0xdd, 0x39, 0xcb,
  0x66, 0x07, 0xfe, 0x35 };

Main my_main;
L4::MainThread *main = &my_main;

static char sadb_msg_buf[1024];

struct Sadb_add_sa_msg 
{
  Nh_sadb_sa_msg m;
  char akey_bits[128/8];
  char ekey_bits[192/8];
};

void Main::run()
{
  Hub hub;
  int err;
  
  L4::cout << "Keying -- L4 Point To Point Network Hub\n";

  hub = nh_resolve_hub("l4/nethub");
  if (l4_is_invalid_id(hub))
    {
      L4::cout << "ERROR connecting to nethub (not found)\n";
      return;
    }

  L4::cout << "Found hub @ " << hub << "\n";

  L4::cout << "Do SADB keying stuff\n"
              "Flush SADB\n";

  nh_sadb_flush_sp(hub);
  nh_sadb_flush_sa(hub);
  
  L4::cout << "Add inbound SA\n";

  Sadb_add_sa_msg *sadb_m = (Sadb_add_sa_msg*)sadb_msg_buf;
  sadb_m->m.key.satype    = NH_SA_TYPE_TUN_ESP;
  sadb_m->m.key.spi       = 0x00020000; /* 0x200 */
  sadb_m->m.key.dst_ip	  = 0x5d304c8d; /* 141.76.48.93 */
  sadb_m->m.data.dst_if   = 1;
  sadb_m->m.data.state    = NH_SA_STATE_MATURE;
  sadb_m->m.data.auth     = NH_AALG_MD5HMAC;
  sadb_m->m.data.auth_key_bits = 128;
  sadb_m->m.data.encrypt  = NH_EALG_3DESCBC;
  sadb_m->m.data.encrypt_key_bits = 192;
  sadb_m->m.data.src_ip   = 0x13304c8d; /* 141.76.48.19 */
  
  for (unsigned i=0; i<sizeof(sadb_m->akey_bits); ++i)
    sadb_m->akey_bits[i] = hmac_md5_key[i];

  for (unsigned i=0; i<sizeof(sadb_m->ekey_bits); ++i)
    sadb_m->ekey_bits[i] = des_cbc_key[i];
  
  err = nh_sadb_add_sa(hub, &sadb_m->m, sizeof(Sadb_add_sa_msg));

  if (err != NH_OK)
    L4::cout << "ERROR adding SA\n";

  L4::cout << "Add outbound SA\n";
  
  sadb_m->m.key.spi       = 0x00020000; /* 0x200 */
  sadb_m->m.key.dst_ip	  = 0x13304c8d; /* 141.76.48.19 */
  sadb_m->m.data.src_ip   = 0x5d304c8d; /* 141.76.48.93 */
  sadb_m->m.data.dst_if   = 0;
  
  err = nh_sadb_add_sa(hub, &sadb_m->m, sizeof(Sadb_add_sa_msg));

  if (err != NH_OK)
    L4::cout << "ERROR adding SA\n";

  L4::cout << "Add eroute\n";
  static Nh_sp_address src, dst;
  static Nh_sadb_sa_key sa;
  sa.satype = NH_SA_TYPE_TUN_ESP;
  sa.spi = 0x00020000; // 0x200
  sa.dst_ip = 0x13304c8d;
  dst.prefix_len = 0;
  dst.address = 0;
  src.proto = 0;
  src.prefix_len = 0;
  src.address = 0;

  err = nh_sadb_add_sp(hub, -1UL /*src_if*/, &src, &dst, &sa);

  if (err != NH_OK)
    L4::cout << "ERROR adding SA\n";

  L4::cout << "Dump SADB\n";

  nh_sadb_dump_sa(hub);
  nh_sadb_dump_sp(hub);

}

