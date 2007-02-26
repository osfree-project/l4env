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

class Mapper_thread : public L4::Thread
{
public:
  Mapper_thread();
  void run();
private:
  l4_umword_t stack[1024];
};

class Test_thread : public L4::Thread
{
public:
  Test_thread( unsigned id );
  Nh_iface iface;
  Nh_packet_ring queue;
private:
  l4_umword_t stack[1024];
};

class Snd_thread : public Test_thread
{
public:
  Snd_thread() : Test_thread(10) {}
  void run();

private:
  char rcv_buffer[4096];
};

class Rcv_thread : public Test_thread
{
public:
  Rcv_thread() : Test_thread(11) {}
  void run();

private:
  char rcv_buffer[4096];
};

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

static char *snd_packet = 0;
static char *rcv_packet = 0;
static unsigned snd_packet_len = 0;
static unsigned rcv_packet_len = 0;

extern char _binary_snd_packets_src_start;
extern char _binary_rcv_packets_src_start;
extern char _binary_snd_packets_src_end;
extern char _binary_rcv_packets_src_end;

Mapper_thread mapper;
Snd_thread s;
Rcv_thread r;
Hub hub;

extern char _end;
extern char _start;

struct Nh_sadb_notify_head *const nb 
  = (struct Nh_sadb_notify_head*)(0xa8000000);

static char sadb_msg_buf[1024];

struct Sadb_add_sa_msg 
{
  Nh_sadb_sa_msg m;
  char akey_bits[128/8];
  char ekey_bits[192/8];
};

void wait_for(L4::Thread *t)
{
  l4_msgdope_t result;
  l4_umword_t w1,w2;
  
  if (!t || t->state() != L4::Thread::Running)
    return;

  l4_ipc_receive(t->self(), 0, &w1, &w2, L4_IPC_NEVER, &result);
}

class Wakeup_guard
{
public:
  Wakeup_guard(L4::Thread *t) : t(t) 
  {}
  
  ~Wakeup_guard()
  {
    l4_msgdope_t result;
    l4_ipc_send(t->self(), 0, 0, 0, L4_IPC_BOTH_TIMEOUT_0, &result);
  }

private:
  L4::Thread *t;
};

