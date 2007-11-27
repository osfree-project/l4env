/**
 *  \file    dice/src/be/BESwitchCase.cpp
 *  \brief   contains the implementation of the class CBESwitchCase
 *
 *  \date    01/19/2002
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

#include "BESwitchCase.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEType.h"
#include "BEDeclarator.h"
#include "BETypedDeclarator.h"
#include "BETypedef.h"
#include "BEUnmarshalFunction.h"
#include "BEMarshalFunction.h"
#include "BEMarshalExceptionFunction.h"
#include "BEComponentFunction.h"
#include "BERoot.h"
#include "BEClass.h"
#include "BEMsgBuffer.h"
#include "BENameFactory.h"
#include "Compiler.h"
#include "Error.h"
#include "Trace.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "TypeSpec-Type.h"
#include "fe/FEStructType.h"
#include <cassert>
#include <iostream>

CBESwitchCase::CBESwitchCase()
: CBEOperationFunction(FUNCTION_SWITCH_CASE)
{
	m_bSameClass = true;
	m_pUnmarshalFunction = 0;
	m_pMarshalFunction = 0;
	m_pMarshalExceptionFunction = 0;
	m_pComponentFunction = 0;
}

/** \brief destructor of target class */
CBESwitchCase::~CBESwitchCase()
{ }

/** \brief check if parameter has ATTR_IN attribute
 *  \param pParameter the parameter to test
 *  \return true if parameter has IN attribute
 */
static bool checkForInAttr(CFETypedDeclarator *pParameter)
{
	return pParameter->m_Attributes.Find(ATTR_IN) != 0;
}

/** \brief creates the back-end receive function
 *  \param pFEOperation the corresponding front-end operation
 *  \param bComponentSide true if this function is create at component side
 *  \return true is successful
 *
 * This implementation calls the base class' implementation and then sets the
 * name of the function.
 */
void CBESwitchCase::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	string exc = string(__func__);
	// we skip OUT functions
	if (pFEOperation->m_Attributes.Find(ATTR_OUT))
		return;
	// set name
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEOperation, FUNCTION_SWITCH_CASE);

	CBEOperationFunction::CreateBackEnd(pFEOperation, bComponentSide);

	CreateTrace();

	// get opcode const
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sOpcode = pNF->GetOpcodeConst(pFEOperation);

	// check for uuid-range
	if (pFEOperation->m_Attributes.Find(ATTR_UUID_RANGE))
		m_sUpper = pNF->GetOpcodeConst(pFEOperation, true);

	// check if this switch case is from the same class as the surrounding
	// dispatcher function or if this is from a base class.
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	if (pClass->GetName() ==
		pFEOperation->GetSpecificParent<CFEInterface>()->GetName())
		m_bSameClass = true;
	else
		m_bSameClass = false;

	string sFunctionName;
	// check if we need unmarshalling function
	vector<CFETypedDeclarator*>::iterator iterP = find_if(pFEOperation->m_Parameters.begin(),
		pFEOperation->m_Parameters.end(), checkForInAttr);
	if (iterP != pFEOperation->m_Parameters.end())
	{
		// create references to unmarshal function
		sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_UNMARSHAL, IsComponentSide());
		m_pUnmarshalFunction = dynamic_cast<CBEUnmarshalFunction *>(
			FindFunction(sFunctionName, FUNCTION_UNMARSHAL));
		assert(m_pUnmarshalFunction);
		// set the call parameters: this is simple, since we use the same
		// names and reference counts
		for_each(m_Parameters.begin(), m_Parameters.end(),
			SetCallVariableCall(m_pUnmarshalFunction));
	}
	// check if we need marshalling function
	// basically we alwas need marshalling, because we at least transfer
	// that there was no exception
	// the only exception is if the function is a oneway (IN) function
	if (!pFEOperation->m_Attributes.Find(ATTR_IN))
	{
		// create reference to marshal function
		sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL, IsComponentSide());
		m_pMarshalFunction = static_cast<CBEMarshalFunction*>(
			FindFunction(sFunctionName, FUNCTION_MARSHAL));
		assert(m_pMarshalFunction);
		// set call parameters
		for_each(m_Parameters.begin(), m_Parameters.end(),
			SetCallVariableCall(m_pMarshalFunction));
	}
	if (!pFEOperation->m_RaisesDeclarators.empty())
	{
		sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL_EXCEPTION, IsComponentSide());
		m_pMarshalExceptionFunction = static_cast<CBEMarshalExceptionFunction*>(
			FindFunction(sFunctionName, FUNCTION_MARSHAL_EXCEPTION));
		assert(m_pMarshalExceptionFunction);
		// set call parameters
		for_each(m_Parameters.begin(), m_Parameters.end(),
			SetCallVariableCall(m_pMarshalExceptionFunction));
	}
	// create reference to component function
	sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_TEMPLATE, IsComponentSide());
	m_pComponentFunction = static_cast<CBEComponentFunction *>(
		FindFunction(sFunctionName, FUNCTION_TEMPLATE));
	assert(m_pComponentFunction);
	// set the call parameters: this is simple, since we use the same names
	// and reference counts
	for_each(m_Parameters.begin(), m_Parameters.end(),
		SetCallVariableCall(m_pComponentFunction));

	// now prefix own parameters
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		vector<CBETypedDeclarator*>::iterator iter = m_Parameters.begin();
		for (; iter != m_Parameters.end(); iter++)
		{
			string sPrefix = pNF->GetWrapperVariablePrefix();
			CBEDeclarator *pDecl = (*iter)->m_Declarators.First();
			pDecl->SetName(sPrefix + pDecl->GetName());
		}
	}
}

