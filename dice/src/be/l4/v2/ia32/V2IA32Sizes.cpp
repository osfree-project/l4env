/**
 *    \file    dice/src/be/l4/v2/ia32/V2IA32Sizes.cpp
 *    \brief   contains the implementation of the class CL4V2IA32BESizes
 *
 *    \date    04/18/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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

#include "V2IA32Sizes.h"
#include "TypeSpec-Type.h"

CL4V2IA32BESizes::CL4V2IA32BESizes()
{ }

/** \brief destroys object of this class */
CL4V2IA32BESizes::~CL4V2IA32BESizes()
{ }

/** \brief gets the size of a type
 *  \param nFEType the type to look up
 *  \param nFESize the size in the front-end
 *  \return the size of the type in bytes
 */
int CL4V2IA32BESizes::GetSizeOfType(int nFEType, int nFESize)
{
    int nSize = 0;
    switch (nFEType)
    {
    case TYPE_MWORD:
	nSize = 4;
	break;
    default:
	nSize = CL4V2BESizes::GetSizeOfType(nFEType, nFESize);
    }
    return nSize;
}