void Main::run()
{
  int err;
  unsigned tmp;
  
  L4::cout << "Test -- L4 Point To Point Network Hub\n"
           << "Main thread is " << self() << '\n';
  


  hub = nh_resolve_hub("l4/nethub");
  
  if (l4_is_invalid_id(hub))
    {
      L4::cout << "ERROR connecting to nethub (not found)\n";
      return;
    }

  mapper.start();

  L4::cout << "Found hub @ " << hub << "\n"
           << "Do master connect\n";
  
  
  extern char _stext;
  extern char _etext;

  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  L4::cout << "Request notify buffer: ";

  if (nh_sadb_map_notify_buffer(hub, nb) != NH_OK)
    {
      L4::cout << "failed\n"; 
      return;
    }

  L4::cout << "done\n";

  r.iface.mapper = mapper.self();
  r.iface.shared_mem_start = &_start;
  r.iface.shared_mem_end   = &_end;
  r.iface.out_ring = &r.queue;
  r.iface.rcv_thread = r.self();

  if (nh_if_open(hub, 0, &r.iface) != NH_OK)
    {

      L4::cout << "ERROR connecting master to nethub port 0\n"; 
      return;
    }

  L4::cout << "Receive partner is " << r.iface.out << " -> " 
           << r.iface.in << "\n";
  

  s.iface.mapper = mapper.self();
  s.iface.shared_mem_start = &_start;
  s.iface.shared_mem_end   = &_end;
  s.iface.out_ring = &s.queue;
  s.iface.rcv_thread = s.self();
  
  L4::cout << "Do slave connect\n";
  if (nh_if_open(hub, 1, &s.iface ) != NH_OK)
    {
      L4::cout << "ERROR connecting slave to nethub port 0\n"; 
      return;
    }

  L4::cout << "Send partner is " << s.iface.out << " -> " << s.iface.in << "\n";

  L4::cout << "Initialize SADB\n";
  
  if ((tmp=nh_sadb_flush_sp(hub)) != NH_OK)
    {
      L4::cout << "  error flushing SPDB for hub(" << hub << ") port 0\n"
	       << "ABORT\n";
      return;
    }
  
  if ((tmp=nh_sadb_flush_sa(hub)) != NH_OK)
    {
      L4::cout << "  error flushing SADB for hub(" << hub << ") port 0\n"
	       << "ABORT\n";
      return;
    }

  
  nh_sadb_chg_iface(hub, 1, NH_SADB_IFACE_INBOUND);
  
  L4::cout << "Add inbound SA\n";

  Sadb_add_sa_msg *sadb_m = (Sadb_add_sa_msg*)sadb_msg_buf;
  sadb_m->m.key.satype    = NH_SA_TYPE_TUN_ESP;
  sadb_m->m.key.spi       = 0x00020000; /* 0x200 */
  sadb_m->m.key.dst_ip	  = 0x0100a8c0; /*192.168.0.1*/
  sadb_m->m.data.state    = NH_SA_STATE_MATURE;
  sadb_m->m.data.auth     = NH_AALG_MD5HMAC;
  sadb_m->m.data.auth_key_bits = 128;
  sadb_m->m.data.encrypt  = NH_EALG_3DESCBC;
  sadb_m->m.data.encrypt_key_bits = 192;
  sadb_m->m.data.src_ip   = 0x0202a8c0; /*192.168.2.2*/
  sadb_m->m.hard.bytes = -1ULL; //INF
  sadb_m->m.soft.bytes = -1ULL; //INF
  
  for (unsigned i=0; i<sizeof(sadb_m->akey_bits); ++i)
    sadb_m->akey_bits[i] = hmac_md5_key[i];

  for (unsigned i=0; i<sizeof(sadb_m->ekey_bits); ++i)
    sadb_m->ekey_bits[i] = des_cbc_key[i];
  
  err = nh_sadb_add_sa(hub, &sadb_m->m, sizeof(Sadb_add_sa_msg));

  if (err != NH_OK)
    {
      L4::cout << "error adding SA\n"
	       << "ABORT\n";
      return;
    }


  L4::cout << "Add SPD entry\n";
  static Nh_sp_address src, dst;
  static Nh_sadb_sa_key sa;
  
  sa.satype = NH_SA_TYPE_TUN_ESP;
  sa.spi = 0x00020000; // 0x200
  sa.dst_ip = 0x0200a8c0;
  dst.prefix_len = 0;
  dst.address = 0;
  dst.port = 0xffff;
  src.prefix_len = 0;
  src.address = 0;
  src.proto = 0;
  src.port = 0xffff;

  err = nh_sadb_add_sp(hub, 0, &src, &dst, &sa);

  if (err != NH_OK)
    {
      L4::cout << "error adding SPD entry\n"
	       << "ABORT\n";
      return;
    }
  
  L4::cout << "Dump SADB\n";

  err = nh_sadb_dump_sa(hub);
  err = nh_sadb_dump_sp(hub);
  
  s.start();
  r.start();

  wait_for(&r);
  wait_for(&s);
//  l4_sleep(2000);

  unsigned tx,rx,txd,rxd;

  nh_if_stats(hub, 0, &tx, &txd, &rx, &rxd);
  L4::cout << "IF[0]: tx=" << tx << " txd=" << txd 
           << " rx=" << rx << " rxd=" << rxd << '\n';
  nh_if_stats(hub, 1, &tx, &txd, &rx, &rxd);
  L4::cout << "IF[1]: tx=" << tx << " txd=" << txd 
           << " rx=" << rx << " rxd=" << rxd << '\n';

  while(1); 
}

Mapper_thread::Mapper_thread()
  : Thread( stack + (sizeof(stack)/sizeof(stack[0]) -1 ))
{}

void Mapper_thread::run()
{
  L4::cout << "Mapper thread for nethub is " << self() << '\n';
  nh_region_mapper( &_start, &_end );
}

Test_thread::Test_thread( unsigned id )
  : Thread( stack + (sizeof(stack)/sizeof(stack[0]) -1 ), id )
{}

