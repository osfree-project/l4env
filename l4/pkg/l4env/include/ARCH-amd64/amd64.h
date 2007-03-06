/*****************************************************************************/
/**
 * \file   l4env/include/ARCH-amd64/amd64.h
 * \brief  System/Architecture specific definitions
 *
 * \date   03/04/2007
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2007
 * Technische Universität Dresden, Operating Systems Research Group
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
#ifndef _L4_L4ENV_AMD64_H
#define _L4_L4ENV_AMD64_H

/* valid ELF file specs for this architecture */
#define ARCH_ELF_ARCH		EM_AMD64
#define ARCH_ELF_ARCH_DATA	ELFDATA2LSB
#define ARCH_ELF_ARCH_CLASS	ELFCLASSS64


#endif /* _L4_L4ENV_AMD64_H */
