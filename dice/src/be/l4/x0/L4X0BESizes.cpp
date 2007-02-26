/**
 *    \file    dice/src/be/l4/x0/L4X0BESizes.cpp
 *  \brief   contains the implementation of the class CL4X0BESizes
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "be/l4/x0/L4X0BESizes.h"
#include "be/l4/TypeSpec-L4Types.h"
#include <cassert>

CL4X0BESizes::CL4X0BESizes()
 : CL4BESizes()
{
}

/** destroys the object */
CL4X0BESizes::~CL4X0BESizes()
{
}

/** \brief determines maximum number
 *  \return the number of bytes until which the short IPC is allowed
 *
 * method, which shall determine the maximum number of bytes of
 * a message for a short IPC
*/
int CL4X0BESizes::GetMaxShortIPCSize()
{
    return 12;
}

/** \brief gets the size of a type
 *  \param nFEType the type to look up
 *  \param nFESize the size in the front-end
 *  \return the size of the type in bytes
 */
int CL4X0BESizes::GetSizeOfType(int nFEType, int nFESize)
{
    int nSize = 0;
    switch (nFEType)
    {
    case TYPE_RCV_FLEXPAGE:
    case TYPE_MSGDOPE_SEND:
    case TYPE_MSGDOPE_SIZE:
        nSize = 4;
        break;
    case TYPE_REFSTRING:
        nSize = 16;
        break;
    default:
        nSize = CL4BESizes::GetSizeOfType(nFEType, nFESize);
        break;
    }
    return nSize;
}

/** \brief returns a value for the maximum  size of a specific type
 *  \param nFEType the type to get the max size for
 *  \return the maximum size of an array of that type
 */
int CL4X0BESizes::GetMaxSizeOfType(int nFEType)
{
    int nSize = CBESizes::GetMaxSizeOfType(nFEType);
    switch (nFEType)
    {
    case TYPE_MESSAGE:
	nSize = (1 << 19) * GetSizeOfType(TYPE_MWORD); /* maximum of 2^19 dwords */
	break;
    default:
        break;
    }
    return nSize;
}
