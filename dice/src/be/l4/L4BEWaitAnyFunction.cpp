/**
 *    \file    dice/src/be/l4/L4BEWaitAnyFunction.cpp
 *    \brief   contains the implementation of the class CL4BEWaitAnyFunction
 *
 *    \date    03/07/2002
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

#include "L4BEWaitAnyFunction.h"
#include "L4BENameFactory.h"
#include "L4BEClassFactory.h"
#include "L4BESizes.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOperationFunction.h"
#include "L4BEMsgBufferType.h"
#include "L4BEIPC.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEWaitAnyFunction::CL4BEWaitAnyFunction(bool bOpenWait, bool bReply)
: CBEWaitAnyFunction(bOpenWait, bReply)
{
}

CL4BEWaitAnyFunction::CL4BEWaitAnyFunction(CL4BEWaitAnyFunction & src)
: CBEWaitAnyFunction(src)
{
}

/**    \brief destructor of target class */
CL4BEWaitAnyFunction::~CL4BEWaitAnyFunction()
{

}

/** \brief writes the variable declarations of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The variable declarations of the wait-any function only contains so-called
 * helper variables. This is the result variable.
 */
void
CL4BEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile,
    CBEContext * pContext)
{
    // first call base class
    CBEWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    string sResult =
        pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    *pFile << "\tl4_msgdope_t " << sResult << " = { msgdope: 0 };\n";

    // write loop variable for msg buffer dump
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
        *pFile << "\tint _i;\n";
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * For reply only:
 * We do not initialize the receive indirect strings, because we assume that
 * they have been initialized by the server loop. After that the buffer is
 * handed to the server function. If they intend to use it after the component
 * function is left, they have to copy it.
 *
 * The receive flexpage is reinitialized, because it might have changed.
 */
void
CL4BEWaitAnyFunction::WriteVariableInitialization(CBEFile * pFile,
    CBEContext * pContext)
{
    // call base class
    CBEWaitAnyFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer && m_bReply)
    {
        // init receive flexpage
        pMsgBuffer->WriteInitialization(pFile, TYPE_FLEXPAGE,
            GetReceiveDirection(), pContext);
    }
}

/** \brief writes the invocation call to thetarget file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The wait any function simply waits for any message and unmarshals the opcode.
 * Since the message buffer is a referenced parameter, we know for sure, that
 * the "buffer" is a pointer.
 */
void CL4BEWaitAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // init size and send dopes
    if (!m_bReply)
        // do not overwrite message buffer if reply is sent
        pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, 0, pContext);

    // invocate
    WriteIPC(pFile, pContext);
    WriteExceptionCheck(pFile, pContext); // reset exception
    WriteIPCErrorCheck(pFile, pContext); // set IPC exception

    if (m_bReply)
        WriteReleaseMemory(pFile, pContext);

    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
    {
        string sResult =
            pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pMsgBuffer->WriteDump(pFile, sResult, pContext);
    }
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEWaitAnyFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    if (m_bOpenWait)
    {
        if (m_bReply)
            WriteIPCReplyWait(pFile, pContext);
        else
            m_pComm->WriteWait(pFile, this, pContext);
    }
    else
        m_pComm->WriteReceive(pFile, this, pContext);
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void
CL4BEWaitAnyFunction::WriteIPCReplyWait(CBEFile *pFile,
    CBEContext *pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER))
    {
        string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD,
            false, pContext, 0);
        string sFunc = pContext->GetTraceServerFunc();
        if (sFunc.empty())
            sFunc = string("printf");
        pFile->PrintIndent("%s(\"reply (dw0=%%x, dw1=%%x)%s\", ",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[0])))), ");
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[4]))))");
        pFile->Print(");\n");
        // dwords
        pFile->PrintIndent("%s(\"  words: %%d%s\", ",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
        pFile->Print(".md.dwords);\n");
        // strings
        pFile->PrintIndent("%s(\"  strings: %%d%s\", ",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
        pFile->Print(".md.strings);\n");
        // print if we got an fpage
        pFile->PrintIndent("%s(\"  fpage: %%s%s\", (",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_OUT, pContext);
        pFile->Print(".md.fpage_received==1)?\"yes\":\"no\");\n");
    }
    CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    int nShortWords = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD);
    // to determine if we can send a short IPC we have to test the size dope of the message
    pFile->PrintIndent("if ((");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
    pFile->Print(".md.dwords <= %d) && (", nShortWords);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
    pFile->Print(".md.strings == 0))\n");
    pFile->IncIndent();
    // if fpage
    pFile->PrintIndent("if (");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
    pFile->Print(".md.fpage_received == 1)\n");
    pFile->IncIndent();
    // short IPC
    WriteShortFlexpageIPC(pFile, pContext);
    // else (fpage)
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // !fpage
    WriteShortIPC(pFile, pContext);
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // if fpage
    pFile->PrintIndent("if (");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_OUT, pContext);
    pFile->Print(".md.fpage_received == 1)\n");
    pFile->IncIndent();
    // long IPC
    WriteLongFlexpageIPC(pFile, pContext);
    // else (fpage)
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // ! fpage
    WriteLongIPC(pFile, pContext);
    pFile->DecIndent();
    pFile->DecIndent();
}

/** \brief write the ip code with a short msg reply containing a flexpage
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void
CL4BEWaitAnyFunction::WriteShortFlexpageIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    ((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, true, true, pContext);
}

/** \brief write the ipc code with a short msg reply
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEWaitAnyFunction::WriteShortIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    ((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, false, true, pContext);
}

/** \brief write the ipc code with a long msg containing flexpages
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void
CL4BEWaitAnyFunction::WriteLongFlexpageIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    ((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, true, false, pContext);
}

/** \brief write ipc code with a long msg
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEWaitAnyFunction::WriteLongIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    ((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, false, false, pContext);
}

/** \brief write the checking code for opcode exceptions
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * reset any previous exceptions. Must be called before IPC Error check
 */
