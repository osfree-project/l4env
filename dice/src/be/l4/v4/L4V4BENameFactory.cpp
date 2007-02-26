/**
 *    \file    dice/src/be/l4/v4/L4V4BENameFactory.cpp
 *    \brief    contains the implementation of the class CL4V4BENameFactory
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"

CL4V4BENameFactory::CL4V4BENameFactory(bool bVerbose)
 : CL4BENameFactory(bVerbose)
{
}

CL4V4BENameFactory::CL4V4BENameFactory(CL4V4BENameFactory & src)
: CL4BENameFactory(src)
{
}

/** \brief destructs this instance
 */
CL4V4BENameFactory::~CL4V4BENameFactory()
{
}


/**    \brief general access function to generate strings
 *    \param nStringCode specifying the requested code
 *    \param pContext the context of the name generation
 *    \param pParam additional untyped parameter
 *    \return generated name
 *
 * This function multiplexes the request to the functions of this class' implementation.
 */
string CL4V4BENameFactory::GetString(int nStringCode, CBEContext * pContext, void *pParam)
{
    switch (nStringCode)
    {
    case STR_MSGTAG_VARIABLE:
        return GetMsgTagVarName(pContext);
        break;
    default:
        break;
    }
    return CL4BENameFactory::GetString(nStringCode, pContext, pParam);
}

/** \brief returns the variable name for the msgtag return variable of an IPC invocation
 *  \return the name of the variable
 */
string CL4V4BENameFactory::GetMsgTagVarName(CBEContext *pContext)
{
    return string("mr0");
}

/** \brief create specific L4 type names
 *  \param nType the type to create the name for
 *  \param bUnsigned true if the type is unsigned
 *  \param pContext the context of the creation process
 *  \param nSize the size of the type in bytes
 *  \return the L4 type name
 */
string CL4V4BENameFactory::GetL4TypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
        switch (nSize)
        {
        case 1:
            if (bUnsigned)
                sReturn = "L4_Word8_t";
            break;
        case 2:
            if (bUnsigned)
                sReturn = "L4_Word16_t";
            break;
        case 4:
            if (bUnsigned)
                sReturn = "L4_Word32_t";
            break;
        case 8:
            if (bUnsigned)
                sReturn = "L4_Word64_t";
            break;
        }
        break;
    case TYPE_CHAR:
        if (bUnsigned)
            sReturn = "L4_Word8_t";
        break;
    case TYPE_BYTE:
        sReturn = "L4_Word8_t";
        break;
    }

    if (m_bVerbose)
        printf("CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
               __FUNCTION__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param pContext the context of the name generation
 *  \param nSize the size of the type
 */
string CL4V4BENameFactory::GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize)
{
    string sReturn;
    if (pContext->IsOptionSet(PROGRAM_USE_L4TYPES))
        sReturn = GetL4TypeName(nType, bUnsigned, pContext, nSize);
    if (!sReturn.empty())
        return sReturn;
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
            sReturn = "L4_Word32_t";
        break;
    default:
        sReturn = CBENameFactory::GetTypeName(nType, bUnsigned, pContext, nSize);
        break;
    }
    if (m_bVerbose)
        printf("CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
               __FUNCTION__, sReturn.c_str(), nType);
    return sReturn;
}
