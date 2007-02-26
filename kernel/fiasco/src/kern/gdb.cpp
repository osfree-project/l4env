INTERFACE:

#include "gdb_server.h"
#include "types.h"
#include "jdb_handler_queue.h"
#include "jdb.h"

class Gdb_s : public Gdb_serv
{
public:

  Gdb_s();
  unsigned char *get_register( unsigned index, unsigned &size );
  unsigned num_registers() const;
  bool push_register( unsigned index ) const;
  unsigned pc_index() const;
  Tlb lookup( unsigned long v_addr ) const;
  
  Jdb_entry_frame *get_entry_frame();

protected:
  unsigned in_buffer_len() const;
  unsigned char *in_buffer();
  unsigned out_buffer_len() const;
  unsigned char *out_buffer();
 
};


class Gdb
{
public:
  static void init();
  static void enter(Jdb_entry_frame *e, char const *msg) asm ("enter_gdb");
//  static Console *console();
  static Jdb_entry_frame *entry_frame;
  static Jdb_handler_queue enter_q;
  static Jdb_handler_queue leave_q;
  
private:
  static Gdb_s *gdb;
};

IMPLEMENTATION:

#include "kernel_uart.h"
#include "kernel_console.h"
#include "filter_console.h"
#include "static_init.h"
#include "processor.h"

#include "jdb_module.h"
#include "jdb.h"

Gdb_s *Gdb::gdb;
Jdb_entry_frame *Gdb::entry_frame;
Jdb_handler_queue Gdb::enter_q;
Jdb_handler_queue Gdb::leave_q;


IMPLEMENT
Gdb_s::Gdb_s()
  : Gdb_serv(Kernel_uart::uart())
{
  Kconsole::console()->register_console(this);
  Kconsole::console()->end_exclusive(DEBUG);
}


IMPLEMENT
unsigned Gdb_s::in_buffer_len() const
{
  return 1024;
}

IMPLEMENT
unsigned char *Gdb_s::in_buffer()
{
  static unsigned char buf[1024];
  return buf;
}

IMPLEMENT
unsigned Gdb_s::out_buffer_len() const
{
  return 1024;
}

IMPLEMENT
unsigned char *Gdb_s::out_buffer()
{
  static unsigned char buf[1024];
  return buf;
}

//IMPLEMENT
//Console *Gdb::console()
//{
//  return gdb;
//}

STATIC_INITIALIZE_P(Gdb, GDB_INIT_PRIO);
#include <cstdio>
IMPLEMENT
void Gdb::init()
{
  static Gdb_s xgdb;
  puts("initialized GDB");
  gdb = &xgdb;
}

IMPLEMENT
void Gdb::enter(Jdb_entry_frame *e, char const *msg)
{
  enter_q.execute();
  
  entry_frame = e;
  if (msg && msg[0] == 'I')
    gdb->enter(2);
  else
    gdb->enter(5);
  
  leave_q.execute();
}

//===================
// Std JDB modules
//===================

/**
 * 'IRQ' module.
 * 
 * This module handles the 'R' command that
 * provides IRQ attachment and listing functions.
 */
class Jdb_gdb
  : public Jdb_module
{
};

static Jdb_gdb jdb_gdb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC Jdb_module::Action_code
Jdb_gdb::action( int cmd, void *&/*args*/, char const *&/*fmt*/, int & )
{
  if (cmd!=0)
    return NOTHING;

  puts("\nWaiting for GDB attach (do not press any key)");

  Jdb::alt_entry = &Gdb::enter;
  Kconsole::console()->start_exclusive(Console::DEBUG);
  
  while (!Kconsole::console()->char_avail()) 
    Proc::pause();
  
  return LEAVE;
}

PUBLIC
int const Jdb_gdb::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_gdb::cmds() const
{
  static Cmd cs[] =
    {{ 0, "G", "", "", "V\tswitch to GDB mode", 0 }};

  return cs;
}

