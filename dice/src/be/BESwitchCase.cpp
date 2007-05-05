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
#include "BETrace.h"
#include "Compiler.h"
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

CBESwitchCase::CBESwitchCase(CBESwitchCase & src)
: CBEOperationFunction(src)
{
    m_bSameClass = src.m_bSameClass;
    m_sOpcode = src.m_sOpcode;
    m_pUnmarshalFunction = src.m_pUnmarshalFunction;
    m_pMarshalFunction = src.m_pMarshalFunction;
    m_pMarshalExceptionFunction = src.m_pMarshalExceptionFunction;
    m_pComponentFunction = src.m_pComponentFunction;
}

/** \brief destructor of target class */
CBESwitchCase::~CBESwitchCase()
{
}

bool checkForInAttr(CFETypedDeclarator *pParameter)
{
    return pParameter->m_Attributes.Find(ATTR_IN) != 0;
}

class SetCallVariableCall {
    CBEFunction *f;
public:
    SetCallVariableCall(CBEFunction *ff) : f(ff) { }
    void operator() (CBETypedDeclarator *pParameter)
    {
	/* instead of simply returning for one of the special parameters, we
	 * set the prefix to empty, so the function's SetCallVariable method
	 * gets called. Only if that happens the call-parameter list is
	 * created. If we have no parameter to prefix we still need that
	 * call-parameter list.
	 */
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sPrefix;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
	    sPrefix = pNF->GetWrapperVariablePrefix();
	    if (pParameter == f->GetObject())
		sPrefix.clear();
	    if (pParameter == f->GetEnvironment())
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
};

/** \brief creates the back-end receive function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true is successful
 *
 * This implementation calls the base class' implementation and then sets the
 * name of the function.
 */
void
CBESwitchCase::CreateBackEnd(CFEOperation * pFEOperation)
{
    string exc = string(__func__);
    // we skip OUT functions
    if (pFEOperation->m_Attributes.Find(ATTR_OUT))
        return;
    // set name
    SetFunctionName(pFEOperation, FUNCTION_SWITCH_CASE);

    CBEOperationFunction::CreateBackEnd(pFEOperation);

    CreateTrace();

    // get opcode const
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_sOpcode = pNF->GetOpcodeConst(pFEOperation);

    // check if this switch case is from the same class as the surrounding
    // dispatcher function or if this is from a base class.
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->GetName() ==
	pFEOperation->GetSpecificParent<CFEInterface>()->GetName())
	m_bSameClass = true;
    else
	m_bSameClass = false;

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    string sFunctionName;

