/**
 *    \file    dice/src/be/l4/L4BEIPC.cpp
 *    \brief   contains the implementation of the class CL4BEIPC
 *
 *    \date    04/18/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4BEIPC.h"
#include "L4BENameFactory.h"
#include "L4BEMsgBuffer.h"
#include "L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"

#include "be/BEMarshalFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BECallFunction.h"
#include "be/BESndFunction.h"
#include "be/BEReplyFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEWaitAnyFunction.h"

#include "Compiler.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEIPC::CL4BEIPC()
{
}

/** \brief destructor of target class */
CL4BEIPC::~CL4BEIPC()
{
}

/** \brief determine if we should use assembler for the IPCs
 *  \param pFunction the function to write the call for
 *  \return true if assembler code should be written
 *
 * This implementation currently always returns false, because assembler code
 * is always ABI specific.
 */
bool
CL4BEIPC::UseAssembler(CBEFunction *)
{
    return false;
}

/** \brief helper function to test for short IPC
 *  \param pFunction the function to test
 *  \param nDirection the direction to test
 *  \return true if the function uses short IPC in the specified direction
 *
 * This is a simple helper function, which just delegates the call to the
 * function's message buffer.
 */
bool
CL4BEIPC::IsShortIPC(CBEFunction *pFunction,
    DIRECTION_TYPE nDirection)
{
    if (nDirection == 0)
	return IsShortIPC(pFunction, pFunction->GetSendDirection()) &&
	    IsShortIPC(pFunction, pFunction->GetReceiveDirection());

    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    return pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC,
	nDirection);
}

/** \brief writes a send with an immediate wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *
 * Invoke the specialized method with default parameters.
 */
void
CL4BEIPC::WriteReplyAndWait(CBEFile& pFile,
    CBEFunction* pFunction)
{
    WriteReplyAndWait(pFile, pFunction, false, false);
}

