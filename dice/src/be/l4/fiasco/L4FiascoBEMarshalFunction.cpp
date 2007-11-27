/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEMarshalFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEMarshalFunction
 *
 *  \date    11/06/2007
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

#include "L4FiascoBEMarshalFunction.h"
#include "L4FiascoBEMsgBuffer.h"
#include "L4FiascoBENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BEFile.h"
#include "be/BEClass.h"
#include "be/BESizes.h"
#include "Compiler.h"
#include <string>
using std::string;
#include <cassert>

CL4FiascoBEMarshalFunction::CL4FiascoBEMarshalFunction()
{ }

/** \brief destructor of target class */
CL4FiascoBEMarshalFunction::~CL4FiascoBEMarshalFunction()
{ }

/** \brief creates the back-end unmarshal function
 *  \param pFEOperation the corresponding front-end operation
 *  \param bComponentSide true if this function is created at component side
 *  \return true if successful
 *
 * We have to add an extra local variable for a UTCB based message buffer.
 */
void CL4FiascoBEMarshalFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	CL4BEMarshalFunction::CreateBackEnd(pFEOperation, bComponentSide);

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

/** \brief adds parameters before all other parameters
 *
 * We use this function to add the message tag variable.
 */
void CL4FiascoBEMarshalFunction::AddBeforeParameters()
{
	CL4BEMarshalFunction::AddBeforeParameters();

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
	m_Parameters.Add(pParameter);
	pParameter->CreateBackEnd(sTagType, sTagVar, 1);
}

/** \brief write the L4 specific unmarshalling code
 *  \param pFile the file to write to
 *
 * If we send UTCB IPC we have to override the send dope to set short IPC.
 */
void CL4FiascoBEMarshalFunction::WriteMarshalling(CBEFile& pFile)
{
	CL4BEMarshalFunction::WriteMarshalling(pFile);

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_UTCB_IPC, CMsgStructType::Generic))
	{
		// set dope in class' message buffer
		CBEClass *pClass = GetSpecificParent<CBEClass>();
		assert(pClass);
		CL4FiascoBEMsgBuffer *pClassBuffer = dynamic_cast<CL4FiascoBEMsgBuffer*>(pClass->GetMessageBuffer());
		pClassBuffer->WriteDopeShortInitialization(pFile, this, TYPE_MSGDOPE_SEND, CMsgStructType::Generic);

		// set msgtag
		int nSize = 0;
		pMsgBuffer->GetMaxSize(nSize, this, GetSendDirection());
		CBESizes *pSizes = CCompiler::GetSizes();
		nSize = pSizes->WordsFromBytes(nSize);
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
		pFile << "\t*" << sTagVar << " = l4_msgtag(0, " << nSize << ", 0, 0);\n";

		// store the first two values from the UTCB in the message buffer,
		// because these are used as IPC parameters directly and unmarshalled
		// from there...
		CBETypedDeclarator *pVariable = pMsgBuffer->GetVariable(this);
		string sMsgBufVar = pVariable->m_Declarators.First()->GetName();
		pFile << "\t";
		pClassBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
		pFile << " = ";
		pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
		pFile << ";\n";
		pFile << "\t";
		pClassBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 1);
		pFile << " = ";
		pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 1);
		pFile << ";\n";
		// "((l4_umword_t*)" << sMsgBufVar << ")[1];\n";
	}
}
