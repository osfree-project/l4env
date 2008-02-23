/**
 *    \file    dice/src/be/l4/v4/L4V4BEClassFactory.cpp
 *  \brief   contains the implementation of the class CL4V4BEClassFactory
 *
 *    \date    01/06/2004
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

#include "L4V4BEClassFactory.h"
#include "L4V4BEIPC.h"
#include "L4V4BESizes.h"
#include "L4V4BECallFunction.h"
#include "L4V4BEReplyFunction.h"
#include "L4V4BEMarshalFunction.h"
#include "L4V4BEWaitAnyFunction.h"
#include "L4V4BEWaitFunction.h"
#include "L4V4BESndFunction.h"
#include "L4V4BEMsgBuffer.h"
#include "L4V4BEMsgBufferType.h"
#include "L4V4BEMarshaller.h"
#include "L4V4BESrvLoopFunction.h"
#include "be/BEContext.h"
#include "Compiler.h"
#include <iostream>

CL4V4BEClassFactory::CL4V4BEClassFactory()
: CL4BEClassFactory()
{
}

/** \brief the destructor of this class */
CL4V4BEClassFactory::~CL4V4BEClassFactory()
{
}

/** \brief create a new instance of the CL4V4BEIPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4V4BEClassFactory::GetNewCommunication()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEIPC\n");
	return new CL4V4BEIPC();
}

/** \brief creates a new sizes class
 *  \return a reference to the new instance
 */
CBESizes* CL4V4BEClassFactory::GetNewSizes()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BESizes\n");
	return new CL4V4BESizes();
}

/** \brief creates a new call function class
 *  \return a reference to the new instance
 */
CBECallFunction* CL4V4BEClassFactory::GetNewCallFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BECallFunction\n");
	return new CL4V4BECallFunction();
}
/** \brief creates a new reply function
 *  \return a reference to the new instance
 */
CBEReplyFunction* CL4V4BEClassFactory::GetNewReplyFunction()
{
    CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEReplyFunction\n");
    return new CL4V4BEReplyFunction();
}


/** \brief creates a new marshal function
 *  \return a reference to the new instance
 */
CBEMarshalFunction* CL4V4BEClassFactory::GetNewMarshalFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEMarshalFunction\n");
	return new CL4V4BEMarshalFunction();
}

/** \brief creates a new wait any function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction* CL4V4BEClassFactory::GetNewWaitAnyFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEWaitAnyFunction\n");
	return new CL4V4BEWaitAnyFunction(true, false);
}

/** \brief creates a new recv any function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction* CL4V4BEClassFactory::GetNewRecvAnyFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEWaitAnyFunction\n");
	return new CL4V4BEWaitAnyFunction(false, false);
}

/** \brief creates a new reply-and-wait function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction *CL4V4BEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEReplyAnyWaitAnyFunction\n");
	return new CL4V4BEWaitAnyFunction(true, true);
}

/** \brief creates a new send function
 *  \return a reference to the new instance
 */
CBESndFunction* CL4V4BEClassFactory::GetNewSndFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BESndFunction\n");
	return new CL4V4BESndFunction();
}

/** \brief creates a new receive function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4V4BEClassFactory::GetNewRcvFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEWaitFunction\n");
	return new CL4V4BEWaitFunction(false);
}

/** \brief creates a new wait function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4V4BEClassFactory::GetNewWaitFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created class CL4V4BEWaitFunction\n");
	return new CL4V4BEWaitFunction(true);
}

/** \brief creates a new instance of a message buffer
 *  \return a reference to the newly created instance
 */
CBEMsgBuffer* CL4V4BEClassFactory::GetNewMessageBuffer()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created new instance of CL4V4BEMsgBuffer\n");
	return new CL4V4BEMsgBuffer();
}

/** \brief create a new instance of the class CBEMsgBufferType
 *  \return a reference to the newly created class
 */
CBEMsgBufferType* CL4V4BEClassFactory::GetNewMessageBufferType()
{
	CCompiler::Verbose("CL4V4BEClassFactory: create class CL4V4BEMsgBufferType\n");
	return new CL4V4BEMsgBufferType();
}

/** \brief creates a new instance of a marshaller
 *  \return a reference to the newly created instance
 */
CBEMarshaller* CL4V4BEClassFactory::GetNewMarshaller()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created new instance of CL4V4BEMarshaller\n");
	return new CL4V4BEMarshaller();
}

/** \brief creates a new instance of a server loop function
 *  \return a reference to the newly created instance
 */
CBESrvLoopFunction* CL4V4BEClassFactory::GetNewSrvLoopFunction()
{
	CCompiler::Verbose("CL4V4BEClassFactory: created new instance of CL4V4BESrvLoopFunction\n");
	return new CL4V4BESrvLoopFunction();
}

