/**
 *    \file    dice/src/be/BESwitchCase.cpp
 *    \brief   contains the implementation of the class CBESwitchCase
 *
 *    \date    01/19/2002
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

#include "be/BESwitchCase.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BETypedef.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEMarshalFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BERoot.h"

#include "fe/FEOperation.h"
#include "TypeSpec-Type.h"
#include "fe/FETaggedStructType.h"

CBESwitchCase::CBESwitchCase()
{
    m_pUnmarshalFunction = 0;
    m_pMarshalFunction = 0;
    m_pComponentFunction = 0;
}

CBESwitchCase::CBESwitchCase(CBESwitchCase & src):CBEOperationFunction(src)
{
    m_sOpcode = src.m_sOpcode;
    m_pUnmarshalFunction = src.m_pUnmarshalFunction;
    m_pMarshalFunction = src.m_pMarshalFunction;
    m_pComponentFunction = src.m_pComponentFunction;
}

/**    \brief destructor of target class */
CBESwitchCase::~CBESwitchCase()
{

}

/**    \brief creates the back-end receive function
 *    \param pFEOperation the corresponding front-end operation
 *    \param pContext the context of the code generation
 *    \return true is successful
 *
 * This implementation calls the base class' implementation and then sets the name
 * of the function.
 */
bool CBESwitchCase::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // we skip OUT functions
    if (pFEOperation->FindAttribute(ATTR_OUT))
    {
        VERBOSE("CBESwitchCase::CreateBackEnd failed because it has been called for an OUT function (%s)\n", pFEOperation->GetName().c_str());
        return true;
    }
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

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    string sFunctionName;
    CBENameFactory *pNF = pContext->GetNameFactory();
    int nOldType = pContext->GetFunctionType();

    // check if we need unmarshalling function
    bool bNeedUnmarshalling = false;
    vector<CFETypedDeclarator*>::iterator iterP = pFEOperation->GetFirstParameter();
    CFETypedDeclarator *pFEParameter;
    while (((pFEParameter = pFEOperation->GetNextParameter(iterP)) != 0)
           && !bNeedUnmarshalling)
    {
        if (pFEParameter->FindAttribute(ATTR_IN))
            bNeedUnmarshalling = true;
    }
    if (bNeedUnmarshalling)
    {
        // create references to unmarshal function
        pContext->SetFunctionType(FUNCTION_UNMARSHAL);
        sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
        m_pUnmarshalFunction = (CBEUnmarshalFunction *) pRoot->FindFunction(sFunctionName);
        if (!m_pUnmarshalFunction)
        {
             VERBOSE("CBESwitchCase::CreateBE failed because unmarshal function (%s) could not be found\n", sFunctionName.c_str());
             return false;
        }
        // set the call parameters: this is simple, since we use the same names and reference counts
        vector<CBETypedDeclarator*>::iterator iter = m_pUnmarshalFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = m_pUnmarshalFunction->GetNextParameter(iter)) != 0)
        {
            vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pName = *iterD;
            m_pUnmarshalFunction->SetCallVariable(pName->GetName(),
                pName->GetStars(), pName->GetName(), pContext);
        }
    }
    // check if we need marshalling function
    // basically we alwas need marshalling, because we at least transfer
    // that there was no exception
    // the only exception is if the function is a oneway (IN) function
    if (!pFEOperation->FindAttribute(ATTR_IN))
    {
        // create reference to marshal function
        pContext->SetFunctionType(FUNCTION_MARSHAL);
        sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
        m_pMarshalFunction = (CBEMarshalFunction*) pRoot->FindFunction(sFunctionName);
        if (!m_pMarshalFunction)
        {
            VERBOSE("CBESwitchCase::CreateBE failed because marshal function (%s) could not be found\n", sFunctionName.c_str());
            return false;
        }
        // set call parameters
        vector<CBETypedDeclarator*>::iterator iter = m_pMarshalFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = m_pMarshalFunction->GetNextParameter(iter)) != 0)
        {
            vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pName = *iterD;
            m_pMarshalFunction->SetCallVariable(pName->GetName(),
                pName->GetStars(), pName->GetName(), pContext);
        }
    }
    // create reference to component function
    pContext->SetFunctionType(FUNCTION_TEMPLATE);
    sFunctionName = pNF->GetFunctionName(pFEOperation, pContext);
    m_pComponentFunction = (CBEComponentFunction *) pRoot->FindFunction(sFunctionName);
    if (!m_pComponentFunction)
    {
        m_pComponentFunction = 0;
        VERBOSE("CBESwitchCase::CreateBE failed because component function (%s) could not be found\n", sFunctionName.c_str());
        return false;
    }
    // set the call parameters: this is simple, since we use the same names and reference counts
    vector<CBETypedDeclarator*>::iterator iterBP = m_pComponentFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pComponentFunction->GetNextParameter(iterBP)) != 0)
    {
        vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
        CBEDeclarator *pName = *iterD;
        m_pComponentFunction->SetCallVariable(pName->GetName(),
            pName->GetStars(), pName->GetName(), pContext);
    }

    pContext->SetFunctionType(nOldType);
    return true;
}

