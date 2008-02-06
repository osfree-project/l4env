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

L4::Uart *uart();
void set_stdio_uart(L4::Uart *uart);

void platform_init(void);

#endif /* __BOOTSTRAP__SUPPORT_H__ */
