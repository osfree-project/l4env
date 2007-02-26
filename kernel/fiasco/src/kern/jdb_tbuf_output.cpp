INTERFACE:

#include "types.h"

class L4_uid;
class Tb_entry;

class Jdb_tbuf_output
{
public:
  static int  thread_eip (int e_nr, L4_uid *tid, Mword *eip);
  static void print_entry(Tb_entry *e, char *buf, int maxlen);
  static void print_entry(int e_nr, char *buf, int maxlen);
  static void init();
  typedef unsigned (Format_entry_fn)(Tb_entry *tb, L4_uid tid, 
				     char *buf, int maxlen);
  static void register_ff(Unsigned8 type, Format_entry_fn format_entry_fn);

private:
  static Format_entry_fn *_format_entry_fn[];
};

IMPLEMENTATION:

#include <cstdarg>
#include <cstdlib>
#include <cstdio>

#include "assert.h"
#include "config.h"
#include "initcalls.h"
#include "jdb_symbol.h"
#include "jdb_tbuf.h"
#include "kdb_ke.h"
#include "kernel_console.h"
#include "l4_types.h"
#include "processor.h"
#include "static_init.h"
#include "tb_entry.h"
#include "thread.h"
#include "watchdog.h"


#define MAX_TBUF	16


Jdb_tbuf_output::Format_entry_fn *Jdb_tbuf_output::_format_entry_fn[MAX_TBUF];

static void
console_log_entry(Tb_entry *e, const char *)
{
  static char log_message[80];

  // disable all interrupts to stop other threads
  Watchdog::disable();
  Proc::Status s = Proc::cli_save();
  
  Jdb_tbuf_output::print_entry(e, log_message, sizeof(log_message));
  printf("%s\n", log_message);
  
  // do not use getchar here because we ran cli'd 
  // and getchar may do halt
  int c;
  while ((c=Kconsole::console()->getchar(false)) == -1)
    Proc::pause();
  if( c == 'i')
    jdb_enter_kdebug("IPC");

  // enable interrupts we previously disabled
  Proc::sti_restore(s);
  Watchdog::enable();
}

PRIVATE static
unsigned
Jdb_tbuf_output::dummy_format_entry(Tb_entry *tb, L4_uid, char *buf, int maxlen)
{
  return snprintf(buf, maxlen,
	 " << no format_entry_fn for type %d registered >>", tb->type());
}

STATIC_INITIALIZE(Jdb_tbuf_output);

IMPLEMENT FIASCO_INIT
void
Jdb_tbuf_output::init()
{
  unsigned i;

  Jdb_tbuf::direct_log_entry = &console_log_entry;
  for (i=0; i<sizeof(_format_entry_fn)/sizeof(_format_entry_fn[0]); i++)
    if (!_format_entry_fn[i])
      _format_entry_fn[i] = dummy_format_entry;
}

IMPLEMENT
void
Jdb_tbuf_output::register_ff(Unsigned8 type, Format_entry_fn format_entry_fn)
{
  assert(type < MAX_TBUF);

  if (   (_format_entry_fn[type] != 0)
      && (_format_entry_fn[type] != dummy_format_entry))
    panic("format entry function for type %d already registered", type);

  _format_entry_fn[type] = format_entry_fn;
}

// return task+eip of entry <e_nr>
IMPLEMENT
int
Jdb_tbuf_output::thread_eip(int e_nr, L4_uid *tid, Mword *eip)
{
  Tb_entry *e = Jdb_tbuf::lookup(e_nr);

  if (!e)
    return false;

  Thread *t = Thread::lookup(e->tid());
  *tid = t ? t->id() : L4_uid(L4_uid::INVALID);
  *eip = e->eip();

  return true;
}

IMPLEMENT
void
Jdb_tbuf_output::print_entry(int e_nr, char *buf, int maxlen)
{
  Tb_entry *tb = Jdb_tbuf::lookup(e_nr);

  if (tb)
    print_entry(tb, buf, maxlen);
}

IMPLEMENT
void Jdb_tbuf_output::print_entry(Tb_entry *tb, char *buf, int maxlen)
{
  assert(tb->type() < MAX_TBUF);

  Thread *t  = Thread::lookup(tb->tid());
  L4_uid tid = t ? t->id() : L4_uid(L4_uid::INVALID);
  unsigned len = _format_entry_fn[tb->type()](tb, tid, buf, maxlen);

  // terminate string
  buf += maxlen-len;
  while (len--)
    *buf++ = ' ';
  buf[-1] = '\0';
}

