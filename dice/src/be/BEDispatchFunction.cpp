/**
 *    \file    dice/src/be/BEDispatchFunction.cpp
 *    \brief   contains the implementation of the class CBEDispatchFunction
 *
 *    \date    10/10/2003
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
#include "be/BEDispatchFunction.h"
#include "be/BESwitchCase.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBufferType.h"
#include "be/BEOpcodeType.h"
#include "be/BEReplyCodeType.h"
#include "be/BEMarshaller.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEComponent.h"
#include "be/BEUserDefinedType.h"

#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"
#include "fe/FEOperation.h"

CBEDispatchFunction::CBEDispatchFunction()
{
}

CBEDispatchFunction::CBEDispatchFunction(CBEDispatchFunction & src)
: CBEInterfaceFunction(src)
{
    vector<CBESwitchCase*>::iterator iter = src.m_vSwitchCases.begin();
    for(; iter != src.m_vSwitchCases.end(); iter++)
    {
        CBESwitchCase *pNew = (CBESwitchCase*)((*iter)->Clone());
        m_vSwitchCases.push_back(pNew);
        pNew->SetParent(this);
    }
    m_sDefaultFunction = src.m_sDefaultFunction;
}

/**    \brief destructor of target class */
CBEDispatchFunction::~CBEDispatchFunction()
{
    while (!m_vSwitchCases.empty())
    {
        delete m_vSwitchCases.back();
        m_vSwitchCases.pop_back();
    }
}

/**    \brief creates the dispatch function for the given interface
 *    \param pFEInterface the respective front-end interface
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * The dispatch function return the reply code which determines if the server
 * loop should return a reply to the client or not.
 *
 * After we created the switch cases, we force them to reset the message buffer type of their functions.
 * This way, we use in all functions the message buffer type of this server loop.
 */
bool CBEDispatchFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_DISPATCH);
    // set target file name
    SetTargetFileName(pFEInterface, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);

    if (!CBEInterfaceFunction::CreateBackEnd(pFEInterface, pContext))
        return false;
    // set source line number to last number of interface
    SetSourceLine(pFEInterface->GetSourceLineEnd());

    // add functions
    if (!AddSwitchCases(pFEInterface, pContext))
        return false;

    // set own message buffer
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    // return type -> set to IPC reply code
    CBEReplyCodeType *pReplyType = pContext->GetClassFactory()->GetNewReplyCodeType();
    if (!pReplyType->CreateBackEnd(pContext))
    {
        delete pReplyType;
        VERBOSE("CBEDispatchFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    if (!SetReturnVar(pReplyType, sReply, pContext))
    {
        delete pReplyType;
        VERBOSE("CBEDispatchFunction::CreateBE failed because return var could not be set\n");
        return false;
    }

    // set message buffer type
    vector<CBESwitchCase*>::iterator iterS = GetFirstSwitchCase();
    CBESwitchCase *pSwitchCase;
    while ((pSwitchCase = GetNextSwitchCase(iterS)) != 0)
    {
        pSwitchCase->SetMessageBufferType(pContext);
    }

    // check if interface has default function and add its name if available
    if (pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION))
    {
        CFEStringAttribute *pDefaultFunc = ((CFEStringAttribute*)(pFEInterface->FindAttribute(ATTR_DEFAULT_FUNCTION)));
        m_sDefaultFunction = pDefaultFunc->GetString();
    }

    // add message buffer and environment as parameters

    return true;
}

/** \brief adds the message buffer parameter
 *  \param pFEInterface the respective front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if the creation was successful
 */
bool CBEDispatchFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    assert(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    assert(pMsgBuffer);
    // msg buffer has to be initialized with correct values,
    pMsgBuffer->InitCounts(pClass, pContext);
    // create own message buffer
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType(true);
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pMsgBuffer, pContext))
    {
        delete m_pMsgBuffer;
        m_pMsgBuffer = 0;
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/**    \brief adds the functions for the given front-end interface
 *    \param pFEInterface the interface to add the functions for
 *    \param pContext the context of the code generation
 */
bool CBEDispatchFunction::AddSwitchCases(CFEInterface * pFEInterface,
    CBEContext * pContext)
{
    if (!pFEInterface)
        return true;

    vector<CFEOperation*>::iterator iterO = pFEInterface->GetFirstOperation();
    CFEOperation *pFEOperation;
    while ((pFEOperation = pFEInterface->GetNextOperation(iterO)) != 0)
    {
        // skip OUT functions
        if (pFEOperation->FindAttribute(ATTR_OUT))
            continue;
        CBESwitchCase *pFunction = pContext->GetClassFactory()->GetNewSwitchCase();
        AddSwitchCase(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveSwitchCase(pFunction);
            delete pFunction;
            return false;
        }
    }

    vector<CFEInterface*>::iterator iterI = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pFEBase;
    while ((pFEBase = pFEInterface->GetNextBaseInterface(iterI)) != 0)
    {
        if (!AddSwitchCases(pFEBase, pContext))
            return false;
    }

    return true;
}

/**    \brief adds a new function to the functions vector
 *    \param pFunction the function to add
 */
void CBEDispatchFunction::AddSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    m_vSwitchCases.push_back(pFunction);
    pFunction->SetParent(this);
}

