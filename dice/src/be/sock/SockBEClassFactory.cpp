/**
 *    \file    dice/src/be/sock/SockBEClassFactory.cpp
 *    \brief   contains the implementation of the class CBEClassFactory
 *
 *    \date    01/10/2002
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

#include "SockBEClassFactory.h"

#include "SockBECallFunction.h"
#include "SockBEWaitAnyFunction.h"
#include "SockBEMarshalFunction.h"
#include "SockBESrvLoopFunction.h"
#include "SockBEUnmarshalFunction.h"
#include "SockBESizes.h"
#include "BESocket.h"
#include "SockBEDispatchFunction.h"


CSockBEClassFactory::CSockBEClassFactory(bool bVerbose)
: CBEClassFactory(bVerbose)
{
}

CSockBEClassFactory::CSockBEClassFactory(CSockBEClassFactory & src)
: CBEClassFactory(src)
{
}

/**    \brief the destructor of this class */
CSockBEClassFactory::~CSockBEClassFactory()
{

}

/** \brief creates a new instance of the class CBECallFunction
 *  \return a reference to the new object
 */
CBECallFunction * CSockBEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBECallFunction\n");
    return new CSockBECallFunction();
}

/** \brief creates a new instance of the class CBESizes
 *  \return a reference to the new object
 */
CBESizes * CSockBEClassFactory::GetNewSizes()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBESizes\n");
    return new CSockBESizes();
}

/** \brief creates a new instance of the class CBEWaitAnyFunction
 *  \return a reference to the new object
 */
CBEWaitAnyFunction * CSockBEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBEWaitAnyFunction\n");
    return new CSockBEWaitAnyFunction(true, false);
}

/** \brief creates a new instance of the class CBESrvLoopFunction
 *  \return a reference to the new object
 */
CBESrvLoopFunction * CSockBEClassFactory::GetNewSrvLoopFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBESrvLoopFunction\n");
    return new CSockBESrvLoopFunction();
}

/** \brief creates a new instance of the class CBEUnmarshalFunction
 *  \return a reference to the new object
 */
CBEUnmarshalFunction * CSockBEClassFactory::GetNewUnmarshalFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBEUnmarshalFunction\n");
    return new CSockBEUnmarshalFunction();
}

/** \brief creates a new instance of the class CBEMarshalFunction
 *  \return a reference to the new object
 */
CBEMarshalFunction * CSockBEClassFactory::GetNewMarshalFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBEMarshalFunction\n");
    return new CSockBEMarshalFunction();
}

/** \brief creates a new instance of the class CBESocket
 *  \return a reference to the new object
 */
CBECommunication * CSockBEClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CBESocket\n");
    return new CBESocket();
}

/** \brief creates a new instance of the class CBEReplyAnyWaitAnyFunction
 *  \return a reference to the new object
 */
CBEWaitAnyFunction* CSockBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBEReplyAnyWaitAnyFunction\n");
    return new CSockBEWaitAnyFunction(true, true);
}


/** \brief creates a new instance of the class CBEDispatchFunction
 *  \return a reference to the new object
 */
CBEDispatchFunction* CSockBEClassFactory::GetNewDispatchFunction()
{
    if (m_bVerbose)
        printf("CSockBEClassFactory: created class CSockBEDispatchFunction\n");
    return new CSockBEDispatchFunction();
}
