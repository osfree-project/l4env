/*!
 * \file   support_x86.cc
 * \brief  Support for the x86 platform
 *
 * \date   2008-01-02
 * \author Adam Lackorznynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include "support.h"

namespace L4
{
  class Uart_x86 : public Uart
  {
  private:
    unsigned long _base;

    inline unsigned long rd(unsigned long reg) const;
    inline void wr(unsigned long reg, unsigned long val) const;

  public:
    Uart_x86(int rx_irq, int tx_irq)
      : Uart(rx_irq, tx_irq), _base(~0UL) {}
    bool startup(unsigned long base);
    void shutdown();
    bool enable_rx_irq(bool enable = true);
    bool enable_tx_irq(bool enable = true);
    bool change_mode(Transfer_mode m, Baud_rate r);
    int get_char(bool blocking = true) const;
    int char_avail() const;
    inline void out_char(char c) const;
    int write(char const *s, unsigned long count) const;
  };
};


#include <strings.h>
#include "base_critical.h"
#include "ARCH-x86/serial.h"
#include <l4/util/cpu.h>
#include <l4/util/port_io.h>

/** VGA console output */
static void
vga_putchar(unsigned char c)
{
  static int ofs = -1, esc, esc_val, attr = 0x07;
  unsigned char *vidbase = (unsigned char*)0xb8000;

  base_critical_enter();

  if (ofs < 0)
    {
      /* Called for the first time - initialize.  */
      ofs = 80*2*24;
      vga_putchar('\n');
    }

  switch (esc)
    {
    case 1:
      if (c == '[')
	{
	  esc++;
	  goto done;
	}
      esc = 0;
      break;

    case 2:
      if (c >= '0' && c <= '9')
	{
	  esc_val = 10*esc_val + c - '0';
	  goto done;
	}
      if (c == 'm')
	{
	  attr = esc_val ? 0x0f : 0x07;
	  goto done;
	}
      esc = 0;
      break;
    }

  switch (c)
    {
    case '\n':
      bcopy(vidbase+80*2, vidbase, 80*2*24);
      bzero(vidbase+80*2*24, 80*2);
      /* fall through... */
    case '\r':
      ofs = 0;
      break;

    case '\t':
      ofs = (ofs + 8) & ~7;
      break;

    case '\033':
      esc = 1;
      esc_val = 0;
      break;

    default:
      /* Wrap if we reach the end of a line.  */
      if (ofs >= 80)
	vga_putchar('\n');

      /* Stuff the character into the video buffer. */
	{
	  volatile unsigned char *p = vidbase + 80*2*24 + ofs*2;
	  p[0] = c;
	  p[1] = attr;
	  ofs++;
	}
      break;
    }

done:
  base_critical_leave();
}

/** Poor man's getchar, only returns raw scan code. We don't need to know
 * _which_ key was pressed, we only want to know _if_ a key was pressed. */
static int
raw_keyboard_getscancode(void)
{
  unsigned status, scan_code;

  base_critical_enter();

  l4util_cpu_pause();

  /* Wait until a scan code is ready and read it. */
  status = l4util_in8(0x64);
  if ((status & 0x01) == 0)
    {
      base_critical_leave();
      return -1;
    }
  scan_code = l4util_in8(0x60);

  /* Drop mouse events */
  if ((status & 0x20) != 0)
    {
      base_critical_leave();
      return -1;
    }

  base_critical_leave();
  return scan_code;
}

namespace L4
{
  bool Uart_x86::startup(unsigned long base)
  // real uart init will be made by startup.cc if told by cmdline
  { return true; }

  void Uart_x86::shutdown() {}
  bool Uart_x86::enable_rx_irq(bool) { return true; }
  bool Uart_x86::enable_tx_irq(bool) { return false; }
  bool Uart_x86::change_mode(Transfer_mode, Baud_rate) { return false; }

  int Uart_x86::get_char(bool blocking) const
  {
    int c;
    do {
      c = com_cons_try_getchar();
      if (c == -1)
        raw_keyboard_getscancode();
      l4util_cpu_pause();
    } while (c == -1 && blocking);

    return c;
  }

  int Uart_x86::char_avail() const
  {
    return com_cons_char_avail();
  }

  void Uart_x86::out_char(char c) const
  {
    vga_putchar(c);      // vga out
    com_cons_putchar(c); // serial out
  }

  int Uart_x86::write(char const *s, unsigned long count) const
  {
    unsigned long c = count;
    while (c)
      {
        if (*s == 10)
          out_char(13);
        out_char(*s++);
        --c;
      }
    return count;
  }
};


void platform_init(void)
{
  // this is just a wrapper around serial.c
  // if you think this could be done better you're right...
  static L4::Uart_x86 _uart(1,1);
  _uart.startup(0);
  set_stdio_uart(&_uart);
}
