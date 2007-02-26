/**
 *	\file	dice/src/be/l4/L4BEReplyWaitFunction.cpp
 *	\brief	contains the implementation of the class CL4BEReplyWaitFunction
 *
 *	\date	03/07/2002
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

#include "be/l4/L4BEReplyWaitFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEClass.h"
#include "be/l4/L4BEMsgBufferType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEReplyWaitFunction);

CL4BEReplyWaitFunction::CL4BEReplyWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEReplyWaitFunction, CBEReplyWaitFunction);
}

CL4BEReplyWaitFunction::CL4BEReplyWaitFunction(CL4BEReplyWaitFunction & src)
:CBEReplyWaitFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEReplyWaitFunction, CBEReplyWaitFunction);
}

/**	\brief destructor of target class */
CL4BEReplyWaitFunction::~CL4BEReplyWaitFunction()
{

}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * We cannot set the size dope here, because this would write the size dope of
 * this function, but we need the size of the whole server. Therefore we trust
 * the wait-any function, which set the size dope in the beginning.
 */
void CL4BEReplyWaitFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // set size and send dopes
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, GetSendDirection(), pContext);

    // invocate
    WriteIPC(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start with unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 *
 * Since the reply-and-wait function can receive any message we only unmarshal the opcode. This
 * is done "by hand".
 */
void CL4BEReplyWaitFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages())
    {
        // we have to always check if this was a flexpage IPC
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
        WriteFlexpageOpcodePatch(pFile, pContext);  // does indent itself
        pFile->PrintIndent("else\n");
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the reply and wait function only contains so-called helper variables.
 * This is the result variable and marshalling helpers (offset or similar).
 */
void CL4BEReplyWaitFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBEReplyWaitFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
}

/**	\brief write the error checking code for the IPC
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CL4BEReplyWaitFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    if (!m_sErrorFunction.IsEmpty())
    {
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("/* test for IPC errors */\n");
        pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
        pFile->IncIndent();
        pFile->PrintIndent("%s(%s);\n", (const char*)m_sErrorFunction, (const char*)sResult);
        pFile->DecIndent();
    }
return;
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);

    pFile->PrintIndent("/* test for reply IPC errors */\n");
    pFile->PrintIndent("while (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();

    pFile->PrintIndent("switch(L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
    pFile->PrintIndent("{\n");
    // reply errors

    pFile->PrintIndent("case L4_IPC_ENOT_EXISTENT: // client does not exist\n");
    pFile->IncIndent();
    pFile->PrintIndent("// panic(\"cannot wait for not existing client\");\n");
    if (GetReturnType()->IsVoid())
	pFile->PrintIndent("return;\n");
    else
	pFile->PrintIndent("return %s;\n", (const char *) sOpcodeVar);
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();

    pFile->PrintIndent("case L4_IPC_SETIMEOUT: // client didn't respond\n");
    pFile->PrintIndent("case L4_IPC_REMSGCUT: // client didn't expect this message size\n");
    // receive errors
    pFile->PrintIndent("case L4_IPC_RETIMEOUT: // response timed out\n");
    pFile->PrintIndent("case L4_IPC_SEMSGCUT: // client send too much\n");
    pFile->IncIndent();
    // ignore reply and simply receive message
    WriteIPCWaitOnly(pFile, pContext);
    pFile->PrintIndent("break;\n");
    pFile->DecIndent();
    // default
    pFile->PrintIndent("}\n");

    // end of while
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/**	\brief writes the receive operation in case of a send error
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CL4BEReplyWaitFunction::WriteIPCWaitOnly(CBEFile * pFile, CBEContext * pContext)
{
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);

    pFile->PrintIndent("l4_i386_ipc_wait(%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
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
 *
 * This code is taken from CL4BEWaitAnyFunction::WriteFlexpageOpcodePatch.
 */
void CL4BEReplyWaitFunction::WriteFlexpageOpcodePatch(CBEFile * pFile, CBEContext * pContext)
{
    if (!(((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages()))
        return;
    CBEClass *pClass = GetClass();
    ASSERT(pClass);
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
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
        String sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", (const char*)sTempVar);
        pFile->PrintIndent("while ((");
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s] != 0) && (", (const char*)sTempVar);
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", (const char*)sTempVar, (const char*)sTempVar);
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", (const char*)sTempVar);
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We do not initialize the receive indirect strings, because we assume that theyhave been
 * initialized by the server loop. After that the buffer is handed to the server function.
 * If they intend to use it after the component function is left, they have to copy it.
 */
void CL4BEReplyWaitFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // call the case class
    CBEReplyWaitFunction::WriteVariableInitialization(pFile, pContext);
    if (m_pMsgBuffer)
    {
        // init flexpages
        ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteReceiveFlexpageInitialization(pFile, pContext);
    }
}

/** \brief test if this function has variable sized parameters (needed to specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEReplyWaitFunction::HasVariableSizedParameters()
{
    bool bRet = CBEReplyWaitFunction::HasVariableSizedParameters();
    bool bFixedNumberOfFlexpages = true;
    CBEClass *pClass = GetClass();
    ASSERT(pClass);
    pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if no flexpages, return
    if (!bFixedNumberOfFlexpages)
        return true;
    return bRet;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the opcode,
 * which should not be counted as a parameter.
 */
int CL4BEReplyWaitFunction::GetMaxReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the opcode,
 * which should not be counted as a parameter.
 */
int CL4BEReplyWaitFunction::GetFixedReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the opcode,
 * which should not be counted as a parameter.
 */
int CL4BEReplyWaitFunction::GetReturnSize(int nDirection, CBEContext * pContext)
{
    return 0;
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BEReplyWaitFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    // if the 1st parameter is the return variable, we cannot exchange it, because
    // we make assumptions about its position in the message buffer
    String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
    if (pPrecessor->FindDeclarator(sReturn))
        return false;
    // if successor is return variable (should not occur) move it forward
    if (pSuccessor->FindDeclarator(sReturn))
        return true;
    // no special case, so use base class' method
    return CBEReplyWaitFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief writes the Ipc code for this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEReplyWaitFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);

    pFile->PrintIndent("l4_i386_ipc_reply_and_wait(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasSendFlexpages())
        pFile->Print("(%s*)((%s)", (const char*)sMWord, (const char*)sMWord);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
        pFile->Print("%s", (const char *) sMsgBuffer);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasSendFlexpages())
        pFile->Print("|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4]))),\n");

    pFile->PrintIndent("%s,\n", (const char *) sServerID);

    pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
}