    // check if we need unmarshalling function
    vector<CFETypedDeclarator*>::iterator iterP = find_if(pFEOperation->m_Parameters.begin(),
	pFEOperation->m_Parameters.end(), checkForInAttr);
    if (iterP != pFEOperation->m_Parameters.end())
    {
        // create references to unmarshal function
        sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_UNMARSHAL);
        m_pUnmarshalFunction = static_cast<CBEUnmarshalFunction *>(
	    pRoot->FindFunction(sFunctionName, FUNCTION_UNMARSHAL));
        if (!m_pUnmarshalFunction)
        {
	    exc +=" failed, because unmarshal function (" +
		sFunctionName + ") could not be found.";
             throw new CBECreateException(exc);
        }
	// set the call parameters: this is simple, since we use the same
	// names and reference counts
	for_each(m_pUnmarshalFunction->m_Parameters.begin(),
	    m_pUnmarshalFunction->m_Parameters.end(),
	    SetCallVariableCall(m_pUnmarshalFunction));
    }
    // check if we need marshalling function
    // basically we alwas need marshalling, because we at least transfer
    // that there was no exception
    // the only exception is if the function is a oneway (IN) function
    if (!pFEOperation->m_Attributes.Find(ATTR_IN))
    {
        // create reference to marshal function
        sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL);
        m_pMarshalFunction = static_cast<CBEMarshalFunction*>(
	    pRoot->FindFunction(sFunctionName, FUNCTION_MARSHAL));
        if (!m_pMarshalFunction)
        {
	    exc += " failed, because marshal function (" + sFunctionName +
		") could not be found.";
            throw new CBECreateException(exc);
        }
        // set call parameters
	for_each(m_pMarshalFunction->m_Parameters.begin(),
	    m_pMarshalFunction->m_Parameters.end(),
	    SetCallVariableCall(m_pMarshalFunction));
    }
    if (!pFEOperation->m_RaisesDeclarators.empty())
    {
	sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_MARSHAL_EXCEPTION);
	m_pMarshalExceptionFunction = static_cast<CBEMarshalExceptionFunction*>(
	    pRoot->FindFunction(sFunctionName, FUNCTION_MARSHAL_EXCEPTION));
	// marshal_exc function has to be here, because we have raises
	// declarators
	if (!m_pMarshalExceptionFunction)
	{
	    exc += " failed, because marshal_exc function (" + sFunctionName +
		") could not be found.";
            throw new CBECreateException(exc);
	}
        // set call parameters
	for_each(m_pMarshalExceptionFunction->m_Parameters.begin(),
	    m_pMarshalExceptionFunction->m_Parameters.end(),
	    SetCallVariableCall(m_pMarshalExceptionFunction));
    }
    // create reference to component function
    sFunctionName = pNF->GetFunctionName(pFEOperation, FUNCTION_TEMPLATE);
    m_pComponentFunction = static_cast<CBEComponentFunction *>(
	pRoot->FindFunction(sFunctionName, FUNCTION_TEMPLATE));
    if (!m_pComponentFunction)
    {
	exc += " failed, because component function (" + sFunctionName +
	    ") could not be found.";
        throw new CBECreateException(exc);
    }
    // set the call parameters: this is simple, since we use the same names
    // and reference counts
    for_each(m_pComponentFunction->m_Parameters.begin(),
	m_pComponentFunction->m_Parameters.end(),
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

/** \brief writes the target code
 *  \param pFile the target file
 */
void CBESwitchCase::Write(CBEFile * pFile)
{
    *pFile << "\tcase " << m_sOpcode << ":\n";
    pFile->IncIndent();
    *pFile << "\t{\n";
    pFile->IncIndent();

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
	assert(m_pTrace);
	m_pTrace->BeforeComponent(pFile, this);
	
        /* if this function has [allow_reply_only] attribute,
         * it has an additional parameter (_dice_reply)
         */
        CBETypedDeclarator *pReturn = GetReturnVariable();
        CBEDeclarator *pRetVar = (pReturn) ? pReturn->m_Declarators.First() : 0;
        m_pComponentFunction->WriteCall(pFile, 
	    pRetVar ? pRetVar->GetName() : string(), m_bSameClass);

	m_pTrace->AfterComponent(pFile, this);
    }

    // write marshalling
    string sReply = CCompiler::GetNameFactory()->GetReplyCodeVariable();
    if (m_pMarshalFunction)
    {
        if (m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
        {
            *pFile << "\tif (" << sReply << " == DICE_REPLY)\n";
            pFile->IncIndent();
        }

	// check for exceptions
	if (m_pMarshalExceptionFunction)
	{
	    CBEDeclarator *pEnv = GetEnvironment()->m_Declarators.First();
	    string sEnv;
	    if (pEnv->GetStars() == 0)
		sEnv = "&";
	    sEnv += pEnv->GetName();
	    *pFile << "\tif (DICE_EXCEPTION_MAJOR(" << sEnv << ") == CORBA_USER_EXCEPTION)\n";
	    *pFile << "\t{\n";
	    pFile->IncIndent();

	    m_pMarshalExceptionFunction->WriteCall(pFile, string(), m_bSameClass);

	    pFile->DecIndent();
	    *pFile << "\t}\n";
	    *pFile << "\telse\n";
	    *pFile << "\t{\n";
	    pFile->IncIndent();
	}

        m_pMarshalFunction->WriteCall(pFile, string(), m_bSameClass);

	if (m_pMarshalExceptionFunction)
	{
	    pFile->DecIndent();
	    *pFile << "\t}\n";
	}

        if (m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
            pFile->DecIndent();
    }
    else if (m_Attributes.Find(ATTR_IN))
    {
        /* this is a send-only function */
        *pFile << "\t" << sReply << " = DICE_NEVER_REPLY;\n";
    }

    WriteCleanup(pFile);

    pFile->DecIndent();
    *pFile << "\t}\n";
    *pFile << "\tbreak;\n";
    pFile->DecIndent();
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
void
CBESwitchCase::WriteVariableDeclaration(CBEFile * pFile)
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
	*pFile << "\t";
        (*iter)->WriteIndirect(pFile);
	*pFile << ";\n";
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
bool
CBESwitchCase::DoWriteVariable(CBETypedDeclarator *pParameter)
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
void 
CBESwitchCase::WriteVariableInitialization(CBEFile * pFile, 
    DIRECTION_TYPE nDirection)
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
void CBESwitchCase::WriteCleanup(CBEFile * pFile)
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
}

