/**
 *  \file    dice/src/be/l4/v2/L4V2BEDispatchFunction.cpp
 *  \brief   contains the implementation of the class CL4V2BEDispatchFunction
 *
 *  \date    08/03/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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
#include "L4V2BEDispatchFunction.h"
#include "L4V2BEMsgBuffer.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4V2BEDispatchFunction::CL4V2BEDispatchFunction()
{ }

/** \brief destructor of target class */
CL4V2BEDispatchFunction::~CL4V2BEDispatchFunction()
{ }

/** \brief write the L4 specific code when setting the opcode exception in \
 *         the message buffer
 *  \param pFile the file to write to
 */
void CL4V2BEDispatchFunction::WriteSetWrongOpcodeException(CBEFile& pFile)
{
	// first call base class
	CL4BEDispatchFunction::WriteSetWrongOpcodeException(pFile);
	// set short IPC
	CL4V2BEMsgBuffer *pMsgBuffer =
		dynamic_cast<CL4V2BEMsgBuffer*>(GetMessageBuffer());
	assert(pMsgBuffer);
	pMsgBuffer->WriteDopeShortInitialization(pFile, TYPE_MSGDOPE_SEND,
		CMsgStructType::Generic);
}

