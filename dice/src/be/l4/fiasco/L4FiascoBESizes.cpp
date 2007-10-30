/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBESizes.cpp
 *  \brief   contains the implementation of the class CL4FiascoBESizes
 *
 *  \date    08/20/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "L4FiascoBESizes.h"
#include "TypeSpec-Type.h"

CL4FiascoBESizes::CL4FiascoBESizes()
{ }

/** \brief destroys object of this class */
CL4FiascoBESizes::~CL4FiascoBESizes()
{ }

/** \brief get the maximum message size in bytes for a short IPC
 *  \return the max size in bytes
 *
 * The size of the opcode is neglected, because its added by the functions
 * themselves (sometimes no opcode is necessary).
 */
int CL4FiascoBESizes::GetMaxShortIPCSize()
{
    return 2 * GetSizeOfType(TYPE_MWORD);
}

/** \brief returns a value for the maximum  size of a specific type
 *  \param nFEType the type to get the max size for
 *  \return the maximum size of an array of that type
 *
 * This function is used to determine a maximum size of an array of a specifc
 * type if the parameter has no maximum size attribute.
 */
int CL4FiascoBESizes::GetMaxSizeOfType(int nFEType)
{
    int nSize = CBESizes::GetMaxSizeOfType(nFEType);
    switch (nFEType)
    {
    case TYPE_CHAR:
    case TYPE_CHAR_ASTERISK:
        nSize = 1024;
        break;
    case TYPE_MESSAGE:
	nSize = (1 << 19) * GetSizeOfType(TYPE_MWORD); /* maximum of 2^19 dwords */
	break;
    default:
        break;
    }
    return nSize;
}

/** \brief try to determine the size of a user defined type based on its name
 *  \param sUserType the name of the type
 *  \return the size of the type or 0 if not found
 *
 * Only test L4.Fiasco specific type names. Leave the rest to the base class.
 */
int CL4FiascoBESizes::GetSizeOfType(std::string sUserType)
{
    if (sUserType == "l4_msgtag_t")
	return GetSizeOfType(TYPE_MSGTAG, 0);
    return CL4BESizes::GetSizeOfType(sUserType);
}
