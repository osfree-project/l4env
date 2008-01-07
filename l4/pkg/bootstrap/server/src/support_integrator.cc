/*!
 * \file   support_integrator.cc
 * \brief  Support for the integrator platform
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

DEFINE_UART_STORAGE(Uart_pl011);

void platform_init(void)
{
  UART_STARTUP(Uart_pl011, 1, 1, 0x16000000);
}