/**    \brief writes the target code
 *    \param pFile the target file
 *    \param pContext the context of the write operation
 */
void CBESwitchCase::Write(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("case %s:\n", m_sOpcode.c_str());
    pFile->IncIndent();
    pFile->PrintIndent("{\n");
    pFile->IncIndent();

    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, DIRECTION_IN, pContext);

    if (m_pUnmarshalFunction)
    {
        // unmarshal has void return code
        pFile->PrintIndent("");
        m_pUnmarshalFunction->WriteCall(pFile, string(), pContext);
        pFile->Print("\n");
    }

    // initialize parameters which depend on IN values
    WriteVariableInitialization(pFile, DIRECTION_OUT, pContext);

    if (m_pComponentFunction)
    {
        /* if this function has [allow_reply_only] attribute,
         * it has an additional parameter (_dice_reply)
         */
        vector<CBEDeclarator*>::iterator iterRet = m_pReturnVar->GetFirstDeclarator();
        CBEDeclarator *pRetVar = *iterRet;
        pFile->PrintIndent("");
        m_pComponentFunction->WriteCall(pFile, pRetVar->GetName(), pContext);
        pFile->Print("\n");
    }

    // write marshalling
    string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    if (m_pMarshalFunction)
    {
        if (FindAttribute(ATTR_ALLOW_REPLY_ONLY))
        {
            *pFile << "\tif (" << sReply << " != DICE_NO_REPLY)\n";
            pFile->IncIndent();
        }

        pFile->PrintIndent("");
        m_pMarshalFunction->WriteCall(pFile, string(), pContext);
        pFile->Print("\n");

        if (FindAttribute(ATTR_ALLOW_REPLY_ONLY))
            pFile->DecIndent();
    }
    else if (FindAttribute(ATTR_IN))
    {
        /* this is a send-only function */
        *pFile << "\t" << sReply << " = DICE_NO_REPLY;\n";
    }

    WriteCleanup(pFile, pContext);

    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
}

/**    \brief writes the variable declaration inside the switch case
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
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
    vector<CBETypedDeclarator*>::iterator iter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = GetNextParameter(iter)) != 0)
    {
        pFile->PrintIndent("");
        pParam->WriteIndirect(pFile, pContext);
        pFile->Print(";\n");
    }
}

/**    \brief initialize local variables
 *    \param pFile the file to write to
 *  \param nDirection the direction of the parameters to initailize
 *    \param pContext the context of the write operation
 *
 * This function takes care of the initialization of the indirect variables.
 */
void CBESwitchCase::WriteVariableInitialization(CBEFile * pFile, int nDirection, CBEContext * pContext)
{
    // initailize indirect variables
    vector<CBETypedDeclarator*>::iterator iter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    // first simply do dereferences
    while ((pParameter = GetNextParameter(iter)) != 0)
    {
        if (!pParameter->IsDirection(nDirection))
            continue;
        if ((nDirection == DIRECTION_OUT) &&
            (pParameter->FindAttribute(ATTR_IN)))
            continue;
        pParameter->WriteIndirectInitialization(pFile, pContext);
    }
    // now initilize variables with dynamic memory
    iter = GetFirstParameter();
    while ((pParameter = GetNextParameter(iter)) != 0)
    {
        if (!pParameter->IsDirection(nDirection))
            continue;
        // only allocate memory for "pure" OUT parameter, because
        // IN's are referenced into the message buffer
        // FIXME: what about INOUT, where the OUT part is bigger than the IN part?
        if (pParameter->FindAttribute(ATTR_IN))
            continue;
        pParameter->WriteIndirectInitializationMemory(pFile, pContext);
    }
}

/**    \brief writes the clean up code
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * We only need to cleanup OUT parameters, which are not IN,
 * because only for those parameters a memory allocation took
 * place.
 */
void CBESwitchCase::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    // cleanup indirect variables
    vector<CBETypedDeclarator*>::iterator iter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(iter)) != 0)
    {
        if (!pParameter->IsDirection(DIRECTION_OUT))
            continue;
        if (pParameter->FindAttribute(ATTR_IN))
            continue;
        pParameter->WriteCleanup(pFile, pContext);
    }
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
    if (m_pMarshalFunction)
        m_pMarshalFunction->SetMsgBufferCastOnCall(true);
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
void CBESwitchCase::SetCallVariable(string sOriginalName, int nStars, string sCallName, CBEContext * pContext)
{
    if (m_pUnmarshalFunction)
        m_pUnmarshalFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
    if (m_pMarshalFunction)
        m_pMarshalFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
    if (m_pComponentFunction)
        m_pComponentFunction->SetCallVariable(sOriginalName, nStars, sCallName, pContext);
}