/**    \brief removes a function from the functions vector
 *    \param pFunction the function to remove
 */
void CBEDispatchFunction::RemoveSwitchCase(CBESwitchCase * pFunction)
{
    if (!pFunction)
        return;
    vector<CBESwitchCase*>::iterator iter;
    for (iter = m_vSwitchCases.begin(); iter != m_vSwitchCases.end(); iter++)
    {
        if (*iter == pFunction)
        {
            m_vSwitchCases.erase(iter);
            return;
        }
    }
}

/**    \brief retrieves a pointer to the first function
 *    \return a pointer to the first function
 */
vector<CBESwitchCase*>::iterator CBEDispatchFunction::GetFirstSwitchCase()
{
    return m_vSwitchCases.begin();
}

/**    \brief retrieves reference to the next function
 *    \param iter the pointer to the next function
 *    \return a reference to the next function
 */
CBESwitchCase *CBEDispatchFunction::GetNextSwitchCase(vector<CBESwitchCase*>::iterator &iter)
{
    if (iter == m_vSwitchCases.end())
        return 0;
    return *iter++;
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEDispatchFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    // write opcode variable
    CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
    if (!pOpcodeType->CreateBackEnd(pContext))
    {
        delete pOpcodeType;
        return;
    }
    pOpcodeType->Write(pFile, pContext);
    string sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->Print(" %s,\n", sOpcodeVar.c_str());
    pFile->PrintIndent("");
    delete pOpcodeType;

    CBEMsgBufferType* pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    WriteParameter(pFile, pMsgBuffer, pContext, false /* no const msgbuf */);
    // writes environment parameter
    CBEInterfaceFunction::WriteAfterParameters(pFile, pContext, true);
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEDispatchFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (bComma)
        *pFile << ",\n\t";
    // write opcode variable
    string sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->Print(" %s,\n", sOpcodeVar.c_str());
    pFile->PrintIndent("");

    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->HasReference())
    {
        /* this should be unset if the message buffer type stays the
         * same for derived interfaces (e.g. char[] for socket Backend)
         */
        if (m_bCastMsgBufferOnCall)
            pMsgBuffer->GetType()->WriteCast(pFile, true, pContext);
        pFile->Print("&");
    }
    WriteCallParameter(pFile, pMsgBuffer, pContext);
    // writes environment parameter
    CBEInterfaceFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Variables declared for the dispatch include:
 * - the opcode
 */
void CBEDispatchFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // declare local exception variable
    // if we do handle opcode exception
    if (m_sDefaultFunction.empty())
        WriteExceptionWordDeclaration(pFile, false, pContext);

    // reply code
    m_pReturnVar->WriteInitDeclaration(pFile, string("DICE_REPLY"), pContext);
}

/**    \brief writes the variable initializations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * don't do anything (no variables to initialize)
 */
void CBEDispatchFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/**    \brief writes the switch statement
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteSwitch(CBEFile * pFile, CBEContext * pContext)
{
    string sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);

    pFile->PrintIndent("switch (%s)\n", sOpcodeVar.c_str());
    pFile->PrintIndent("{\n");

    // iterate over functions
    assert(m_pClass);
    vector<CBESwitchCase*>::iterator iter = GetFirstSwitchCase();
    CBESwitchCase *pFunction;
    while ((pFunction = GetNextSwitchCase(iter)) != 0)
    {
        pFunction->Write(pFile, pContext);
    }

    // writes default case
    WriteDefaultCase(pFile, pContext);

    pFile->PrintIndent("}\n");
}

/** \brief writes the default case of the switch statetment
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The default case is usually empty. It is only used if the opcode does not match any
 * of the defined opcodes. Because the switch statement expects a valid opcode after the
 * function returns, it hastohave the format:
 * &lt;opcode type&gt; &lt;name&gt;(&lt;corba object&gt;*, &lt;msgbuffer type&gt;*, &lt;corba environment&gt;*)
 *
 * An alternative is to call the wait-any function.
 */