void
CL4BEWaitAnyFunction::WriteExceptionCheck(CBEFile * pFile, CBEContext * pContext)
{
    // set exception if not set already
    *pFile << "\t// clear exception if set\n";
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;
    pFile->PrintIndent("if (");
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars() > 0)
        pFile->Print("->");
    else
        pFile->Print(".");
    pFile->Print("major != CORBA_NO_EXCEPTION)\n");
    pFile->IncIndent();
    // set exception
    pFile->PrintIndent("CORBA_server_exception_set(");
    if (pDecl->GetStars() == 0)
        pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(",\n");
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_NO_EXCEPTION,\n");
    pFile->PrintIndent("CORBA_DICE_EXCEPTION_NONE,\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
    pFile->DecIndent();
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEWaitAnyFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("/* test for IPC errors */\n");
    pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", sResult.c_str());
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    // set opcode to zero value
    if (m_pReturnVar)
        m_pReturnVar->WriteSetZero(pFile, pContext);
    if (!m_sErrorFunction.empty())
    {
        *pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
        WriteCallParameter(pFile, m_pCorbaEnv, pContext);
        *pFile << ");\n";
    }
    // set zero value in msgbuffer
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    bool bUseConstOffset = true;
    pMarshaller->MarshalValue(pFile, this, pContext->GetSizes()->GetOpcodeSize(), 0, 0, bUseConstOffset, pContext);
    delete pMarshaller;
    // set exception if not set already
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;
    pFile->PrintIndent("if (");
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars() > 0)
        pFile->Print("->");
    else
        pFile->Print(".");
    pFile->Print("major == CORBA_NO_EXCEPTION)\n");
    pFile->IncIndent();
    // set exception
    string sSetFunc;
    if (((CBEUserDefinedType*)m_pCorbaEnv->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";
    *pFile << "\t" << sSetFunc << "(";
    if (pDecl->GetStars() == 0)
        pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(",\n");
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
    pFile->PrintIndent("CORBA_DICE_INTERNAL_IPC_ERROR,\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
    pFile->DecIndent();
    // returns 0 -> falls into default branch of server loop
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief free memory allocated at the server which should be freed after the reply
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function uses the dice_get_last_ptr method to free memory
 *
 * Only print this if the class has any [out, ref] parameters and if the option
 * -ffree_mem_after_reply is set
 */
void
CL4BEWaitAnyFunction::WriteReleaseMemory(CBEFile *pFile, CBEContext *pContext)
{
    // check [out, ref] parameters
    assert(m_pClass);
    if (!pContext->IsOptionSet(PROGRAM_FREE_MEM_AFTER_REPLY) &&
        !m_pClass->HasParametersWithAttribute(ATTR_REF, ATTR_OUT))
        return;

    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("void* ptr;\n");
    pFile->PrintIndent("while ((ptr = dice_get_last_ptr(");
    // env
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;
    if (pDecl->GetStars() == 0)
        pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(")) != 0)\n");
    pFile->IncIndent();
    pFile->PrintIndent("");
    pContext->WriteFree(pFile, this);
    pFile->Print("(ptr);\n");
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to \
 *           false if not possible
 *  \param pContext the context of the write operation
 *
 * The wait-any function does only unmarshal the opcode. We can print this code
 * by hand. We should use a marshaller anyways.
 */
void CL4BEWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext * pContext)
{
    /* If the option noopcode is set, we do not unmarshal anything at all. */
    if (FindAttribute(ATTR_NOOPCODE))
        return;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0)
    {
        // we have to always check if this was a flexpage IPC
        string sResult =
            pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        *pFile << "\tif (l4_ipc_fpage_received(" << sResult << "))\n";
        WriteFlexpageOpcodePatch(pFile, pContext);  // does indent itself
        *pFile << "\telse\n";
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
}

/** \brief writes a patch to find the opcode if flexpage were received
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function may receive messages from different function. Because we don't know at
 * compile time, which function sends, we don't know if the message contains a flexpage.
 * If it does the unmarshalled opcode is wrong. Because flexpages have to come first in
 * the message buffer, the opcode cannot be the first parameter. We have to check this
 * condition and get the opcode from behind the flexpages.
 *
 * First we get the number of flexpages of the interface. If it has none, we don't need
 * this extra code. If it has a fixed number of flexpages (either none is sent or one, but
 * if flexpages are sent it is always the same number of flexpages) we can hard code
 * the offset were to find the opcode. If we have different numbers of flexpages (one
 * function may send one, another sends two) we have to use code which can deal with a variable
 * number of flexpages.
 */
void CL4BEWaitAnyFunction::WriteFlexpageOpcodePatch(CBEFile *pFile,
    CBEContext *pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) == 0)
        return;
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the opcode) is:
        // offset = 8*nMaxNumberOfFlexpages + 8
        bool bUseConstOffset = true;
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, 8*nNumberOfFlexpages+8, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        // the variable offset can be determined by searching for the delimiter flexpage
        // which is two zero dwords
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // search for delimiter flexpage
        string sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", sTempVar.c_str());
        pFile->PrintIndent("while ((");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[%s] != 0) && (", sTempVar.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", sTempVar.c_str(), sTempVar.c_str());
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", sTempVar.c_str());
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}
