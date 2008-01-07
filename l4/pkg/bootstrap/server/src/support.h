/*!
 * \file   support.h
 * \brief  Support header file
 *
 * \date   2008-01-02
 * \author Adam Lackorznynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __BOOTSTRAP__SUPPORT_H__
#define __BOOTSTRAP__SUPPORT_H__

#include <l4/drivers/uart_base.h>

#define DEFINE_UART_STORAGE(c)            \
    static union __uart_storage_type {    \
      char u[sizeof(L4::c)];              \
    } __uart_storage_container;           \
                                          \
    L4::Uart *uart()                      \
    { return reinterpret_cast<L4::Uart*>(__uart_storage_container.u); }

#define UART_STARTUP(c, rx_irq, tx_irq, baseaddr)              \
    do {                                                       \
      new (__uart_storage_container.u) L4::c(rx_irq, tx_irq);  \
      uart()->startup(baseaddr);                               \
    } while (0)

L4::Uart *uart();

void platform_init(void);

#endif /* __BOOTSTRAP__SUPPORT_H__ */