/** \brief tests if this function should be written
 *  \return true if successful
 */
bool CBESwitchCase::DoWriteFunction(CBEFile* /*pFile*/)
{
	return true;
}

/** \brief write the start of the case statement
 *  \param pFile the file to write to
 *
 * if the second opcode const string is empty, then this is a simple case.
 * Otherwise its a range and we have to write an if statement.
 */
void CBESwitchCase::WriteCaseStart(CBEFile& pFile)
{
	if (m_sUpper.empty())
	{
		pFile << "\tcase " << m_sOpcode << ":\n";
		++pFile << "\t{\n";
	}
	else
	{
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sOpcodeVar = pNF->GetOpcodeVariable();

		pFile << "\tif (" << m_sOpcode << " <= " << sOpcodeVar << " && " <<
			sOpcodeVar << " < " << m_sUpper << ")\n";
		pFile << "\t{\n";
	}
	++pFile;
}

/** \brief write the end of the case statement
 *  \param pFile the file to write to
 */
void CBESwitchCase::WriteCaseEnd(CBEFile& pFile)
{
	--pFile << "\t}\n";
	if (m_sUpper.empty())
	{
		pFile << "\tbreak;\n";
		--pFile;
	}
}

/** \brief writes the target code
 *  \param pFile the target file
 */
