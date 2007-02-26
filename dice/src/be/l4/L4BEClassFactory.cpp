/**
 *    \file    dice/src/be/l4/L4BEClassFactory.cpp
 *    \brief   contains the implementation of the class CL4BEClassFactory
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

#include "L4BEClassFactory.h"

#include "L4BECallFunction.h"
#include "L4BESrvLoopFunction.h"
#include "L4BEUnmarshalFunction.h"
#include "L4BEWaitAnyFunction.h"
#include "L4BEReplyFunction.h"
#include "L4BESndFunction.h"
#include "L4BEWaitFunction.h"
#include "L4BEMsgBufferType.h"
#include "L4BEHeaderFile.h"
#include "L4BETestServerFunction.h"
#include "L4BETestMainFunction.h"
#include "L4BEMarshaller.h"
#include "L4BETestFunction.h"
#include "L4BEMarshalFunction.h"
#include "L4BETypedDeclarator.h"
#include "L4BEClass.h"
#include "L4BEIPC.h"
#include "L4BEDispatchFunction.h"
#include "L4BESwitchCase.h"

#include "be/BEContext.h"

CL4BEClassFactory::CL4BEClassFactory(bool bVerbose):CBEClassFactory(bVerbose)
{
}

CL4BEClassFactory::CL4BEClassFactory(CL4BEClassFactory & src):CBEClassFactory(src)
{
}

/**    \brief the destructor of this class */
CL4BEClassFactory::~CL4BEClassFactory()
{

}

/**    \brief creates a new instance of the class CBECallFunction
 *    \return a reference to the new instance
 */
CBECallFunction *CL4BEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BECallFunction\n");
    return new CL4BECallFunction();
}

/**    \brief creates a new instance of the class CBESrvLoopFunction
 *    \return a reference to the new instance
 */
CBESrvLoopFunction *CL4BEClassFactory::GetNewSrvLoopFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BESrvLoopFunction\n");
    return new CL4BESrvLoopFunction();
}

/**    \brief creates a new instance of the class CBEMsgBufferType
 *    \param bInterface true if msgbuf for interface, otherwise msgbuf for function
 *  \return a reference to the new instance
 */
CBEMsgBufferType *CL4BEClassFactory::GetNewMessageBufferType(bool bInterface)
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEMsgBufferType\n");
    return new CL4BEMsgBufferType();
}

/**    \brief creates a new instance of the class CBEUnmarshalFunction
 *    \return a reference to the new instance
 */
CBEUnmarshalFunction *CL4BEClassFactory::GetNewUnmarshalFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEUnmarshalFunction\n");
    return new CL4BEUnmarshalFunction();
}

/**    \brief creates a new instance of the class CBEWaitAnyFunction
 *    \return a reference to the new instance
 */
CBEWaitAnyFunction *CL4BEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEWaitAnyFunction\n");
    return new CL4BEWaitAnyFunction(true, false);
}

/**    \brief creates a new instance of the class CBEHeaderFile
 *    \return a reference to the new instance
 */
CBEHeaderFile *CL4BEClassFactory::GetNewHeaderFile()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEHeaderFile\n");
    return new CL4BEHeaderFile();
}

/**    \brief creates a new instance of the class CBETestServerFunction
 *    \return a reference to the new instance
 */
CBETestServerFunction *CL4BEClassFactory::GetNewTestServerFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BETestServerFunction\n");
    return new CL4BETestServerFunction();
}

/**    \brief creates a new instance of the class CBETestMainFunction
 *    \return a reference to the new instance
 */
CBETestMainFunction *CL4BEClassFactory::GetNewTestMainFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BETestMainFunction\n");
    return new CL4BETestMainFunction();
}

/** \brief creates a new instance of the class CBEMarshaller
 *  \param pContext used to determine optimization level
 *  \return a reference to the new instance
 */
CBEMarshaller * CL4BEClassFactory::GetNewMarshaller(CBEContext * pContext)
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEMarshaller\n");
    return new CL4BEMarshaller();
}

/** \brief creates a new instance of a test-fuinction class
 *  \return a reference to the new instance
 */
CBETestFunction * CL4BEClassFactory::GetNewTestFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BETestFunction\n");
    return new CL4BETestFunction();
}

/** \brief creates a new send function
 *  \return a reference to the new function
 */
CBESndFunction * CL4BEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BESndFunction\n");
    return new CL4BESndFunction();
}

/** \brief creates a new receive function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4BEClassFactory::GetNewRcvFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEWaitFunction\n");
    return new CL4BEWaitFunction(false);
}

/** \brief creates a new wait function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4BEClassFactory::GetNewWaitFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEWaitFunction\n");
    return new CL4BEWaitFunction(true);
}

/** \brief creates a new reply-and-wait function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4BEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEReplyAnyWaitAnyFunction\n");
    return new CL4BEWaitAnyFunction(true, true);
}

/** \brief creates a new rcv-any function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4BEClassFactory::GetNewRcvAnyFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEWaitAnyFunction\n");
    return new CL4BEWaitAnyFunction(false, false);
}

/** \brief creates a new typed declarator
 *  \return a reference to a new typed declarator
 */
CBETypedDeclarator * CL4BEClassFactory::GetNewTypedDeclarator()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BETypedDeclarator\n");
    return new CL4BETypedDeclarator();
}

/** \brief create a new class class
 *  \return a reference to the new class object
 */
CBEClass * CL4BEClassFactory::GetNewClass()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEClass\n");
    return new CL4BEClass;
}

/** \brief create new ipc class
 *  \return a reference to the new ipc object
 */
CBECommunication* CL4BEClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEIPC\n");
    return new CL4BEIPC();
}

/** \brief creates a new reply function
 *  \return a reference to the new instance
 */
CBEReplyFunction* CL4BEClassFactory::GetNewReplyFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEReplyFunction\n");
    return new CL4BEReplyFunction();
}

/** \brief creates a new marshal function
 *  \return a reference to the new instance
 */
CBEMarshalFunction* CL4BEClassFactory::GetNewMarshalFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEMarshalFunction\n");
    return new CL4BEMarshalFunction();
}

/** \brief creates a new dispatch function
 *  \return a reference to the new instance
 */
CBEDispatchFunction* CL4BEClassFactory::GetNewDispatchFunction()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BEDispatchFunction\n");
    return new CL4BEDispatchFunction();
}

/** \brief creates a new switch case function
 *  \return a reference to the new instance
 */
CBESwitchCase* CL4BEClassFactory::GetNewSwitchCase()
{
    if (m_bVerbose)
        printf("CL4BEClassFactory: created class CL4BESwitchCase\n");
    return new CL4BESwitchCase();
}
