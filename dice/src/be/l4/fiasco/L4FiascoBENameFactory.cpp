/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBENameFactory.cpp
 *  \brief   contains the implementation of the class CL4FiascoBENameFactory
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

#include "L4FiascoBENameFactory.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"

CL4FiascoBENameFactory::CL4FiascoBENameFactory()
 : CL4BENameFactory()
{ }

/** \brief destructs this instance
 */
CL4FiascoBENameFactory::~CL4FiascoBENameFactory()
{ }

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param nSize the size of the type
 */
string
CL4FiascoBENameFactory::GetTypeName(int nType,
    bool bUnsigned,
    int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_MSGTAG:
        sReturn = "l4_msgtag_t";
        break;
    default:
        break;
    }
    if (sReturn.empty())
        sReturn = CL4BENameFactory::GetTypeName(nType, bUnsigned, nSize);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4FiascoBENameFactory::%s Generated type name \"%s\" for type code %d\n",
	__func__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief returns the variable name for the msgtag return variable of an IPC invocation
 *  \return the name of the variable
 *
 * The generic L4 name factory has no idea what a message tag variable is.
 */
string
CL4FiascoBENameFactory::GetMsgTagVariable()
{
    return string("_dice_tag");
}
