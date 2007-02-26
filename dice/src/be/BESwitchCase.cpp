/**
 *	\file	dice/src/be/BESwitchCase.cpp
 *	\brief	contains the implementation of the class CBESwitchCase
 *
 *	\date	01/19/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/BESwitchCase.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BETypedef.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEReplyWaitFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BERoot.h"

#include "fe/FEOperation.h"
#include "fe/FETypeSpec.h"
#include "fe/FETaggedStructType.h"

IMPLEMENT_DYNAMIC(CBESwitchCase);

CBESwitchCase::CBESwitchCase()
{
    m_pUnmarshalFunction = 0;
    m_pReplyWaitFunction = 0;
    m_pComponentFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBESwitchCase, CBEOperationFunction);
}

CBESwitchCase::CBESwitchCase(CBESwitchCase & src):CBEOperationFunction(src)
{
    m_sOpcode = src.m_sOpcode;
    m_pUnmarshalFunction = src.m_pUnmarshalFunction;
    m_pReplyWaitFunction = src.m_pReplyWaitFunction;
    m_pComponentFunction = src.m_pComponentFunction;
    IMPLEMENT_DYNAMIC_BASE(CBESwitchCase, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBESwitchCase::~CBESwitchCase()
{

}

/**	\brief creates the back-end receive function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true is successful
 *	
 * This implementation calls the base class' implementation and then sets the name
 * of the function.
 */
bool CBESwitchCase::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // set name
    pContext->SetFunctionType(FUNCTION_SWITCH_CASE);
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
    {
        VERBOSE("CBESwitchCase::CreateBE failed because base function could not be created\n");
        return false;
    }

    // get opcode const
    m_sOpcode = pContext->GetNameFactory()->GetOpcodeConst(pFEOperation, pContext);

    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    String sFunctionName;
    CBENameFactory *pNF = pContext->GetNameFactory();
    int nOldType = pContext->GetFunctionType();

    // check if we need unmarshalling function
    bool bNeedUnmarshalling = false;
    VectorElement *pIter = pFEOperation->GetFirstParameter();
    CFETypedDeclarator *pFEParameter;
    while (((pFEParameter = pFEOperation->GetNextParameter(pIter)) != 0)
           && !bNeedUnmarshalling)
    {
        if ((pFEParameter->FindAttribute(ATTR_IN)))
            bNeedUnmarshalling = true;
    }
    if (bNeedUnmarshalling)
    {
        // create references to unmarshal and reply function
        pContext->SetFunctionType(FUNCTION_UNMARSHAL);
        sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
        m_pUnmarshalFunction = (CBEUnmarshalFunction *) pRoot->FindFunction(sFunctionName);
        if (!m_pUnmarshalFunction)
        {
             m_pUnmarshalFunction = 0;
             VERBOSE("CBESwitchCase::CreateBE failed because unmarshal function could not be found\n");
             return false;
        }
        // set the call parameters: this is simple, since we use the same names and reference counts
        VectorElement *pIter = m_pUnmarshalFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = m_pUnmarshalFunction->GetNextParameter(pIter)) != 0)
        {
            VectorElement *pIterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pName = pParameter->GetNextDeclarator(pIterD);
            m_pUnmarshalFunction->SetCallVariable(pName->GetName(), pName->GetStars(), pName->GetName(), pContext);
        }
        // set the reference count for the  corba object and environment
// FIXME
    }
    // reply or wait-any function
    if (pFEOperation->FindAttribute(ATTR_IN))
    {
        // use wait any
        pContext->SetFunctionType(FUNCTION_WAIT_ANY);
        sFunctionName = pNF->GetFunctionName(pFEOperation->GetParentInterface(), pContext);
        m_pReplyWaitFunction = pRoot->FindFunction(sFunctionName);
        if (!m_pReplyWaitFunction)
        {
            m_pReplyWaitFunction = 0;
            VERBOSE("CBESwitchCase::CreateBE failed because reply-wait (IN) function could not be created\n");
            return false;
        }
        // set the call parameters: this is simple, since we use the same names and reference counts
        VectorElement *pIter = m_pReplyWaitFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = m_pReplyWaitFunction->GetNextParameter(pIter)) != 0)
        {
            VectorElement *pIterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pName = pParameter->GetNextDeclarator(pIterD);
            m_pReplyWaitFunction->SetCallVariable(pName->GetName(), pName->GetStars(), pName->GetName(), pContext);
        }
        // set the reference count for the  corba object and environment
// FIXME
    }
    else
    {
        // use reply-wait
        pContext->SetFunctionType(FUNCTION_REPLY_WAIT);
        sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
        m_pReplyWaitFunction = pRoot->FindFunction(sFunctionName);
        if (!m_pReplyWaitFunction)
        {
            m_pReplyWaitFunction = 0;
            VERBOSE("CBESwitchCase::CreateBE failed because reply-wait (OUT) function could not be found\n");
            return false;
        }
        // set the call parameters: this is simple, since we use the same names and reference counts
        VectorElement *pIter = m_pReplyWaitFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = m_pReplyWaitFunction->GetNextParameter(pIter)) != 0)
        {
            VectorElement *pIterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pName = pParameter->GetNextDeclarator(pIterD);
            m_pReplyWaitFunction->SetCallVariable(pName->GetName(), pName->GetStars(), pName->GetName(), pContext);
        }
        // set the reference count for the  corba object and environment
