/**
 *    \file    dice/src/be/l4/L4BESrvLoopFunction.cpp
 *    \brief   contains the implementation of the class CL4BESrvLoopFunction
 *
 *    \date    02/10/2002
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

#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BESrvLoopFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BEOperationFunction.h"
#include "be/BEClass.h"
#include "be/BETypedDeclarator.h"
#include "be/BEType.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEWaitAnyFunction.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BESrvLoopFunction::CL4BESrvLoopFunction()
{
}

CL4BESrvLoopFunction::CL4BESrvLoopFunction(CL4BESrvLoopFunction & src)
:CBESrvLoopFunction(src)
{
}

/**    \brief destructor of target class */
CL4BESrvLoopFunction::~CL4BESrvLoopFunction()
{

}

/** \brief write the declaration of the CORBA_Object variable
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESrvLoopFunction::WriteCorbaObjectDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    if (m_pCorbaObject)
    {
        vector<CBEDeclarator*>::iterator iterCO = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pName = *iterCO;
        pFile->PrintIndent("CORBA_Object_base _%s;\n", pName->GetName().c_str());
        pFile->PrintIndent("");
        m_pCorbaObject->WriteDeclaration(pFile, pContext);
        pFile->Print(" = &_%s; // is client id\n", pName->GetName().c_str());
    }
}

/**    \brief writes the varaible initialization
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CL4BESrvLoopFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // call base class - initializes opcode
    CBESrvLoopFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // zero msg buffer
    if (pContext->IsOptionSet(PROGRAM_ZERO_MSGBUF))
        pMsgBuffer->WriteSetZero(pFile, pContext);
    // set the size dope here, so we do not need to set it anywhere else
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SIZE, 0, pContext);
    // init receive flexpage
    pMsgBuffer->WriteInitialization(pFile, TYPE_FLEXPAGE,
        GetReceiveDirection(), pContext);
    // init indirect strings
    pMsgBuffer->WriteInitialization(pFile, TYPE_REFSTRING,
        GetReceiveDirection(), pContext);
    // when we finished initialization, signal that we are ready
    WriteServerStartupInfo(pFile, pContext);
}

/**    \brief writes the server's startup notification
 *    \param pFile the file to write to
 *    \param pContext the context of this operation
 */
void CL4BESrvLoopFunction::WriteServerStartupInfo(CBEFile *pFile, CBEContext *pContext)
{
    // if test-suite send creation thread signal that we started
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        pFile->PrintIndent("l4thread_started(0);\n");
    }
}

/** \brief create this instance of a server loop function
 *  \param pFEInterface the interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if create was successful
 *
 * We have to check whether the server loop might receive flexpages. If it does, we have
 * to use a server function parameter (the Corba Environment) to set the receive flexpage.
 * To find out if we have receive flexpages, we use the message buffer, but we then have to
 * set the context option.
 */
bool CL4BESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!CBESrvLoopFunction::CreateBackEnd(pFEInterface, pContext))
        return false;
    // test for flexpages
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0) &&
        m_pCorbaEnv)
    {
        vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterCE;
        if (pDecl->GetStars() == 0)
            pDecl->IncStars(1);
        // set the call variables
        if (m_pWaitAnyFunction)
            m_pWaitAnyFunction->SetCallVariable(pDecl->GetName(),
                pDecl->GetStars(), pDecl->GetName(), pContext);
        if (m_pReplyAnyWaitAnyFunction)
            m_pReplyAnyWaitAnyFunction->SetCallVariable(pDecl->GetName(),
                pDecl->GetStars(), pDecl->GetName(), pContext);
    }
    return true;
}

/** \brief writes the parameters of the server loop
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameters
 *
 * This server loop will only have one parameter: a void pointer.
 */
void CL4BESrvLoopFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (bComma)
        pFile->Print(", ");
    string sServerParam = pContext->GetNameFactory()->GetServerParameterName();
    pFile->Print("void* %s", sServerParam.c_str());
}

/** \brief test if server loop parameter should be used as environment
 *  \param pContext the current context
 *  \return true if parameter should be used as CORBA_Environment
 */
bool CL4BESrvLoopFunction::DoUseParameterAsEnv(CBEContext * pContext)
{
    if (CBESrvLoopFunction::DoUseParameterAsEnv(pContext))
        return true;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0) &&
        m_pCorbaEnv)
        return true;
    return false;
}
