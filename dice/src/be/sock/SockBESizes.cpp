/**
 *    \file    dice/src/be/sock/SockBESizes.cpp
 *  \brief   contains the implementation of the class CSockBESizes
 *
 *    \date    11/28/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */

#include "be/sock/SockBESizes.h"
#include <sys/socket.h>

CSockBESizes::CSockBESizes()
{ }

/** \brief destructor of target class */
CSockBESizes::~CSockBESizes()
{ }

/** \brief calculate the maximum for a specific type
 *  \param nFEType the type to get the max-size for
 *  \return the maximum size in bytes
 */
int CSockBESizes::GetMaxSizeOfType(int /*nFEType*/)
{
	// for sockets, the maximum size is approximately 8KB,
	// so we restrict this to 2KB, assuming we have a max of 4
	// var sized parameters
	// even if we would have more than 4, so what...;)
	return 2048;
}
