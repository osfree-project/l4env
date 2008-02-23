/**
 *  \file    dice/src/be/l4/v2/L4V2BEClassFactory.cpp
 *  \brief   contains the implementation of the class CL4V2BEClassFactory
 *
 *  \date    02/07/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "L4V2BESizes.h"
#include "L4V2BEMsgBuffer.h"
#include "L4V2BEDispatchFunction.h"
#include "L4V2BEIPC.h"

#include "Compiler.h"
#include "be/BEContext.h"
#include <iostream>

CL4V2BEClassFactory::CL4V2BEClassFactory()
: CL4BEClassFactory()
{
}

/** \brief the destructor of this class */
CL4V2BEClassFactory::~CL4V2BEClassFactory()
{

}

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4V2BEClassFactory::GetNewSizes()
{
    CCompiler::Verbose("CL4V2BEClassFactory: created class CL4V2BESizes\n");
    return new CL4V2BESizes();
}

/** \brief create a new IPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4V2BEClassFactory::GetNewCommunication()
{
    CCompiler::Verbose("CL4V2BEClassFactory: created class CL4V2BEIPC\n");
    return new CL4V2BEIPC();
}

/** \brief creates a new dispatch function
 *  \return a reference to the newly created object
 */
CBEDispatchFunction* CL4V2BEClassFactory::GetNewDispatchFunction()
{
    CCompiler::Verbose("CL4V2BEClassFactory: created class CL4V2BEDispatchFunction\n");
    return new CL4V2BEDispatchFunction();
}

/** \brief creates a new message buffer class
 *  \return a reference to the newly created object
 */
CBEMsgBuffer* CL4V2BEClassFactory::GetNewMessageBuffer()
{
    CCompiler::Verbose("CL4V2BEClassFactory: created class CL4V2BEMsgBuffer\n");
    return new CL4V2BEMsgBuffer();
}

