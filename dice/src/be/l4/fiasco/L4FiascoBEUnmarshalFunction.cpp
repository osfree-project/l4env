/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEUnmarshalFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEUnmarshalFunction
 *
 *  \date    11/02/2007
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

#include "L4FiascoBEUnmarshalFunction.h"
#include "L4FiascoBEMsgBuffer.h"
#include "L4FiascoBENameFactory.h"
#include <string>
using std::string;
#include <cassert>

CL4FiascoBEUnmarshalFunction::CL4FiascoBEUnmarshalFunction()
{ }

/** \brief destructor of target class */
CL4FiascoBEUnmarshalFunction::~CL4FiascoBEUnmarshalFunction()
{ }

/** \brief creates the back-end unmarshal function
 *  \param pFEOperation the corresponding front-end operation
 *  \param bComponentSide true if this function is created at component side
 *  \return true if successful
 *
 * We have to add an extra local variable for a UTCB based message buffer.
 */
void CL4FiascoBEUnmarshalFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	CL4BEUnmarshalFunction::CreateBackEnd(pFEOperation, bComponentSide);

	// get the message buffer of the unmarshal function (NOT the class'
	// message buffer)
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_UTCB_IPC, CMsgStructType::Generic))
	{
		AddLocalVariable(pMsgBuffer);
		pMsgBuffer->m_Declarators.First()->SetStars(1);
		string sInit;
		pMsgBuffer->GetType(this)->WriteCastToStr(sInit, true);
		CBENameFactory *pNF = CBENameFactory::Instance();
		sInit += pNF->GetString(CL4FiascoBENameFactory::STR_UTCB_INITIALIZER, this);
		pMsgBuffer->SetDefaultInitString(sInit);
		pMsgBuffer->AddLanguageProperty(string("attribute"), string("__attribute__ ((unused))"));

		// because the message buffer is not yet created in AddAfterParameters
		// and we need it to check if we use UTCB IPC we have to delay the
		// modification of the parameter until after the message buffer is
		// available which happens to be now.
		string sName = pNF->GetMessageBufferVariable();
		CBETypedDeclarator* pParameter = m_Parameters.Find(sName);
		assert(pParameter);
		string sDummyName = pNF->GetDummyVariable(sName);
		CBEDeclarator *pDecl = pParameter->m_Declarators.First();
		pDecl->SetName(sDummyName);
		// to avoid usage of internal parameter names, set the call variable
		// name of the message buffer to the previous name
		SetCallVariable(sDummyName, pDecl->GetStars(), sName);
	}
}
