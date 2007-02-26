/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/lib-pci/include/glue.h
 *
 * \brief	L4Env l4io PCIlib Glue Code Macros
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

#ifndef _LIBPCI_GLUE_H
#define _LIBPCI_GLUE_H

#define printk		printf

/* \krishna now in linux/init.h:51
 *
 * #define __initcall(x)	(x) *
 * etc. */

/* \krishna now in asm/page.h:81
 *
 * #define __PAGE_OFFSET	(0x0) */

/* \krishna now in local Makefiles
 *
 * emulate 'lcall': 
 *	__asm__("lcall (%%edi); cld"
 *   ->	__asm__("push %%cs; call *(%%edi); cld" */

#endif /* !_LIBPCI_GLUE_H */
