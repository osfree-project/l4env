/**
 *    \file    dice/src/be/sock/SockBEWaitAnyFunction.cpp
 *  \brief   contains the implementation of the class CSockBEWaitAnyFunction
 *
 *    \date    11/28/2002
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

#include "be/sock/SockBEWaitAnyFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "be/BEClassFactory.h"
#include "be/BENameFactory.h"
#include "be/sock/BESocket.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include <cassert>

CSockBEWaitAnyFunction::CSockBEWaitAnyFunction(bool bOpenWait, bool bReply)
: CBEWaitAnyFunction(bOpenWait, bReply)
{ }

/** \brief destructor of target class */
CSockBEWaitAnyFunction::~CSockBEWaitAnyFunction()
{ }

/** \brief writes the message invocation
 *  \param pFile the file to write to
 *
 * The socket has to be open already.
 */
void CSockBEWaitAnyFunction::WriteInvocation(CBEFile& pFile)
{
	// wait for new request
	CBECommunication *pComm = GetCommunication();
	assert(pComm);
	if (m_bReply)
		pComm->WriteReplyAndWait(pFile, this);
	else
		pComm->WriteWait(pFile, this);
}

/** \brief initializes this instance of the class
 *  \param pFEInterface the front-end interface to use as reference
 *  \return true if successful
 */
void CSockBEWaitAnyFunction::CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide)
{
	CBEWaitAnyFunction::CreateBackEnd(pFEInterface, bComponentSide);

	// add local variables
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pVariable = pCF->GetNewTypedDeclarator();
	CBEType *pType = pCF->GetNewType(TYPE_INTEGER);
	pType->SetParent(pVariable);
	AddLocalVariable(pVariable);
	pType->CreateBackEnd(false, 4, TYPE_INTEGER);
	pVariable->CreateBackEnd(pType, string("dice_ret_size"));
	delete pType; // has been cloned by typed decl.

	pVariable = pCF->GetNewTypedDeclarator();
	AddLocalVariable(pVariable);
	pVariable->CreateBackEnd(string("socklen_t"), string("dice_fromlen"), 0);
	string sInit = "sizeof(*" +
		CBENameFactory::Instance()->GetCorbaObjectVariable() + ")";
	pVariable->SetDefaultInitString(sInit);
}

