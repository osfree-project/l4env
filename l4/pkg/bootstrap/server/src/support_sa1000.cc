/*!
 * \file   support_sa1000.cc
 * \brief  Support for SA1000 platform
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

#include <l4/drivers/uart_sa1000.h>

DEFINE_UART_STORAGE(Uart_sa1000);

void platform_init(void)
{
  UART_STARTUP(Uart_sa1000, 1, 1, 0x80010000);
  //UART_STARTUP(Uart_sa1000, 1, 1, 0x80050000);
}
