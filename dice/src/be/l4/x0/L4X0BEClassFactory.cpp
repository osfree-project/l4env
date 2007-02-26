/**
 *    \file    dice/src/be/l4/x0/L4X0BEClassFactory.cpp
 *  \brief   contains the implementation of the class CL4X0BEClassFactory
 *
 *    \date    12/01/2002
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

#include "L4X0BEClassFactory.h"
#include "L4X0BEDispatchFunction.h"
#include "L4X0BEMsgBuffer.h"
#include "L4X0BESizes.h"
#include "L4X0BETrace.h"
#include "L4X0BEIPC.h"
#include "be/BEContext.h"
#include "Compiler.h"
#include <iostream>

CL4X0BEClassFactory::CL4X0BEClassFactory()
: CL4BEClassFactory()
{
}

/** \brief the destructor of this class */
CL4X0BEClassFactory::~CL4X0BEClassFactory()
{
}

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4X0BEClassFactory::GetNewSizes()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4X0BEClassFactory: created class CL4X0BESizes\n");
    return new CL4X0BESizes();
}

/** \brief creates a new instance of the class CBETrace
 *  \return a reference to the new instance
 */
CBETrace* CL4X0BEClassFactory::GetNewTrace()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4X0BEClassFactory: created class CL4X0BETrace\n");
    return new CL4X0BETrace();
}

/** \brief create a new IPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4X0BEClassFactory::GetNewCommunication()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4X0BEClassFactory: created class CL4X0BEIPC\n");
    return new CL4X0BEIPC();
}

/** \brief creates a new dispatch function
 *  \return a reference to the newly created object
 */
CBEDispatchFunction* CL4X0BEClassFactory::GetNewDispatchFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CL4X0BEClassFactory: created class CL4X0BEDispatchFunction\n");
    return new CL4X0BEDispatchFunction();
}

/** \brief creates a new message buffer class
 *  \return a reference to the newly created object
 */
CBEMsgBuffer* CL4X0BEClassFactory::GetNewMessageBuffer()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4X0BEClassFactory: created class CL4X0BEMsgBuffer\n");
    return new CL4X0BEMsgBuffer();
}