void CBEDispatchFunction::WriteDefaultCase(CBEFile *pFile, CBEContext *pContext)
{
    pFile->PrintIndent("default:\n");
    pFile->IncIndent();
    if (m_sDefaultFunction.empty())
        WriteDefaultCaseWithoutDefaultFunc(pFile, pContext);
    else
        WriteDefaultCaseWithDefaultFunc(pFile, pContext);
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
}

/** \brief writes the code in the default case if the default function is not available
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(CBEFile *pFile, CBEContext *pContext)
{
    string sOpcode = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    pFile->PrintIndent("/* unknown opcode */\n");
    WriteSetWrongOpcodeException(pFile, pContext);
    // send reply
    string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    pFile->PrintIndent("%s = DICE_REPLY;\n", sReply.c_str());
}

/** \brief writes the code in the default case if the default function is available
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteDefaultCaseWithDefaultFunc(CBEFile *pFile, CBEContext *pContext)
{
    string sOpcode = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    string sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    string sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);

    pFile->PrintIndent("return %s(%s, %s, %s);\n", m_sDefaultFunction.c_str(),
                    sObj.c_str(), sMsgBuffer.c_str(), sEnv.c_str());
}

/** \brief writes function declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation adds the decalartion of the default function if defined and used. And
 * it has to declare the reply-any-wait-any function
 */
void CBEDispatchFunction::WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // call base
    CBEInterfaceFunction::WriteFunctionDeclaration(pFile, pContext);
    // add declaration of default function
    pFile->Print("\n");
    if (!m_sDefaultFunction.empty())
    {
        CBEMsgBufferType *pMsgBuffer = m_pClass->GetMessageBuffer();
        string sMsgBufferType = pMsgBuffer->GetAlias()->GetName();
        // int &lt;name&gt;(&lt;corba object&gt;, &lt;msg buffer type&gt;*, &lt;corba environment&gt;*)
        pFile->Print("CORBA_int %s(CORBA_Object, %s*, CORBA_Server_Environment*);\n", m_sDefaultFunction.c_str(),
                     sMsgBufferType.c_str());
    }
}

/** \brief writes the code to set and marshal the exception codes for a wrong opcode
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteSetWrongOpcodeException(CBEFile* pFile, CBEContext* pContext)
{
    // set the exception in the environment
    string sSetFunc;
    if (((CBEUserDefinedType*)m_pCorbaEnv->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pEnv = *iterCE;
    *pFile << "\t" << sSetFunc << "(";
    if (pEnv->GetStars() == 0)
        pFile->Print("&");
    pEnv->WriteName(pFile, pContext);
    pFile->Print(",\n");
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
    pFile->PrintIndent("CORBA_DICE_EXCEPTION_WRONG_OPCODE,\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
    // copy from environment to local exception variable
    WriteExceptionWordInitialization(pFile, pContext);
    // marshal exception variable
    bool bUseConstOffset = true;
    WriteMarshalException(pFile, 0, bUseConstOffset, pContext);
}

/** \brief test fi this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBEDispatchFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return dynamic_cast<CBEComponent*>(pFile->GetTarget());
}

/** \brief test fi this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return  true if this function should be written
 *
 * A server loop is only written at the component's side.
 */
bool CBEDispatchFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    // do not write this function implementation if
    // option NO_DISPATCHER is set
    if (pContext->IsOptionSet(PROGRAM_NO_DISPATCHER))
        return false;
    return dynamic_cast<CBEComponent*>(pFile->GetTarget());
}

/**    \brief writes the invocation of the message transfer
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
}

/**    \brief clean up the mess
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/**    \brief writes the server loop's function body
 *    \param pFile the target file
 *    \param pContext the context of the write operation
 */
void CBEDispatchFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
    // write variable declaration and initialization
    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);
    // write loop (contains switch)
    WriteSwitch(pFile, pContext);
    // write clean up
    WriteCleanup(pFile, pContext);
    // write return
    WriteReturn(pFile, pContext);
}
