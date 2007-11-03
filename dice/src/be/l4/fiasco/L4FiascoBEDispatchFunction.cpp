/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEDispatchFunction.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEDispatchFunction
 *
 *  \date    08/24/2007
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
#include "L4FiascoBEDispatchFunction.h"
#include "L4FiascoBEMsgBuffer.h"
#include "be/BEFile.h"
#include "be/BEClassFactory.h"
#include "be/l4/L4BENameFactory.h"
#include "TypeSpec-Type.h"
#include <cassert>

CL4FiascoBEDispatchFunction::CL4FiascoBEDispatchFunction()
{ }

/** \brief destructor of target class */
CL4FiascoBEDispatchFunction::~CL4FiascoBEDispatchFunction()
{ }

/** \brief adds parameters before all other parameters
 *
 * We use this function to add the message tag variable.
 */
void CL4FiascoBEDispatchFunction::AddBeforeParameters()
{
	CL4BEDispatchFunction::AddBeforeParameters();

	if (!m_sDefaultFunction.empty())
	{
		CBENameFactory *pNF = CBENameFactory::Instance();
		std::string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
		std::string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);

		CBEClassFactory *pCF = CBEClassFactory::Instance();
		CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
		m_Parameters.Add(pParameter);
		pParameter->CreateBackEnd(sTagType, sTagVar, 1);
	}
}

/** \brief writes the default case if there is a default function
 *  \param pFile the file to write to
 */
void CL4FiascoBEDispatchFunction::WriteDefaultCaseWithDefaultFunc(CBEFile& pFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	std::string sMsgBuffer = pNF->GetMessageBufferVariable();
	std::string sObj = pNF->GetCorbaObjectVariable();
	std::string sEnv = pNF->GetCorbaEnvironmentVariable();
	std::string sTagVar = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);

	pFile << "\treturn " << m_sDefaultFunction << " (" << sObj <<
		", " << sTagVar << ", " << sMsgBuffer << ", " << sEnv << ");\n";
}

/** \brief write the L4 specific code when setting the opcode exception in \
 *         the message buffer
 *  \param pFile the file to write to
 *
 * If we have a wrong opcode, then we have to send a reply to the client that
 * is an exception and fits (currently) into a short IPC.
 */
void CL4FiascoBEDispatchFunction::WriteSetWrongOpcodeException(CBEFile& pFile)
{
	// first call base class
	CL4BEDispatchFunction::WriteSetWrongOpcodeException(pFile);
	// set short IPC
	CL4FiascoBEMsgBuffer *pMsgBuffer = dynamic_cast<CL4FiascoBEMsgBuffer*>(GetMessageBuffer());
	assert(pMsgBuffer);
	pMsgBuffer->WriteDopeShortInitialization(pFile, TYPE_MSGDOPE_SEND, CMsgStructType::Generic);
}

/** \brief writes the decalaration of the default function
 *  \param pFile the file to write to
 */
void CL4FiascoBEDispatchFunction::WriteDefaultFunctionDeclaration(CBEFile& pFile)
{
	// add declaration of default function
	if (m_sDefaultFunction.empty())
		return;

	// get the class' message buffer to get the correct type
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	CBEMsgBuffer *pMsgBuffer = pClass->GetMessageBuffer();
	assert(pMsgBuffer);
	std::string sMsgBufferType = pMsgBuffer->m_Declarators.First()->GetName();
	CBENameFactory *pNF = CBENameFactory::Instance();
	std::string sTagType = pNF->GetTypeName(TYPE_MSGTAG, 0);
	// int \<name\>(\<corba object\>, \<msg buffer type\>*,
	//              \<corba environment\>*)
	pFile << "int " << m_sDefaultFunction << " (CORBA_Object, " << sTagType <<
		"*, " << sMsgBufferType << "*, CORBA_Server_Environment*);\n";
}