void CBESwitchCase::Write(CBEFile& pFile)
{
	WriteCaseStart(pFile);

	WriteVariableDeclaration(pFile);
	WriteVariableInitialization(pFile, DIRECTION_IN);

	if (m_pUnmarshalFunction)
	{
		// unmarshal has void return code
		m_pUnmarshalFunction->WriteCall(pFile, string(), m_bSameClass);
	}

	// initialize parameters which depend on IN values
	WriteVariableInitialization(pFile, DIRECTION_OUT);

	if (m_pComponentFunction)
	{
		if (m_pTrace)
			m_pTrace->BeforeComponent(pFile, this);

		/* if this function has [allow_reply_only] attribute,
		 * it has an additional parameter (_dice_reply)
		 */
		CBETypedDeclarator *pReturn = GetReturnVariable();
		CBEDeclarator *pRetVar = (pReturn) ? pReturn->m_Declarators.First() : 0;
		m_pComponentFunction->WriteCall(pFile,
			pRetVar ? pRetVar->GetName() : string(), m_bSameClass);

		if (m_pTrace)
			m_pTrace->AfterComponent(pFile, this);
	}

	// write marshalling
	string sReply = CBENameFactory::Instance()->GetReplyCodeVariable();
	if (m_pMarshalFunction)
	{
		if (m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
		{
			pFile << "\tif (" << sReply << " == DICE_REPLY)\n";
			++pFile;
		}

		// check for exceptions
		if (m_pMarshalExceptionFunction)
		{
			CBEDeclarator *pEnv = GetEnvironment()->m_Declarators.First();
			string sEnv;
			if (pEnv->GetStars() == 0)
				sEnv = "&";
			sEnv += pEnv->GetName();
			pFile << "\tif (DICE_EXCEPTION_MAJOR(" << sEnv << ") == CORBA_USER_EXCEPTION)\n";
			pFile << "\t{\n";
			++pFile;

			m_pMarshalExceptionFunction->WriteCall(pFile, string(), m_bSameClass);

			--pFile << "\t}\n";
			pFile << "\telse\n";
			pFile << "\t{\n";
			++pFile;
		}

		m_pMarshalFunction->WriteCall(pFile, string(), m_bSameClass);

		if (m_pMarshalExceptionFunction)
			--pFile << "\t}\n";

		if (m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
			--pFile;
	}
	else if (m_Attributes.Find(ATTR_IN))
	{
		/* this is a send-only function */
		pFile << "\t" << sReply << " = DICE_NEVER_REPLY;\n";
	}

	WriteCleanup(pFile);

	WriteCaseEnd(pFile);
}

/** \brief writes the variable declaration inside the switch case
 *  \param pFile the file to write to
 *
 * local variable declaration inside a switch case include the variables,
 * which have to be unmarshaled from the message buffer and those which are
 * returned by the called function.
 *
 * For variables, which are pointers, we have to "allocate" a unreferenced
 * variable on the stack and then assign its address to the referenced
 * variable. E.g.
 * <code>
 * CORBA_long *t1, _t1;<br>
 * CORBA_long **t2, *_t2, __t2;<br>
 * t1 = \&_t1;<br>
 * _t2 = \&__t2;<br>
 * t2 = \&_t2;<br>
 * </code>
 *
 * We init the return variable already here, because if it is a struct, it has
 * to be initialized on declaration.
 */
void CBESwitchCase::WriteVariableDeclaration(CBEFile& pFile)
{
	// write local variable declaration
	CBEOperationFunction::WriteVariableDeclaration(pFile);
	// FIXME: use local variable for rest

	// write parameters
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		// skip "special" parameters
		if (!DoWriteVariable(*iter))
			continue;
		pFile << "\t";
		(*iter)->WriteIndirect(pFile);
		pFile << ";\n";
	}
}

/** \brief check if this is a special parameter
 *  \param pParameter the parameter to check
 *  \return true if this parameter should be used
 *
 * Here we filter out CORBA_Object, CORBA_Environment, and msg buffer.
 *
 * The easiest way to test is check if the parameter matches any of the
 * respective private pointers.
 */
bool CBESwitchCase::DoWriteVariable(CBETypedDeclarator *pParameter)
{
	if (pParameter == GetObject())
		return false;
	if (pParameter == GetEnvironment())
		return false;
	if (pParameter == GetMessageBuffer())
		return false;
	return true;
}

/** \brief initialize local variables
 *  \param pFile the file to write to
 *  \param nDirection the direction of the parameters to initailize
 *
 * This function takes care of the initialization of the indirect variables.
 */
void CBESwitchCase::WriteVariableInitialization(CBEFile& pFile, DIRECTION_TYPE nDirection)
{
	// initailize indirect variables
	vector<CBETypedDeclarator*>::iterator iter;
	// first simply do dereferences
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		if (!DoWriteVariable(*iter))
			continue;
		if (!(*iter)->IsDirection(nDirection))
			continue;
		if ((nDirection == DIRECTION_OUT) &&
			((*iter)->m_Attributes.Find(ATTR_IN)))
			continue;
		(*iter)->WriteIndirectInitialization(pFile, false);
	}
	// now initilize variables with dynamic memory
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		if (!DoWriteVariable(*iter))
			continue;
		if (!(*iter)->IsDirection(nDirection))
			continue;
		// only allocate memory for "pure" OUT parameter, because
		// IN's are referenced into the message buffer
		// FIXME: what about INOUT, where the OUT part is bigger than the IN
		// part?
		if ((*iter)->m_Attributes.Find(ATTR_IN))
			continue;
		(*iter)->WriteIndirectInitialization(pFile, true);
	}
}