static bool send_pkt( l4_threadid_t partner, Nh_packet_ring *q, 
                      char const *name, char *pkt, unsigned len )
{
  int err;
  l4_msgdope_t result;

  if (q->ring[q->next].flags & 1)
    {
      L4::cout << name << "packet ring overflow -> drop\n";
      return false;
    }

  q->ring[q->next].pkt = pkt;
  q->ring[q->next].flags = 1;
  q->next = (q->next + 1) % 32;

  if (q->empty==0)
    {
      L4::cout << name << "send wakeup IPC to hub\n";
      err = l4_ipc_send( partner, 0, 0, 0, L4_IPC_SEND_TIMEOUT_0, &result );
  
#if 0
  L4::cout << name << "send packet (len=" << L4::dec << len << ") ...\n";
  err = l4_ipc_call( partner,
                     0, (l4_umword_t)pkt, len,
		     0, &w1, &w2,
		     L4_IPC_TIMEOUT(0,0,153,7,0,0),
		     &result );
#endif
  if (!err || err == L4_IPC_SETIMEOUT) 
    {
      L4::cout << name << "send OK\n"; 
      return true;
    }
  else
    {
      L4::cout << name << "send ERROR (" << (L4::MsgDope)result << ")\n";
      return false;
    }
    }
  return true;
}

static bool recv_pkt( l4_threadid_t partner, char const *name, 
                      char *buffer, unsigned long &len )
{
  l4_umword_t w1,w2;
  int err;
  l4_msgdope_t result;
  L4::cout << name << "receive packet ...\n"; 
  err = l4_ipc_call( partner, 
	             0, (l4_umword_t)buffer, 
		     len | 0x80000000,
		     0, &w1, &w2,
		     L4_IPC_TIMEOUT(0,0,153,7,0,0),
		     &result );
  if (!err)
    {
      L4::cout << name << "receive OK (len=" << L4::dec << w2 << ")\n";
      len = w2;
      return true;
    }
  else
    {
      L4::cout << name << "receive ERROR (" << (L4::MsgDope)result << ")\n";
      return false;
    }
}

