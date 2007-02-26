/**
 *	\file	dice/src/be/l4/x0/L4X0BEClassFactory.cpp
 *	\brief	contains the implementation of the class CL4X0BEClassFactory
 *
 *	\date	12/01/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/l4/x0/L4X0BEClassFactory.h"
#include "be/l4/x0/L4X0BECallFunction.h"
#include "be/l4/x0/L4X0BERcvAnyFunction.h"
#include "be/l4/x0/L4X0BEReplyAnyWaitAnyFunction.h"
#include "be/l4/x0/L4X0BEReplyRcvFunction.h"
#include "be/l4/x0/L4X0BEReplyWaitFunction.h"
#include "be/l4/x0/L4X0BESndFunction.h"
#include "be/l4/x0/L4X0BEWaitAnyFunction.h"
#include "be/l4/x0/L4X0BEWaitFunction.h"
#include "be/l4/x0/L4X0BESizes.h"
#include "be/BEContext.h"

IMPLEMENT_DYNAMIC(CL4X0BEClassFactory);

CL4X0BEClassFactory::CL4X0BEClassFactory(bool bVerbose)
: CL4BEClassFactory(bVerbose)
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BEClassFactory, CL4BEClassFactory);
}

CL4X0BEClassFactory::CL4X0BEClassFactory(CL4X0BEClassFactory & src)
: CL4BEClassFactory(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BEClassFactory, CL4BEClassFactory);
}

/**	\brief the destructor of this class */
CL4X0BEClassFactory::~CL4X0BEClassFactory()
{

}

/**	\brief creates a new instance of the class CBECallFunction
 *	\return a reference to the new instance
 */
CBECallFunction *CL4X0BEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4BECallFunction\n");
    return new CL4X0BECallFunction();
}

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4X0BEClassFactory::GetNewSizes()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BESizes\n");
    return new CL4X0BESizes();
}

/**	\brief creates a new instance of the class CBERcvAnyFunction
 *	\return a reference to the new instance
 */
CBERcvAnyFunction * CL4X0BEClassFactory::GetNewRcvAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BERcvAnyFunction\n");
    return new CL4X0BERcvAnyFunction();
}

/**	\brief creates a new instance of the class CBEReplyAnyWaitAnyFunction
 *	\return a reference to the new instance
 */
CBEReplyAnyWaitAnyFunction * CL4X0BEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BEReplyAnyWaitAnyFunction\n");
    return new CL4X0BEReplyAnyWaitAnyFunction();
}

/**	\brief creates a new instance of the class CBEReplyRcvFunction
 *	\return a reference to the new instance
 */
CBEReplyRcvFunction * CL4X0BEClassFactory::GetNewReplyRcvFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BEReplyRcvFunction\n");
    return new CL4X0BEReplyRcvFunction();
}

/**	\brief creates a new instance of the class CBEReplyWaitFunction
 *	\return a reference to the new instance
 */
CBEReplyWaitFunction * CL4X0BEClassFactory::GetNewReplyWaitFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BEReplyWaitFunction\n");
    return new CL4X0BEReplyWaitFunction();
}

/**	\brief creates a new instance of the class CBESndFunction
 *	\return a reference to the new instance
 */
CBESndFunction * CL4X0BEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BESndFunction\n");
    return new CL4X0BESndFunction();
}

/**	\brief creates a new instance of the class CBEWaitAnyFunction
 *	\return a reference to the new instance
 */
CBEWaitAnyFunction * CL4X0BEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BEWaitAnyFunction\n");
    return new CL4X0BEWaitAnyFunction();
}

/**	\brief creates a new instance of the class CBEWaitFunction
 *	\return a reference to the new instance
 */
CBEWaitFunction * CL4X0BEClassFactory::GetNewWaitFunction()
{
    if (m_bVerbose)
        printf("CL4X0BEClassFactory: created class CL4X0BEWaitFunction\n");
    return new CL4X0BEWaitFunction();
}
