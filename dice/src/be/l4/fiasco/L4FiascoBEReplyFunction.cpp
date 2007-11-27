/**
 *    \file    dice/src/be/l4/fiasco/L4FiascoBEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4FiascoBEReplyFunction
 *
 *    \date    11/06/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4FiascoBEReplyFunction.h"
#include "L4FiascoBENameFactory.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/BEClass.h"
#include "be/BECallFunction.h"
#include <string>
using std::string;
#include <cassert>

CL4FiascoBEReplyFunction::CL4FiascoBEReplyFunction()
{ }

/** destroy the object */
CL4FiascoBEReplyFunction::~CL4FiascoBEReplyFunction()
{ }

/** \brief creates the back-end reply function
 *  \param pFEOperation the corresponding front-end operation
 *  \param bComponentSide true if this function is created at component side
 *  \return true if successful
 *
 * We have make the message buffer access the UTCB if necessary.
 *
 * Because the reply function can only use UTCB IPC if the respective call
 * function does. Search the call function and check its message buffer for
 * UTCB IPC. (reply might be UTCB IPC capable, but send not...)
 */
void CL4FiascoBEReplyFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	CL4BEReplyFunction::CreateBackEnd(pFEOperation, bComponentSide);

	// get the message buffer of the call function
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	CBECallFunction *pCallFunc = dynamic_cast<CBECallFunction*>(pClass->FindFunctionFor(this, FUNCTION_CALL));
	assert(pCallFunc);
	CBEMsgBuffer *pMsgBuffer = pCallFunc->GetMessageBuffer();
	if (pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_UTCB_IPC, CMsgStructType::Generic))
	{
		// now that we know we can use UTCB IPC, get our own message buffer
		pMsgBuffer = GetMessageBuffer();
		pMsgBuffer->m_Declarators.First()->SetStars(1);
		string sInit;
		pMsgBuffer->GetType(this)->WriteCastToStr(sInit, true);
		CBENameFactory *pNF = CBENameFactory::Instance();
		sInit += pNF->GetString(CL4FiascoBENameFactory::STR_UTCB_INITIALIZER, this);
		pMsgBuffer->SetDefaultInitString(sInit);
		pMsgBuffer->AddLanguageProperty(string("attribute"), string("__attribute__ ((unused))"));
	}
}