/** \brief writes the clean up code
 *  \param pFile the file to write to
 *
 * We only need to cleanup OUT parameters, which are not IN,
 * because only for those parameters a memory allocation took
 * place.
 */
void CBESwitchCase::WriteCleanup(CBEFile& pFile)
{
	// cleanup indirect variables
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		if (!DoWriteVariable(*iter))
			continue;
		if (!(*iter)->IsDirection(DIRECTION_OUT))
			continue;
		if ((*iter)->m_Attributes.Find(ATTR_IN))
			continue;
		(*iter)->WriteCleanup(pFile, false);
	}
}

/** \brief writes the initialization of the variables
 */
void CBESwitchCase::WriteVariableInitialization(CBEFile& /*pFile*/)
{ }

/** \brief writes the invocation of the message transfer
 */
void CBESwitchCase::WriteInvocation(CBEFile& /*pFile*/)
{ }

/** \brief resets the message buffer type of the respective functions
 *
 * The unmarshal and reply functions need their message buffer parameter to be
 * casted when they are called. Tell them.
 */
void CBESwitchCase::SetMessageBufferType()
{
	if (m_pUnmarshalFunction)
		m_pUnmarshalFunction->SetMsgBufferCastOnCall(true);
	if (m_pMarshalFunction)
		m_pMarshalFunction->SetMsgBufferCastOnCall(true);
	if (m_pMarshalExceptionFunction)
		m_pMarshalExceptionFunction->SetMsgBufferCastOnCall(true);
}

/** \brief propagates the SetCallVariable call if the internal variables are \
 * Corba object and environment
 *  \param sOriginalName the internal name of the variable
 *  \param nStars the new number of stars of the variable
 *  \param sCallName the external name
 *
 * This method is called by the dispatch function to set the reply-code
 * variable according to its internal representation. This may also be used
 * for other local variables of the dispatch function. Because we use the
 * nested functions, we have to propagate the invocation respectively.
 */
void CBESwitchCase::SetCallVariable(std::string sOriginalName, int nStars, std::string sCallName)
{
	if (m_pUnmarshalFunction)
		m_pUnmarshalFunction->SetCallVariable(sOriginalName, nStars,
			sCallName);
	if (m_pMarshalFunction)
		m_pMarshalFunction->SetCallVariable(sOriginalName, nStars,
			sCallName);
	if (m_pMarshalExceptionFunction)
		m_pMarshalExceptionFunction->SetCallVariable(sOriginalName, nStars,
			sCallName);
	if (m_pComponentFunction)
		m_pComponentFunction->SetCallVariable(sOriginalName, nStars,
			sCallName);
}

/** \brief operator called when iterating parameters
 *  \param pParameter the parameter to st the call variable for
 */
void CBESwitchCase::SetCallVariableCall::operator() (CBETypedDeclarator *pParameter)
{
	/** instead of simply returning for one of the special parameters, we
	 * set the prefix to empty, so the function's SetCallVariable method
	 * gets called. Only if that happens the call-parameter list is
	 * created. If we have no parameter to prefix we still need that
	 * call-parameter list.
	 */
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sPrefix;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		// because the parameter is member of a different function than
		// the one given in the constructor, we have to check names
		// instead of pointers
		sPrefix = pNF->GetWrapperVariablePrefix();
		string sName = pParameter->m_Declarators.First()->GetName().c_str();
		if (f->GetObject()->m_Declarators.Find(sName))
			sPrefix.clear();
		if (f->GetEnvironment()->m_Declarators.Find(sName))
			sPrefix.clear();
		string sMsgBuf = pNF->GetMessageBufferVariable();
		if (pParameter->m_Declarators.Find(sMsgBuf))
			sPrefix.clear();
		string sReturn = pNF->GetReturnVariable();
		if (pParameter->m_Declarators.Find(sReturn))
			sPrefix.clear();
	}
	CBEDeclarator *pName = pParameter->m_Declarators.First();
	f->SetCallVariable(pName->GetName(), pName->GetStars(),
		sPrefix + pName->GetName());
}
