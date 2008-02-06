/*!
 * \file   support_pxa.cc
 * \brief  Support for the PXA platform
 *
 * \date   2008-01-04
 * \author Adam Lackorznynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include "support.h"
#include <l4/drivers/uart_pxa.h>

void platform_init(void)
{
  static L4::Uart_pxa _uart(1,1);
  _uart.startup(0x40100000);
  set_stdio_uart(&_uart);
}
