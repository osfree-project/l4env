/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEClassFactory.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEClassFactory
 *
 *  \date    20/08/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "L4FiascoBEClassFactory.h"
#include "L4FiascoBESizes.h"
#include "L4FiascoBEIPC.h"
#include "L4FiascoBEMsgBuffer.h"
#include "L4FiascoBEDispatchFunction.h"
#include "L4FiascoBECallFunction.h"
#include "L4FiascoBESrvLoopFunction.h"
#include "L4FiascoBEWaitAnyFunction.h"
#include "L4FiascoBEUnmarshalFunction.h"
#include "L4FiascoBEMarshalFunction.h"
#include "L4FiascoBEWaitFunction.h"
#include "L4FiascoBESndFunction.h"
#include "L4FiascoBEReplyFunction.h"
#include "L4FiascoBEClass.h"

#include "Compiler.h"

CL4FiascoBEClassFactory::CL4FiascoBEClassFactory()
: CL4BEClassFactory()
{ }

/** \brief the destructor of this class */
CL4FiascoBEClassFactory::~CL4FiascoBEClassFactory()
{ }

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4FiascoBEClassFactory::GetNewSizes()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4FiascoBEClassFactory: created class CL4V2BESizes\n");
	return new CL4FiascoBESizes();
}

/** \brief create a new IPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4FiascoBEClassFactory::GetNewCommunication()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4FiascoBEClassFactory: created class CL4V2BEIPC\n");
	return new CL4FiascoBEIPC();
}

/** \brief creates a new message buffer class
 *  \return a reference to the newly created object
 */
CBEMsgBuffer* CL4FiascoBEClassFactory::GetNewMessageBuffer()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEMsgBuffer\n");
	return new CL4FiascoBEMsgBuffer();
}

/** \brief creates a new dispatch function
 *  \return a reference to the new instance
 */
CBEDispatchFunction* CL4FiascoBEClassFactory::GetNewDispatchFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEDispatchFunction\n");
	return new CL4FiascoBEDispatchFunction();
}

/** \brief creates a new call function
 *  \return a reference to the new instance
 */
CBECallFunction* CL4FiascoBEClassFactory::GetNewCallFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBECallFunction\n");
	return new CL4FiascoBECallFunction();
}

/** \brief create a new class class
 *  \return a reference to the new class object
 */
CBEClass * CL4FiascoBEClassFactory::GetNewClass()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEClass\n");
	return new CL4FiascoBEClass;
}

/** \brief creates a new instance of the class CBESrvLoopFunction
 *  \return a reference to the new instance
 */
CBESrvLoopFunction *CL4FiascoBEClassFactory::GetNewSrvLoopFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBESrvLoopFunction\n");
	return new CL4FiascoBESrvLoopFunction();
}

/** \brief creates a new instance of the class CBEWaitAnyFunction
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction *CL4FiascoBEClassFactory::GetNewWaitAnyFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEWaitAnyFunction\n");
	return new CL4FiascoBEWaitAnyFunction(true, false);
}

/** \brief creates a new reply-and-wait function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4FiascoBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEReplyAnyWaitAnyFunction\n");
	return new CL4FiascoBEWaitAnyFunction(true, true);
}

/** \brief creates a new rcv-any function
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction * CL4FiascoBEClassFactory::GetNewRcvAnyFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEWaitAnyFunction\n");
	return new CL4FiascoBEWaitAnyFunction(false, false);
}

/** \brief creates a new unmarshal function
 *  \return a reference to the new instance
 */
CBEUnmarshalFunction * CL4FiascoBEClassFactory::GetNewUnmarshalFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEUnmarshalFunction\n");
	return new CL4FiascoBEUnmarshalFunction();
}

/** \brief creates a new marshal function
 *  \return a reference to the new instance
 */
CBEMarshalFunction * CL4FiascoBEClassFactory::GetNewMarshalFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEMarshalFunction\n");
	return new CL4FiascoBEMarshalFunction();
}

/** \brief creates a new receive function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4FiascoBEClassFactory::GetNewRcvFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEWaitFunction\n");
	return new CL4FiascoBEWaitFunction(false);
}

/** \brief creates a new wait function
 *  \return a reference to the new receive function
 */
CBEWaitFunction * CL4FiascoBEClassFactory::GetNewWaitFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEWaitFunction\n");
	return new CL4FiascoBEWaitFunction(true);
}

/** \brief creates a new send function
 *  \return a reference to the new receive function
 */
CBESndFunction * CL4FiascoBEClassFactory::GetNewSndFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBESndFunction\n");
	return new CL4FiascoBESndFunction();
}

/** \brief creates a new reply function
 *  \return a reference to the new receive function
 */
CBEReplyFunction * CL4FiascoBEClassFactory::GetNewReplyFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4FiascoBEClassFactory: created class CL4FiascoBEReplyFunction\n");
	return new CL4FiascoBEReplyFunction();
}