void Snd_thread::run()
{
  Wakeup_guard guard(main);
  L4::cout << "Snd thread is " << self() << '\n';
  unsigned long w2;
  char *snd_packets_end = &_binary_snd_packets_src_end;
  snd_packet = &_binary_snd_packets_src_start;
  
  L4::cout << "Sender (" << self() << ") thread is up\n";
  while(1)
    {
      snd_packet_len = *(((l4_umword_t*&)snd_packet)++);
      if (!send_pkt( iface.out, &queue, "S1: ", snd_packet, snd_packet_len ))
	{
	  L4::cout << "retry\n";
	  continue;
	}
      w2 = sizeof(rcv_buffer);
      if (!recv_pkt( iface.in, "R2: ", rcv_buffer, w2 ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
      if (w2 != snd_packet_len) 
	{
	  L4::cout << "R2: length mismatch\n";
	  return;
	}

      L4::cout << "R2: Length OK\n"; 
#if 0
      if (!send_pkt( snd_p,&queue, "S3: ", rcv_buffer, w2 ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
#endif	  
      snd_packet += snd_packet_len;
      if (snd_packet >= snd_packets_end) 
	{
	  L4::cout << "done\n";
	  return;
	}
    }
}

static bool check_pkt( char const *name, unsigned long a_len, char const *a_pkt,
                       unsigned long b_len, char const *b_pkt )
{
      if (a_len != b_len)
	{
	  L4::cout << name << "Length mismatch: " << L4::dec 
	    << (unsigned)(a_len) << " != " 
	    << (unsigned)(b_len) << "\n";
	  return false;
	}
      
      L4::cout << name << "Length: OK\n";
	    
      for (unsigned x= 0; x<b_len ; x++)
	{
	  if ( a_pkt[x] != b_pkt[x])
	    {
	      L4::cout << name << "Packet content mismatch: offset=" << L4::dec
		<< x << ": " << (unsigned)a_pkt[x]
		<< " != " << (unsigned)b_pkt[x] << "\n";
	      return false;
	    }
	}
      L4::cout << name << "Packet content: OK\n";
      return true;
}


void Rcv_thread::run()
{
  Wakeup_guard guard(main);
  L4::cout << "Rcv thread is " << self() << '\n';
  enum {
    //chk_offset = 20, // offset for Nethub in transport mode
    chk_offset = 0, // offset for nethub in tunnel mode
  };
  unsigned long w2;

  rcv_packet = &_binary_rcv_packets_src_start;
  
  L4::cout << "Receiver (" << self() << ") thread is up\n";
  while (1) 
    {
      w2 = sizeof(rcv_buffer);
      if (!recv_pkt( iface.in, "R1: ", rcv_buffer, w2 ))
	{
	  L4::cout << "retry\n";
	  continue;
	}

      rcv_packet_len = *(((l4_umword_t*&)rcv_packet)++);
	  
      if (!check_pkt( "R1: ", w2 - chk_offset, rcv_buffer + chk_offset, 
	              rcv_packet_len, rcv_packet ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
      
      if (!send_pkt( iface.out, &queue, "S2: ", rcv_buffer, w2 ))
	{
	  L4::cout << "Abort\n";
	  return;
	}

      l4_sleep(10);

      struct Nh_sadb_notify_head *notify;
   
      notify = nh_sadb_next_notification(nb);

      if (!notify)
	{
	  L4::cerr << "We should have been notified about a missing SA,\n"
	           << "but were not -> Abort\n";
	  return;
	}

      if (notify->type != NH_SADB_NOTIFY_ACQUIRE)
	{
	  L4::cerr << "Got wrong notification type: " 
	           << (unsigned)(notify->type) << "\nAbort\n";
	  return;
	}

      struct Nh_sadb_acquire_msg *acq = (struct Nh_sadb_acquire_msg *)notify;
      
      if (acq->satype != NH_SA_TYPE_ESP)
	{
	  L4::cerr << "Got acquire for wrong SA type: " << acq->satype
	           << "\nAbort\n";
	  return;
	}

      nh_sadb_clear_notification(notify);
   
      L4::cout << "Add outbound SA\n";
  
      Sadb_add_sa_msg *sadb_m = (Sadb_add_sa_msg*)sadb_msg_buf;
      sadb_m->m.key.satype    = NH_SA_TYPE_TUN_ESP;
      sadb_m->m.key.dst_ip    = 0x0200a8c0; /*192.168.0.2*/
      sadb_m->m.data.src_ip   = 0x0102a8c0; /*192.168.2.1*/
      sadb_m->m.data.dst_if   = 1;
      sadb_m->m.key.spi       = 0x00020000; /* 0x200 */
      sadb_m->m.data.state    = NH_SA_STATE_MATURE;
      sadb_m->m.data.auth     = NH_AALG_MD5HMAC;
      sadb_m->m.data.auth_key_bits = 128;
      sadb_m->m.data.encrypt  = NH_EALG_3DESCBC;
      sadb_m->m.data.encrypt_key_bits = 192;
      sadb_m->m.hard.bytes = -1ULL; //INF
      sadb_m->m.soft.bytes = -1ULL; //INF

      int err = nh_sadb_add_sa(hub, &sadb_m->m, sizeof(Sadb_add_sa_msg));

      if (err != NH_OK)
	{
	  L4::cout << "error adding SA\n"
	    << "ABORT\n";
	  return;
	}
#if 0
      
      L4::cout << "Change SPD entry\n";
      static Nh_sp_address src, dst;
      static Nh_sadb_sa_key sa;

      sa.satype = NH_SA_TYPE_TUN_ESP;
      sa.spi = 0x00020000; // 0x200
      sa.dst_ip = 0x0200a8c0;
      dst.prefix_len = 0;
      dst.address = 0;
      dst.port = 0xffff;
      src.prefix_len = 0;
      src.address = 0;
      src.proto = 0;
      src.port = 0xffff;

      if (nh_sadb_chg_sp(hub, 0, &src, &dst, &sa) != NH_OK)
	{
	  L4::cout << "error changing SPD entry\n"
	    << "ABORT\n";
	  return;
	}
#endif
      
      if (!send_pkt( iface.out, &queue, "S2x: ", rcv_buffer, w2 ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
#if 0	  
      w2 = sizeof(rcv_buffer);
      if (!recv_pkt( rcv_p, "R3: ", rcv_buffer, w2 ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
      
      if (!check_pkt( "R3: ", w2 - chk_offset, rcv_buffer + chk_offset,
	              rcv_packet_len, rcv_packet ))
	{
	  L4::cout << "Abort\n";
	  return;
	}
#endif 
      rcv_packet += rcv_packet_len;
      if (rcv_packet >= &_binary_rcv_packets_src_end) 
	{
	  L4::cout << "Test done\nSUCCESS\n";
	  return;
	}
    }
}

