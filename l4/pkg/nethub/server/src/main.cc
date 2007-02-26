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

#include <l4/names/libnames.h>
#include <l4/util/util.h>

#include <l4/cxx/main_thread.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/alloc.h>
#include <l4/cxx/l4types.h>

#include <l4/nethub/cfg-types.h>
#include <l4/nethub/client-types.h>
#include <l4/nethub/base-types.h>

#include <l4/nethub/nh-server.h>

#include "ike_connector.h"
#include "ip_forward.h"
#include "ip_sec.h"
#include "ip_sec_fwd.h"
#include "ip.h"
#include "sadb.h"

#include "pager.h"
#include "sadb_cfg.h"
#include "routing_fab.h"
#include "time.h"
//#include "main_interface.h"

#include <stddef.h>

static bool const debug_malloc = 0;


static char alloc_buffer[4096*20];
L4::Alloc_list allocator( alloc_buffer, sizeof(alloc_buffer) );

extern "C" void *malloc( size_t size )
{
  unsigned *p = (unsigned *)allocator.alloc(size + sizeof(unsigned));
  if (debug_malloc)
    L4::cout << "\033[1;32mmalloc(" << L4::dec << size << L4::hex << ") = "
             << p << "\033[0m\n";
  if (!p) 
    return 0;
  *p = size + sizeof(unsigned);
  return p + 1;  
}

extern "C" void *CORBA_alloc(unsigned long size) __attribute__((alias("malloc")));

extern "C" void CORBA_free(void *b) __attribute__((alias("free")));

extern "C" void free( void *blk )
{
  unsigned *b = (unsigned*)blk;
  if (debug_malloc)
    L4::cout << "\033[1;34mfree(" << L4::hex << blk << L4::dec << ") [size="
             << *(b-1) << "]\033[0m\n";
  allocator.free(b-1,*(b-1));
}

void *operator new (size_t size) throw()
{
  unsigned *p = (unsigned *)allocator.alloc(size + sizeof(unsigned));
  if (!p)
    {
      L4::cerr << "error: allocator out of memory\nAbort\n";
      enter_kdebug("OFM");
    }
#if 0
  L4::cout << "alloc: " << L4::hex << p << L4::dec << " size=" 
           << size + sizeof(unsigned) << '\n';
#endif
  *p = size + sizeof(unsigned);
  
  return p + 1;  
}

void operator delete (void *blk) throw()
{
  unsigned *b = (unsigned*)blk;
#if 0
  L4::cout << "free: " << L4::hex << (b-1) << L4::dec << " size=" 
           << *(b-1) << '\n';
#endif
  allocator.free(b-1,*(b-1));
}

void *operator new [] (size_t size) throw()
{
  unsigned *p = (unsigned *)allocator.alloc(size + sizeof(unsigned));
  if (!p)
    {
      L4::cout << "error: allocator out of memory\nAbort\n";
      enter_kdebug("OFM");
    }
  *p = size + sizeof(unsigned);
  
  return p + 1;  
}

void operator delete [] (void *blk) throw()
{
  unsigned *b = (unsigned*)blk;
  allocator.free(b-1,*(b-1));
}

class Ip_sec_iface : public Iface, public Ip_ipsec_fwd
{
public:
  Ip_sec_iface() : Iface(), Ip_ipsec_fwd((Iface*)this)
  {}
};


char LOG_tag[9]=LOG_TAG;

class Main : public L4::MainThread
{
public:
  void run();
};

enum {
  NUM_IFS = 10,
};

Main my_main;
L4::MainThread *main = &my_main;
static Pager pager;

static Iface_vector ifs(NUM_IFS);
static Ip_fwd *fwds[NUM_IFS];
static Ike_connector ike_conn("l4/nethub/ike");
static SADB sadb(&ike_conn);
static Eroute_table rtab(&sadb, &ike_conn);
SADB_env env(&ifs, &rtab, &sadb);
static Routing_fab router(&rtab, &ifs);

//static char sadb_msg_buffer[1024];


void Main::run()
{
#if 0
  l4_threadid_t th,mapper;
  l4_msgdope_t result;
  l4_umword_t d1 = 0;
  struct {
    l4_fpage_t fp;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t w[5];
    l4_strdope_t m;
  } msg;

  int err;
  unsigned port;
#endif
  L4::cout << "L4 Point To Point Network Hub\n";
  L4::cout << '(' << self() << ") Nethub master is up\n";

  Time::init();
  u64 t = Time::get_time();

  L4::cout << "Starting time: " << t << '\n';
  
  names_register("l4/nethub");
  pager.start();

  Thread::set_pager( pager.self() );

  ike_conn.start();
  router.start();

  Nh_server_loop(0);
}

int
Nh_client_open_component(CORBA_Object client,
                         CORBA_unsigned_int niface,
                         struct Nh_iface *iface,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (niface>=NUM_IFS)
    {
      L4::cerr << "warning: non existing hub requested: " << niface
	       << "(0-" << (unsigned)NUM_IFS << " allowed)\n";
      return NH_EINVAL;
    }

#if 0
  L4::cout << "connect to interface " << niface 
           << " from " << *client << '\n';
#endif
  
  if (fwds[niface])
    fwds[niface]->stop();
  else
    fwds[niface] = new Ip_ipsec_fwd(env.ifl->get(niface));

  fwds[niface]->snd(iface->rcv_thread);
  fwds[niface]->rcv(my_main.self());

  iface->out = router.self();
  iface->in  = fwds[niface]->self();
#if 0
  msg.w[1] = router.self().id.lthread
    | (fwds[port]->self().id.lthread << 8);
#endif
	      
  env.ifl->get(niface)->peer(fwds[niface]->self());

  if (env.ifl->set(niface, iface->out_ring,
	           (long unsigned)iface->shared_mem_start,
	           iface->mapper, &router, iface->txe_irq))
    {
      pager.add_region(env.ifl->get(niface)->region());
      fwds[niface]->start();
//    L4::cout << "Interface " << niface << " is up\n";
      return NH_OK;
    }
  else
    {
      L4::cerr << "warning: interface setup failed -> may be the packet "
	          "ring is not in shmem\n";
      return NH_EINVAL;
    }
}

int
Nh_client_close_component(CORBA_Object o,
                          CORBA_unsigned_int niface,
                          CORBA_Server_Environment *_dice_corba_env)
{
  if (niface >= NUM_IFS)
    return NH_EINVAL;

  if (fwds[niface])
    fwds[niface]->stop();
  else
    return NH_ENOENT;

  return NH_OK;
}

CORBA_int
Nh_client_stats_component(CORBA_Object o,
                          CORBA_unsigned_int niface,
                          unsigned *tx,
                          unsigned *txd,
                          unsigned *rx,
                          unsigned *rxd,
                          CORBA_Server_Environment *_dice_corba_env)
{
  if (niface >= NUM_IFS)
    return NH_EINVAL;

  Iface *iface = env.ifl->get(niface);

  if (!iface)
    return NH_ENOENT;

  iface->get_stats(*rx,*rxd,*tx,*txd);

  return NH_OK;
}