// FIXME
    }

    // create reference to component function
    pContext->SetFunctionType(FUNCTION_SKELETON);
    sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
    m_pComponentFunction = (CBEComponentFunction *) pRoot->FindFunction(sFunctionName);
    if (!m_pComponentFunction)
    {
        m_pComponentFunction = 0;
        VERBOSE("CBESwitchCase::CreateBE failed because component function could not be found\n");
        return false;
    }
    // set the call parameters: this is simple, since we use the same names and reference counts
    pIter = m_pComponentFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pComponentFunction->GetNextParameter(pIter)) != 0)
    {
        VectorElement *pIterD = pParameter->GetFirstDeclarator();
        CBEDeclarator *pName = pParameter->GetNextDeclarator(pIterD);
        m_pComponentFunction->SetCallVariable(pName->GetName(), pName->GetStars(), pName->GetName(), pContext);
    }
    // set the reference count for the  corba object and environment
// FIXME

    pContext->SetFunctionType(nOldType);
    return true;
}

/**	\brief writes the target code
 *	\param pFile the target file
 *	\param pContext the context of the write operation
 */
void CBESwitchCase::Write(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("case %s:\n", (const char *) m_sOpcode);
    pFile->IncIndent();
    pFile->PrintIndent("{\n");
    pFile->IncIndent();

    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);

    if (m_pUnmarshalFunction)
    {
        // unmarshal has void return code
        pFile->PrintIndent("");
        m_pUnmarshalFunction->WriteCall(pFile, String(), pContext);
        pFile->Print("\n");
    }

    if (m_pComponentFunction)
    {
        VectorElement *pIter = m_pReturnVar->GetFirstDeclarator();
        CBEDeclarator *pRetVar = m_pReturnVar->GetNextDeclarator(pIter);
        pFile->PrintIndent("");
        m_pComponentFunction->WriteCall(pFile, pRetVar->GetName(), pContext);
        pFile->Print("\n");
    }

    // write reply or wait-any
    if (m_pReplyWaitFunction)
    {
        String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
        pFile->PrintIndent("");
        m_pReplyWaitFunction->WriteCall(pFile, sOpcodeVar, pContext);
        pFile->Print("\n");
    }

    WriteCleanup(pFile, pContext);

    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
}

/**	\brief writes the variable declaration inside the switch case
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * local variable declaration inside a switch case include the variables, which have to be
 * unmarshaled from the message buffer and those which are returned by the called function.
 *
 * For variables, which are pointers, we have to "allocate" a unreferenced variable on the
 * stack and then assign its address to the referenced variable. E.g.
 * <code>
 * CORBA_long *t1, _t1;<br>
 * CORBA_long **t2, *_t2, __t2;<br>
 * t1 = \&_t1;<br>
 * _t2 = \&__t2;<br>
 * t2 = \&_t2;<br>
 * </code>
 *
 * We init the return variable already here, because if it is a struct, it has to
 * be initialized on declaration.
 */
void CBESwitchCase::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // declare return variable if necessary
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);

    // write parameters
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        pFile->PrintIndent("");
        pParam->WriteIndirect(pFile, pContext);
        pFile->Print(";\n");
    }
}

/**	\brief initialize local variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This function takes care of the initialization of the indirect variables.
 */
void CBESwitchCase::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // initailize indirect variables
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        pParam->WriteIndirectInitialization(pFile, pContext);
    }
}

/**	\brief writes the clean up code
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBESwitchCase::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/** \brief resets the message buffer type of the respective functions
 *  \param pContext the context of this operation
 *
 * The unmarshal and reply functions need their message buffer parameter to be
 * casted when they are called. Tell them.
 */
void CBESwitchCase::SetMessageBufferType(CBEContext *pContext)
{
    if (m_pUnmarshalFunction)
        m_pUnmarshalFunction->SetMsgBufferCastOnCall(true);
    if (m_pReplyWaitFunction)
        m_pReplyWaitFunction->SetMsgBufferCastOnCall(true);
}

/** \brief propagates the SetCallVariable call if the internal variables are Corba object and environment
 *  \param sOriginalName the internal name of the variable
 *  \param nStars the new number of stars of the variable
 *  \param sCallName the external name
 *  \param pContext the context of this operation
 *
 * Since we reference the unmarshal, wait and component functions, we have to propagate
 * this call, so they initialize their arguments repectively.
 */
void CBESwitchCase::SetCallVariable(String sOriginalName, int nStars, String sCallName, CBEContext * pContext)
{
    if (m_pUnmarshalFunction)
        m_pUnmarshalFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
    if (m_pReplyWaitFunction)
        m_pReplyWaitFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
    if (m_pComponentFunction)
        m_pComponentFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
}
