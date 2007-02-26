INTERFACE:

#include "gdb_server.h"
#include "types.h"

class Gdb_entry_frame 
{
};

class Gdb_s : public Gdb_serv
{
public:

  Gdb_s();
  void get_register( unsigned index );
  void set_register( unsigned index, char const *buffer ); 
  unsigned num_registers() const;
  Gdb_entry_frame *get_entry_frame();
 
};


class Gdb
{
public:
  static void init();
  static void enter(Gdb_entry_frame *e) asm ("enter_jdb");
  static Console *console();
private:
  static Gdb_s *gdb;
};

IMPLEMENTATION:

#include "kernel_console.h"
#include "static_init.h"

Gdb_s *Gdb::gdb;

IMPLEMENT
Console *Gdb::console()
{
  return gdb;
}

STATIC_INITIALIZE_P(Gdb, KDB_INIT_PRIO);

IMPLEMENT
void Gdb::init()
{
  static Gdb_s xgdb;
  gdb = &xgdb;
  Kconsole::console()->register_console(gdb, 0);
}

IMPLEMENT
void Gdb::enter(Gdb_entry_frame *e)
{
  gdb->enter();
}
