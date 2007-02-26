/**
 *  \file    dice/src/be/l4/v4/L4V4BESizes.cpp
 *  \brief   contains the implementation of the class CL4V4BESizes
 *
 *  \date    01/08/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4V4BESizes.h"
#include "TypeSpec-L4V4Types.h"

CL4V4BESizes::CL4V4BESizes()
 : CL4BESizes()
{
}

/** \brief destroys object of this class */
CL4V4BESizes::~CL4V4BESizes()
{
}

/** \brief get the maximum message size in bytes for a short IPC
 *  \return the max size in bytes
 *
 * For generic V4 we return 0, because all IPC loads into MRs.
 */
int CL4V4BESizes::GetMaxShortIPCSize()
{
    return 0;
}

/** \brief gets the size of a type
 *  \param nFEType the type to look up
 *  \param nFESize the size in the front-end
 *  \return the size of the type in bytes
 */
int CL4V4BESizes::GetSizeOfType(int nFEType, int nFESize)
{
    int nSize = 0;
    switch (nFEType) 
    {
    case TYPE_RCV_FLEXPAGE:
    case TYPE_MSGDOPE_SEND:
    case TYPE_MSGDOPE_SIZE:
    case TYPE_MSGTAG:
	nSize = 4;
	break;
    case TYPE_REFSTRING:
	nSize = 8;
	break;
    case TYPE_FLEXPAGE:
	nSize = 8;
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
int CL4V4BESizes::GetMaxSizeOfType(int nFEType)
{
    int nSize = CBESizes::GetMaxSizeOfType(nFEType);
    switch (nFEType)
    {
    case TYPE_MESSAGE:
	nSize = 64 * GetSizeOfType(TYPE_MWORD); /* maximum of 2^19 dwords */
	break;
    default:
        break;
    }
    return nSize;
}

