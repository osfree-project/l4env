/**
 *  \file    dice/src/be/l4/L4BEMarshalFunction.cpp
 *  \brief   contains the implementation of the class CL4BEMarshalFunction
 *
 *  \date    10/10/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "L4BEMarshalFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEUserDefinedType.h"
#include "be/BEContext.h"
#include "be/BEClass.h"
#include "be/BEMsgBuffer.h"
#include "be/BESizes.h"
#include "be/Trace.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BEMarshalFunction::CL4BEMarshalFunction()
{ }

/** \brief destructor of target class */
CL4BEMarshalFunction::~CL4BEMarshalFunction()
{ }

/** \brief write the L4 specific unmarshalling code
 *  \param pFile the file to write to
 *
 * If we send flexpages we have to do special handling:
 * if (exception)
 *   marshal exception
 * else
 *   marshal normal flexpages
 *
 * Without flexpages:
 * marshal exception
 * marshal rest
 */
void CL4BEMarshalFunction::WriteMarshalling(CBEFile& pFile)
{
	bool bLocalTrace = false;
	if (!m_bTraceOn && m_pTrace)
	{
		m_pTrace->BeforeMarshalling(pFile, this);
		m_bTraceOn = bLocalTrace = true;
	}

	CBEMsgBuffer *pMsgBuffer = m_pClass->GetMessageBuffer();
	CMsgStructType nType = GetSendDirection();
	bool bSendFpages = GetParameterCount(TYPE_FLEXPAGE, GetSendDirection()) > 0;
	if (bSendFpages)
	{
		// if (env.major == CORBA_NO_EXCEPTION)
		//   marshal flexpages
		// else
		//   marshal exception
		CBETypedDeclarator *pEnv = GetEnvironment();
		string sFreeFunc;
		if (((CBEUserDefinedType*)pEnv->GetType())->GetName() ==
			"CORBA_Server_Environment")
			sFreeFunc = "CORBA_server_exception_free";
		else
			sFreeFunc = "CORBA_exception_free";
		CBEDeclarator *pDecl = pEnv->m_Declarators.First();
		string sEnv;
		if (pDecl->GetStars() == 0)
			sEnv = "&";
		sEnv += pDecl->GetName();

		pFile << "\tif (DICE_EXPECT_TRUE(DICE_IS_NO_EXCEPTION(" <<
			sEnv << ")))\n";
		pFile << "\t{\n";
		++pFile;
		CBEOperationFunction::WriteMarshalling(pFile);
		--pFile << "\t}\n";
		pFile << "\telse\n";
		pFile << "\t{\n";
		++pFile;
		WriteMarshalException(pFile, true, false);
		// clear exception
		pFile << "\t" << sFreeFunc << "(" << sEnv << ");\n";
		// set send dope
		pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
			nType);
		// write return (don't marshal any parameters if exception)
		WriteReturn(pFile);
		--pFile << "\t}\n";
	}
	else
		CBEMarshalFunction::WriteMarshalling(pFile);

	// set send dope
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
		nType);
	// if we had send flexpages,we have to set the flexpage bit
	if (bSendFpages)
	{
		pFile << "\t";
		pMsgBuffer->WriteMemberAccess(pFile, this, nType,
			TYPE_MSGDOPE_SEND, 0);
		pFile << ".md.fpage_received = 1;\n";
	}

	if (bLocalTrace)
	{
		m_pTrace->AfterMarshalling(pFile, this);
		m_bTraceOn = false;
	}
}

/** \brief test if this function has variable sized parameters (needed to \
 *         specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEMarshalFunction::HasVariableSizedParameters(DIRECTION_TYPE nDirection)
{
	bool bRet =
		CBEMarshalFunction::HasVariableSizedParameters(nDirection);
	bool bFixedNumberOfFlexpages = true;
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages, DIRECTION_INOUT);
	// if no flexpages, return
	if (!bFixedNumberOfFlexpages)
		return true;
	// if we have indirect strings to marshal then we need the offset vars
	if (GetParameterCount(ATTR_REF, ATTR_NONE, nDirection))
		return true;
	return bRet;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we send flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEMarshalFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
	int nSize = CBEMarshalFunction::GetFixedSize(nDirection);
	if ((nDirection & DIRECTION_OUT) &&
		!m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
		(GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
		nSize -= CCompiler::GetSizes()->GetExceptionSize();
	return nSize;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we send flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEMarshalFunction::GetSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEMarshalFunction::GetSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

