/*!
 * \file   support_rv.cc
 * \brief  Support for the rv platform
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

#include <l4/drivers/uart_pl011.h>

void platform_init(void)
{
  static L4::Uart_pl011 _uart(36,36);
  _uart.startup(0x10009000);
  set_stdio_uart(&_uart);
}
