/**
 *	\file	dice/src/be/l4/x0adapt/L4X0aBEClassFactory.cpp
 *	\brief	contains the implementation of the class CL4X0aBEClassFactory
 *
 *	\date	18/02/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "be/l4/x0adapt/L4X0aBERcvAnyFunction.h"
#include "be/l4/x0adapt/L4X0aBEReplyAnyWaitAnyFunction.h"
#include "be/l4/x0adapt/L4X0aBEReplyRcvFunction.h"
#include "be/l4/x0adapt/L4X0aBEReplyWaitFunction.h"
#include "be/l4/x0adapt/L4X0aBESndFunction.h"
#include "be/l4/x0adapt/L4X0aBEWaitAnyFunction.h"
#include "be/l4/x0adapt/L4X0aBEWaitFunction.h"
#include "be/l4/x0adapt/L4X0aBESizes.h"
#include "be/BEContext.h"

IMPLEMENT_DYNAMIC(CL4X0aBEClassFactory);

CL4X0aBEClassFactory::CL4X0aBEClassFactory(bool bVerbose)
: CL4BEClassFactory(bVerbose)
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEClassFactory, CL4BEClassFactory);
}

CL4X0aBEClassFactory::CL4X0aBEClassFactory(CL4X0aBEClassFactory & src)
: CL4BEClassFactory(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEClassFactory, CL4BEClassFactory);
}

/**	\brief the destructor of this class */
CL4X0aBEClassFactory::~CL4X0aBEClassFactory()
{

}

/**	\brief creates a new instance of the class CBECallFunction
 *	\return a reference to the new instance
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

/**	\brief creates a new instance of the class CBERcvAnyFunction
 *	\return a reference to the new instance
 */
CBERcvAnyFunction * CL4X0aBEClassFactory::GetNewRcvAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBERcvAnyFunction\n");
    return new CL4X0aBERcvAnyFunction();
}

/**	\brief creates a new instance of the class CBEReplyAnyWaitAnyFunction
 *	\return a reference to the new instance
 */
CBEReplyAnyWaitAnyFunction * CL4X0aBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEReplyAnyWaitAnyFunction\n");
    return new CL4X0aBEReplyAnyWaitAnyFunction();
}

/**	\brief creates a new instance of the class CBEReplyRcvFunction
 *	\return a reference to the new instance
 */
CBEReplyRcvFunction * CL4X0aBEClassFactory::GetNewReplyRcvFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEReplyRcvFunction\n");
    return new CL4X0aBEReplyRcvFunction();
}

/**	\brief creates a new instance of the class CBEReplyWaitFunction
 *	\return a reference to the new instance
 */
CBEReplyWaitFunction * CL4X0aBEClassFactory::GetNewReplyWaitFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEReplyWaitFunction\n");
    return new CL4X0aBEReplyWaitFunction();
}

/**	\brief creates a new instance of the class CBESndFunction
 *	\return a reference to the new instance
 */
CBESndFunction * CL4X0aBEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBESndFunction\n");
    return new CL4X0aBESndFunction();
}

/**	\brief creates a new instance of the class CBEWaitAnyFunction
 *	\return a reference to the new instance
 */
CBEWaitAnyFunction * CL4X0aBEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEWaitAnyFunction\n");
    return new CL4X0aBEWaitAnyFunction();
}

/**	\brief creates a new instance of the class CBEWaitFunction
 *	\return a reference to the new instance
 */
CBEWaitFunction * CL4X0aBEClassFactory::GetNewWaitFunction()
{
    if (m_bVerbose)
        printf("CL4X0aBEClassFactory: created class CL4X0aBEWaitFunction\n");
    return new CL4X0aBEWaitFunction();
}
