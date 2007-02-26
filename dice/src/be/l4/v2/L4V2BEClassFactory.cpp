/**
 *    \file    dice/src/be/l4/v2/L4V2BEClassFactory.cpp
 *    \brief   contains the implementation of the class CL4V2BEClassFactory
 *
 *    \date    02/07/2002
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

#include "L4V2BEClassFactory.h"
#include "L4V2BECallFunction.h"
#include "L4V2BESndFunction.h"
#include "L4V2BEMarshaller.h"
#include "L4V2BESizes.h"
#include "L4V2BEIPC.h"
#include "L4V2BEReplyFunction.h"

#include "be/BEContext.h"

CL4V2BEClassFactory::CL4V2BEClassFactory(bool bVerbose)
: CL4BEClassFactory(bVerbose)
{
}

CL4V2BEClassFactory::CL4V2BEClassFactory(CL4V2BEClassFactory & src)
: CL4BEClassFactory(src)
{
}

/**    \brief the destructor of this class */
CL4V2BEClassFactory::~CL4V2BEClassFactory()
{

}

/**    \brief creates a new instance of the class CBECallFunction
 *    \return a reference to the new instance
 */
CBECallFunction *CL4V2BEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4BECallFunction\n");
    return new CL4V2BECallFunction();
}

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4V2BEClassFactory::GetNewSizes()
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4V2BESizes\n");
    return new CL4V2BESizes();
}

/** \brief create a new send function class
 *  \return a reference to the new instance
 */
CBESndFunction* CL4V2BEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4V2BESndFunction\n");
    return new CL4V2BESndFunction();
}

/** \brief create a new marshaller class
 *  \return a reference to the new instance
 */
CBEMarshaller* CL4V2BEClassFactory::GetNewMarshaller(CBEContext* pContext)
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4V2BEMarshaller\n");
    return new CL4V2BEMarshaller();
}

/** \brief create a new IPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4V2BEClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4V2BEIPC\n");
    return new CL4V2BEIPC();
}

/** \brief create a new reply function class
 *  \return a reference to the new instance
 */
CBEReplyFunction* CL4V2BEClassFactory::GetNewReplyFunction()
{
    if (m_bVerbose)
        printf("CL4V2BEClassFactory: created class CL4V2BEReplyFunction\n");
    return new CL4V2BEReplyFunction();
}
