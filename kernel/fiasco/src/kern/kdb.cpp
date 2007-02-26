INTERFACE: 

struct x86_gate;

class Console;

class kdb
{
private:
  kdb();			// default constructors are undefined
  kdb(const kdb&);

  static bool _connected;
  static int com_port;
  
  friend class _foo;		// avoids warning "no public cons/no friends"
};

IMPLEMENTATION:

#include <flux/gdb.h>
#include <flux/gdb_serial.h>

#include <flux/x86/base_trap.h>

#include "io.h"
#include "boot_info.h"
#include "filter_console.h"
#include "kernel_console.h"
#include "kernel_uart.h"
#include "kmem.h"
#include "idt.h"

#include <cstring>

//#include "legacy_console.h"
#include "irq.h"
#include "pic.h"
#include "static_init.h"


class Kdb_cons 
  : public Console
{
public:
  int write( char const *str, size_t len );
  int getchar( bool blocking = true );
  int char_avail() const;
};

IMPLEMENT
int Kdb_cons::write( char const *str, size_t len )
{
  for( size_t i=0; i<len; ++i )
    gdb_serial_putchar(str[i]);
  return len;
}

IMPLEMENT
int Kdb_cons::getchar( bool )
{
  return -1;
}

IMPLEMENT 
int Kdb_cons::char_avail() const
{
  return 0;
}

PUBLIC static 
Console *const kdb::kdb_console()
{
  static Kdb_cons k;
  static Filter_console f(&k);
  return &f;
}

PUBLIC static 
Console *const kdb::com_console()
{
  static Filter_console f(Kernel_uart::uart());
  return &f;
}


bool kdb::_connected = false;
//int kdb::qcom_port;

extern "C" void gdb_pc_com_intr();
extern unsigned gdb_pc_com_irq;

PUBLIC static
inline bool kdb::connected()
{
  return _connected;
}


PUBLIC static
void kdb::disconnect()
{
  // XXX should tell the remote GDB that we disconnect; for now, we
  // just unset the connected flag so that stdio will no longer do
  // output via the remote gdb

  _connected = false;
  Kconsole::console()->unregister_console(kdb_console());

  char const *cmdline = Boot_info::cmdline();
  if(! strstr(cmdline, " -noserial"))
    {
      Kconsole::console()->register_console(com_console());
    }
  //  console::use_gdb = false;
}

PUBLIC static
void kdb::reconnect()
{
  _connected = true;
  Kconsole::console()->register_console(kdb_console());
}


STATIC_INITIALIZE_P(kdb, KDB_INIT_PRIO);


static void uart_putchar( int c )
{
  char ch = c;
  Kernel_uart::uart()->write(&ch,1);
}

static int uart_getchar()
{
  return Kernel_uart::uart()->getchar(true); // blocking getchar
}



PUBLIC static
void kdb::init()
{
  char const *cmdline = Boot_info::cmdline();
  
  if(strstr(cmdline, " -nokdb") || strstr(cmdline, " -noserial"))
    {
      disconnect();
      return;
    }
    
  Uart *com = Kernel_uart::uart();

  // initialize GDB-via-serial

  // code copied from oskit/libkern/x86/pc/gdb_pc_com.c
  //com_port = console::serial_com_port;
  int com_irq = com->irq();//com_port & 1 ? 4 : 3;

  /* Tell the generic serial GDB code
     how to send and receive characters.  */
  gdb_serial_recv = uart_getchar;
  gdb_serial_send = uart_putchar;
  
  /* Tell the GDB proxy trap handler to use the serial stub.  */
  gdb_signal = gdb_serial_signal;
  
  /* Hook in the GDB proxy trap handler as the main trap handler.  */
  base_trap_handler = gdb_trap;

  /*
   * Initialize the serial port.
   */

  reconnect();
  //com_port_init();

  /* Hook the COM port's hardware interrupt.
     The com_cons itself uses only polling for communication;
     the interrupt is only used to allow the remote debugger
     to stop us at any point, e.g. when the user presses CTRL-C.  */
  // fill_irq_gate(com_irq, (unsigned)gdb_pc_com_intr, KERNEL_CS, ACC_PL_K);
   if (! strstr(cmdline, " -I-") 
      && !strstr(cmdline, " -irqcom"))
     {
       Idt::set_vector (0x20 + com_irq, (unsigned) gdb_pc_com_intr, false);
       gdb_pc_com_irq = com_irq;
       
       /* Enable the COM port interrupt.  */
       com->enable_rcv_irq();
       //       com_cons_enable_receive_interrupt();
       Pic::enable(com_irq);
       
       //       irq_enable(com_irq);
     }
}

#if 0
PUBLIC static
void 
kdb::com_port_init()
{
  if (_connected)
    com_cons_init(com_port);
}
#endif
// The following routines are for use with the kernel GDB, ala
// call kdb::inb(port)

PRIVATE static
unsigned char 
kdb::in(unsigned port)
{
  return Io::in8_p(port);
}

PRIVATE static
void
kdb::out(unsigned port, unsigned char val)
{
  Io::out8_p(val, port);
}
