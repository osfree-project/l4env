/**
 *  \file    dice/src/be/l4/fiasco/amd64/L4FiascoAMD64NameFactory.cpp
 *  \brief   contains the implementation of the class CL4FiascoAMD64BENameFactory
 *
 *  \date    08/24/2007
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

#include "L4FiascoAMD64NameFactory.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"

CL4FiascoAMD64BENameFactory::CL4FiascoAMD64BENameFactory()
: CL4FiascoBENameFactory()
{ }

/** \brief the destructor of this class */
CL4FiascoAMD64BENameFactory::~CL4FiascoAMD64BENameFactory()
{ }

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned

 *  \param nSize the size of the type
 */
std::string CL4FiascoAMD64BENameFactory::GetTypeName(int nType, bool bUnsigned, int nSize)
{
    std::string sReturn;
    switch (nType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
	if (bUnsigned)
	    sReturn = "unsigned ";
	switch (nSize)
	{
	case 1:
	    sReturn = "unsigned char";
	    break;
	case 2:
	    sReturn += "short";
	    break;
	case 4:
	    sReturn += "int";
	    break;
	case 8:
	    sReturn += "long";
	    break;
	}
	break;
    default:
        sReturn = CL4FiascoBENameFactory::GetTypeName(nType, bUnsigned, nSize);
        break;
    }
    CCompiler::Verbose("CL4FiascoAMD64BENameFactory::%s: Generated type name \"%s\" for type code %d\n",
	__func__, sReturn.c_str(), nType);
    return sReturn;
}

