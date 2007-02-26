INTERFACE:

#include "initcalls.h"

class Console;

class Kdb
{
private:
  Kdb();			// default constructors are undefined
  Kdb(const Kdb&);

  friend class _foo;		// avoids warning "no public cons/no friends"
};

IMPLEMENTATION:

#include <cstdio>
#include <flux/gdb.h>
#include <flux/gdb_serial.h>

#include "cmdline.h"
#include "io.h"
#include "filter_console.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "idt.h"
#include "trap_state.h"

#include <cstring>

#include "irq.h"
#include "pic.h"
#include "static_init.h"


class Kdb_cons : public Console
{
};

PUBLIC
int
Kdb_cons::write( char const *str, size_t len )
{
  for (size_t i=0; i<len; ++i)
    gdb_serial_putchar(str[i]);
  return len;
}

PUBLIC
int
Kdb_cons::getchar (bool)
{
  return -1;
}

PUBLIC 
int
Kdb_cons::char_avail() const
{
  return 0;
}

PUBLIC
Mword
Kdb_cons::get_attributes() const
{
  return DEBUG | OUT;
}


PUBLIC static 
Console *const
Kdb::kdb_console()
{
  static Kdb_cons k;
  static Filter_console f(&k);
  return &f;
}

PUBLIC static 
Console *const
Kdb::com_console()
{
  static Filter_console f(Kernel_uart::uart());
  return &f;
}

extern "C" void gdb_pc_com_intr();
extern unsigned gdb_pc_com_irq;

static
void
uart_putchar( int c )
{
  char ch = c;
  Kernel_uart::uart()->write(&ch,1);
}

static
int
uart_getchar()
{
  return Kernel_uart::uart()->getchar(true); // blocking getchar
}


STATIC_INITIALIZE_P(Kdb, GDB_INIT_PRIO);

PUBLIC static FIASCO_INIT
void
Kdb::init()
{
  if (strstr(Cmdline::cmdline(), " -noserial"))
    return;

  Kconsole::console()->register_console(com_console());
}


STATIC_INITIALIZER(kdb_init_kdb_cons);

static
void
kdb_init_kdb_cons()
{
  Kdb::init_kdb_cons();
}

PUBLIC static
void
Kdb::init_kdb_cons()
{
  char const *cmdline = Cmdline::cmdline();
  
  if (strstr(cmdline, " -nokdb") || strstr(cmdline, " -noserial") ||
      Kernel_uart::uart()->failed())
    return;

  // Tell the generic serial GDB code how to send and receive characters.
  gdb_serial_recv = uart_getchar;
  gdb_serial_send = uart_putchar;

  // Tell the GDB proxy trap handler to use the serial stub.
  gdb_signal = gdb_serial_signal;

  // Hook in the GDB proxy trap handler as the main trap handler.
  Trap_state::base_handler = (int(*)(Trap_state*))gdb_trap;

  // Disable serial console
  Kconsole::console()->unregister_console(com_console());

  // Enable kdb console
  Kconsole::console()->register_console(kdb_console());

  Uart *com = Kernel_uart::uart();

  int com_irq = com->irq();

  /* Hook the COM port's hardware interrupt.
     The com_cons itself uses only polling for communication;
     the interrupt is only used to allow the remote debugger
     to stop us at any point, e.g. when the user presses CTRL-C.  */
  if (! strstr(cmdline, " -I-") && !strstr(cmdline, " -irqcom"))
    {
      Idt::set_entry (0x20 + com_irq, (unsigned long) gdb_pc_com_intr, false);
      gdb_pc_com_irq = com_irq;

      com->enable_rcv_irq();
      Pic::enable(com_irq);
    }
}

PRIVATE static
unsigned char 
Kdb::in(unsigned port)
{
  return Io::in8_p(port);
}

PRIVATE static
void
Kdb::out(unsigned port, unsigned char val)
{
  Io::out8_p(val, port);
}
