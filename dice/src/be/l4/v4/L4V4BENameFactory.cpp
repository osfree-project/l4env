/**
 *    \file    dice/src/be/l4/v4/L4V4BENameFactory.cpp
 *  \brief   contains the implementation of the class CL4V4BENameFactory
 *
 *    \date    01/08/2004
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

#include "L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/l4/TypeSpec-L4Types.h"
#include "Compiler.h"
#include <iostream>

CL4V4BENameFactory::CL4V4BENameFactory()
 : CL4BENameFactory()
{
}

/** \brief destructs this instance
 */
CL4V4BENameFactory::~CL4V4BENameFactory()
{
}


/** \brief general access function to generate strings
 *  \param nStringCode specifying the requested code
 *  \param pParam additional untyped parameter
 *  \return generated name
 *
 * This function multiplexes the request to the functions of this class' implementation.
 */
string CL4V4BENameFactory::GetString(int nStringCode, void *pParam)
{
    switch (nStringCode)
    {
    case STR_INIT_RCVSTR_VARIABLE:
	return GetInitRcvstrVariable();
	break;
    default:
        break;
    }
    return CL4BENameFactory::GetString(nStringCode, pParam);
}

/** \brief returns the variable name for the msgtag return variable of an IPC invocation
 *  \return the name of the variable
 */
string CL4V4BENameFactory::GetMsgTagVariable()
{
    return string("mr0");
}

/** \brief return the variable name for a temporary string length variable
 *  \return name of the variable
 */
string CL4V4BENameFactory::GetInitRcvstrVariable()
{
    return string("_dice_str_len");
}

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param nSize the size of the type
 */
string
CL4V4BENameFactory::GetTypeName(int nType,
    bool bUnsigned,
    int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_FLEXPAGE:
        sReturn = "L4_Fpage_t";
        break;
    case TYPE_RCV_FLEXPAGE:
        sReturn = "L4_Fpage_t";
        break;
    case TYPE_MWORD:
        if (bUnsigned)
            sReturn = "L4_Word_t";
        break;
    case TYPE_MSGTAG:
        sReturn = "L4_MsgTag_t";
        break;
    case TYPE_REFSTRING:
        sReturn = "L4_StringItem_t";
        break;
    default:
        break;
    }
    if (sReturn.empty())
        sReturn = CBENameFactory::GetTypeName(nType, bUnsigned, nSize);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
	__func__, sReturn.c_str(), nType);
    return sReturn;
}
