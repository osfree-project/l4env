/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aBEClassFactory.cpp
 *    \brief   contains the implementation of the class CL4X0aBEClassFactory
 *
 *    \date    18/02/2003
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

#include "be/l4/x0adapt/L4X0aBEClassFactory.h"
#include "be/l4/x0adapt/L4X0aBECallFunction.h"
#include "be/l4/x0adapt/L4X0aBESndFunction.h"
#include "be/l4/x0adapt/L4X0aBEWaitAnyFunction.h"
#include "be/l4/x0adapt/L4X0aBEWaitFunction.h"
#include "be/l4/x0adapt/L4X0aBEReplyFunction.h"
#include "be/l4/x0adapt/L4X0aBESizes.h"
#include "be/l4/x0adapt/L4X0aIPC.h"
#include "be/BEContext.h"


CL4X0aBEClassFactory::CL4X0aBEClassFactory(bool bVerbose)
: CL4BEClassFactory(bVerbose)
{
}

CL4X0aBEClassFactory::CL4X0aBEClassFactory(CL4X0aBEClassFactory & src)
: CL4BEClassFactory(src)
{
}

/**    \brief the destructor of this class */
CL4X0aBEClassFactory::~CL4X0aBEClassFactory()
{
}

/**    \brief creates a new instance of the class CBECallFunction
 *    \return a reference to the new instance
 */
CBECallFunction *CL4X0aBEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4BECallFunction\n");
    return new CL4X0aBECallFunction();
}

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4X0aBEClassFactory::GetNewSizes()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBESizes\n");
    return new CL4X0aBESizes();
}

/**    \brief creates a new instance of the class CBERcvAnyFunction
 *    \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4X0aBEClassFactory::GetNewRcvAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBERcvAnyFunction\n");
    return new CL4X0aBEWaitAnyFunction(false, false);
}

/**    \brief creates a new instance of the class CBEReplyAnyWaitAnyFunction
 *    \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4X0aBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEReplyAnyWaitAnyFunction\n");
    return new CL4X0aBEWaitAnyFunction(true, true);
}

/**    \brief creates a new instance of the class CBESndFunction
 *    \return a reference to the new instance
 */
CBESndFunction * CL4X0aBEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBESndFunction\n");
    return new CL4X0aBESndFunction();
}

/**    \brief creates a new instance of the class CBEWaitAnyFunction
 *    \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4X0aBEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEWaitAnyFunction\n");
    return new CL4X0aBEWaitAnyFunction(true, false);
}

/**    \brief creates a new instance of the class CBEWaitFunction
 *    \return a reference to the new instance
 */
CBEWaitFunction * CL4X0aBEClassFactory::GetNewWaitFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEWaitFunction\n");
    return new CL4X0aBEWaitFunction(true);
}

/** \brief creates a new IPC object
 *  \return a reference to the new instance
 */
CBECommunication* CL4X0aBEClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aIPC\n");
    return new CL4X0aIPC();
}

/** \brief creates a new reply function object
 *  \return a reference to the new instance
 */
CBEReplyFunction* CL4X0aBEClassFactory::GetNewReplyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEReplyFunction\n");
    return new CL4X0aBEReplyFunction();
}

/** \brief creates a new reply function object
 *  \return a reference to the new instance
 */
CBEWaitFunction* CL4X0aBEClassFactory::GetNewRcvFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEWaitFunction\n");
    return new CL4X0aBEWaitFunction(false);
}
