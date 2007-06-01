/**
 *  \file    dice/src/be/l4/L4BEWaitAnyFunction.cpp
 *  \brief   contains the implementation of the class CL4BEWaitAnyFunction
 *
 *  \date    03/07/2002
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

#include "L4BEWaitAnyFunction.h"
#include "L4BENameFactory.h"
#include "L4BEClassFactory.h"
#include "L4BESizes.h"
#include "L4BEIPC.h"
#include "L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEOperationFunction.h"
#include "be/BEMsgBuffer.h"
#include "be/Trace.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BEWaitAnyFunction::CL4BEWaitAnyFunction(bool bOpenWait, bool bReply)
: CBEWaitAnyFunction(bOpenWait, bReply)
{
}

CL4BEWaitAnyFunction::CL4BEWaitAnyFunction(CL4BEWaitAnyFunction & src)
: CBEWaitAnyFunction(src)
{
}

/** \brief destructor of target class */
CL4BEWaitAnyFunction::~CL4BEWaitAnyFunction()
{
}

/** \brief initialize instance of this class
 *  \param pFEInterface the interface to use as reference
 *  \return true if successful
 */
void
CL4BEWaitAnyFunction::CreateBackEnd(CFEInterface *pFEInterface)
{
    // set local variables
    CBEWaitAnyFunction::CreateBackEnd(pFEInterface);

    string exc = string(__func__);
    // add local variables
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sDope = pNF->GetTypeName(TYPE_MSGDOPE_SEND, false);

    string sCurr;
    try
    {
	sCurr = sResult;
	AddLocalVariable(sDope, sResult, 0, 
	    string("{ msgdope: 0 }"));
    }
    catch (CBECreateException *e)
    {
	exc += " failed, because local variable (" + sCurr +
	    ") could not be added.";
	throw new CBECreateException(exc);
    }
}

/** \brief initializes the variables
 *  \param pFile the file to write to
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
CL4BEWaitAnyFunction::WriteVariableInitialization(CBEFile * pFile)
{
    // call base class
    CBEWaitAnyFunction::WriteVariableInitialization(pFile);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer && m_bReply)
    {
	// init receive flexpage
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_FLEXPAGE,
	    GetReceiveDirection());
    }
}

/** \brief writes the invocation call to thetarget file
 *  \param pFile the file to write to
 *
 * The wait any function simply waits for any message and unmarshals the opcode.
 * Since the message buffer is a referenced parameter, we know for sure, that
 * the "buffer" is a pointer.
 */
void
CL4BEWaitAnyFunction::WriteInvocation(CBEFile * pFile)
{
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // if not sending a reply, i.e., no send phase, we can ignore the
    // send dope.
    // The size dope should be set independently.
    // When sending a reply, we do not overwrite the send dope, because it
    // has been set in the marshal functions.

    // invocate
    WriteIPC(pFile);
    WriteExceptionCheck(pFile); // reset exception

    // print trace code before IPC error check to have unmodified values in
    // message buffer
    if (m_pTrace)
	m_pTrace->AfterReplyWait(pFile, this);
	
    WriteIPCErrorCheck(pFile); // set IPC exception

    WriteReleaseMemory(pFile);
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 */
void CL4BEWaitAnyFunction::WriteIPC(CBEFile *pFile)
{
    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    if (m_bOpenWait)
    {
	if (m_bReply)
	    WriteIPCReplyWait(pFile);
	else
	{
	    CBEClass *pClass = GetSpecificParent<CBEClass>();
	    assert(pClass);
	    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
		WriteDedicatedWait(pFile);
	    else
		pComm->WriteWait(pFile, this);
	}
    }
    else
	pComm->WriteReceive(pFile, this);
}


/** \brief writes the switch between receive and wait
 *  \param pFile the file to write to
 */
void
CL4BEWaitAnyFunction::WriteDedicatedWait(CBEFile *pFile)
{
    CL4BENameFactory *pNF = static_cast<CL4BENameFactory*>(
	CCompiler::GetNameFactory());
    string sPartner = pNF->GetPartnerVariable();
    *pFile << "\tif (l4_is_invalid_id(" << sPartner << "))" <<
	" /* no dedicated partner */\n";
    pFile->IncIndent();
    CBECommunication *pComm = GetCommunication();
    pComm->WriteWait(pFile, this);
    pFile->DecIndent();
    *pFile << "\telse /* dedicated partner */\n";
    pFile->IncIndent();
    pComm->WriteReceive(pFile, this);
    pFile->DecIndent();
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 */
void
CL4BEWaitAnyFunction::WriteIPCReplyWait(CBEFile *pFile)
{
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(GetMarshaller());
    assert(pMarshaller);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    CMsgStructType nType = GetSendDirection();

    if (m_pTrace)
	m_pTrace->BeforeReplyWait(pFile, this);

    CL4BESizes *pSizes = (CL4BESizes*)CCompiler::GetSizes();
    int nShortWords = pSizes->GetMaxShortIPCSize() / 
	pSizes->GetSizeOfType(TYPE_MWORD);
    // to determine if we can send a short IPC we have to test the size dope
    // of the message
    *pFile << "\tif ((";
    pMsgBuffer->WriteMemberAccess(pFile, this, nType, TYPE_MSGDOPE_SEND,
	0);
    *pFile << ".md.dwords <= " << nShortWords << ") && (";
    pMsgBuffer->WriteMemberAccess(pFile, this, nType, TYPE_MSGDOPE_SEND,
	0);
    *pFile << ".md.strings == 0))\n";
    pFile->IncIndent();
    // if fpage
    *pFile << "\tif (";
    pMsgBuffer->WriteMemberAccess(pFile, this, nType, TYPE_MSGDOPE_SEND,
	0);
    *pFile << ".md.fpage_received == 1)\n";
    pFile->IncIndent();
    // short IPC
    WriteShortFlexpageIPC(pFile);
    // else (fpage)
    pFile->DecIndent();
    *pFile << "\telse\n";
    pFile->IncIndent();
    // !fpage
    WriteShortIPC(pFile);
    pFile->DecIndent();
    pFile->DecIndent();
    *pFile << "\telse\n";
    pFile->IncIndent();
    // if fpage
    *pFile << "\tif (";
    pMsgBuffer->WriteMemberAccess(pFile, this, nType, TYPE_MSGDOPE_SEND,
	0);
    *pFile << ".md.fpage_received == 1)\n";
    pFile->IncIndent();
    // long IPC
    WriteLongFlexpageIPC(pFile);
    // else (fpage)
    pFile->DecIndent();
    *pFile << "\telse\n";
    pFile->IncIndent();
    // ! fpage
    WriteLongIPC(pFile);
    pFile->DecIndent();
    pFile->DecIndent();
}

/** \brief write the ip code with a short msg reply containing a flexpage
 *  \param pFile the file to write to
 */
void
CL4BEWaitAnyFunction::WriteShortFlexpageIPC(CBEFile *pFile)
{
    CL4BEIPC *pComm = static_cast<CL4BEIPC*>(GetCommunication());
    assert(pComm);

    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
	CL4BENameFactory *pNF = static_cast<CL4BENameFactory*>(
	    CCompiler::GetNameFactory());
	string sPartner = pNF->GetPartnerVariable();
	*pFile << "\tif (l4_is_invalid_id(" << sPartner << "))" <<
	    " /* no dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteReplyAndWait(pFile, this, true, true);
	pFile->DecIndent();
	*pFile << "\telse /* dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteCall(pFile, this);
	pFile->DecIndent();
    }
    else
	pComm->WriteReplyAndWait(pFile, this, true, true);
}

/** \brief write the ipc code with a short msg reply
 *  \param pFile the file to write to
 */
void CL4BEWaitAnyFunction::WriteShortIPC(CBEFile *pFile)
{
    CL4BEIPC *pComm = static_cast<CL4BEIPC*>(GetCommunication());
    assert(pComm);

    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
	CL4BENameFactory *pNF = static_cast<CL4BENameFactory*>(
	    CCompiler::GetNameFactory());
	string sPartner = pNF->GetPartnerVariable();
	*pFile << "\tif (l4_is_invalid_id(" << sPartner << "))" <<
	    " /* no dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteReplyAndWait(pFile, this, false, true);
	pFile->DecIndent();
	*pFile << "\telse /* dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteCall(pFile, this);
	pFile->DecIndent();
    }
    else
	pComm->WriteReplyAndWait(pFile, this, false, true);
}

/** \brief write the ipc code with a long msg containing flexpages
 *  \param pFile the file to write to
 */
void
CL4BEWaitAnyFunction::WriteLongFlexpageIPC(CBEFile *pFile)
{
    CL4BEIPC *pComm = static_cast<CL4BEIPC*>(GetCommunication());
    assert(pComm);

    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
	CL4BENameFactory *pNF = static_cast<CL4BENameFactory*>(
	    CCompiler::GetNameFactory());
	string sPartner = pNF->GetPartnerVariable();
	*pFile << "\tif (l4_is_invalid_id(" << sPartner << "))" <<
	    " /* no dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteReplyAndWait(pFile, this, true, false);
	pFile->DecIndent();
	*pFile << "\telse /* dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteCall(pFile, this);
	pFile->DecIndent();
    }
    else
	pComm->WriteReplyAndWait(pFile, this, true, false);
}

/** \brief write ipc code with a long msg
 *  \param pFile the file to write to
 */
void CL4BEWaitAnyFunction::WriteLongIPC(CBEFile *pFile)
{
    CL4BEIPC *pComm = static_cast<CL4BEIPC*>(GetCommunication());
    assert(pComm);

    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
	CL4BENameFactory *pNF = static_cast<CL4BENameFactory*>(
	    CCompiler::GetNameFactory());
	string sPartner = pNF->GetPartnerVariable();
	*pFile << "\tif (l4_is_invalid_id(" << sPartner << "))" <<
	    " /* no dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteReplyAndWait(pFile, this, false, false);
	pFile->DecIndent();
	*pFile << "\telse /* dedicated partner */\n";
	pFile->IncIndent();
	pComm->WriteCall(pFile, this);
	pFile->DecIndent();
    }
    else
	pComm->WriteReplyAndWait(pFile, this, false, false);
}

/** \brief write the checking code for opcode exceptions
 *  \param pFile the file to write to
 *
 * reset any previous exceptions. Must be called before IPC Error check
 */
void
CL4BEWaitAnyFunction::WriteExceptionCheck(CBEFile * pFile)
{
    // set exception if not set already
    *pFile << "\t/* clear exception if set*/\n";
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();

    *pFile << "\tif (DICE_EXPECT_FALSE(DICE_HAS_EXCEPTION(" << sEnv << ")))\n";
    pFile->IncIndent();
    // set exception
    *pFile << "\tCORBA_server_exception_set(";
    if (pDecl->GetStars() == 0)
	*pFile << "&";
    pDecl->WriteName(pFile);
    *pFile << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_NO_EXCEPTION,\n";
    *pFile << "\tCORBA_DICE_EXCEPTION_NONE,\n";
    *pFile << "\t0);\n";
    pFile->DecIndent();
    pFile->DecIndent();
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 */
void 
CL4BEWaitAnyFunction::WriteIPCErrorCheck(CBEFile * pFile)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    *pFile << "\t/* test for IPC errors */\n";
    *pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult << ")))\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    // set opcode to zero value
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (pReturn)
	pReturn->WriteSetZero(pFile);
    if (!m_sErrorFunction.empty())
    {
	*pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
	WriteCallParameter(pFile, pEnv, true);
	*pFile << ");\n";
    }
    // get environment variable
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    // if scheduling attribute is used, then reset the scheduling bit in case
    // of error
    if (m_Attributes.Find(ATTR_SCHED_DONATE))
    {
	*pFile << "\t/* reset scheduling bits */\n";
	*pFile << "\tdice_l4_sched_donate_srv(" << sEnv << ");\n";
    }

    // if error print it here
    if (m_pTrace)
	m_pTrace->WaitCommError(pFile, this);

    // set zero value in opcode in msgbuffer
    CBEMarshaller *pMarshaller = GetMarshaller();
    pMarshaller->MarshalValue(pFile, this, pReturn, 0);
    // set exception if not set already
    *pFile << "\tif (DICE_IS_NO_EXCEPTION(" << sEnv << "))\n";
    pFile->IncIndent();
    // set exception
    string sSetFunc;
    if (((CBEUserDefinedType*)pEnv->GetType())->GetName() ==
	"CORBA_Server_Environment")
	sSetFunc = "CORBA_server_exception_set";
    else
	sSetFunc = "CORBA_exception_set";
    *pFile << "\t" << sSetFunc << "(";
    if (pDecl->GetStars() == 0)
	*pFile << "&";
    pDecl->WriteName(pFile);
    *pFile << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
    *pFile << "\tCORBA_DICE_INTERNAL_IPC_ERROR,\n";
    *pFile << "\t0);\n";
    pFile->DecIndent();
    pFile->DecIndent();
    // returns 0 -> falls into default branch of server loop
    WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief free memory allocated at the server which should be freed after \
 *         the reply
 *  \param pFile the file to write to
 *
 * This function uses the dice_get_last_ptr method to free memory
 *
 * Only print this if the class has any [out, ref] parameters and if the option
 * -ffree_mem_after_reply is set
 */
void
CL4BEWaitAnyFunction::WriteReleaseMemory(CBEFile *pFile)
{
    // check [out, ref] parameters
    assert(m_pClass);
    if (!CCompiler::IsOptionSet(PROGRAM_FREE_MEM_AFTER_REPLY) &&
	!m_pClass->HasParametersWithAttribute(ATTR_REF, ATTR_OUT))
	return;

    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tvoid* ptr;\n";
    *pFile << "\twhile ((ptr = dice_get_last_ptr(";
    // env
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();
    if (pDecl->GetStars() == 0)
	*pFile << "&";
    pDecl->WriteName(pFile);
    *pFile << ")) != 0)\n";
    pFile->IncIndent();
    *pFile << "\t";
    CBEContext::WriteFree(pFile, this);
    *pFile << "(ptr);\n";
    pFile->DecIndent();
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *
 * The wait-any function does only unmarshal the opcode. We can print this code
 * by hand. We should use a marshaller anyways.
 */
void CL4BEWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile)
{
    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeUnmarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }

    /* If the option noopcode is set, we do not unmarshal anything at all. */
    if (m_Attributes.Find(ATTR_NOOPCODE))
	return;
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer->GetCountAll(TYPE_FLEXPAGE, GetReceiveDirection()) > 0)
    {
	// we have to always check if this was a flexpage IPC
	//
	// Because the wait-any function always has a return type (the opcode)
	// we do not have to check for it separately
	string sResult =
	    CCompiler::GetNameFactory()->GetString(CL4BENameFactory::STR_RESULT_VAR);
	*pFile << "\tif (l4_ipc_fpage_received(" << sResult << "))\n";
	WriteFlexpageOpcodePatch(pFile);  // does indent itself
	*pFile << "\telse\n";
	pFile->IncIndent();
	WriteMarshalReturn(pFile, false);
	pFile->DecIndent();
    }
    else
    {
	WriteMarshalReturn(pFile, false);
    }

    if (bLocalTrace)
    {
	m_pTrace->AfterUnmarshalling(pFile, this);
	m_bTraceOn = false;
    }
}

/** \brief writes a patch to find the opcode if flexpage were received
 *  \param pFile the file to write to
 *
 * This function may receive messages from different function. Because we
 * don't know at compile time, which function sends, we don't know if the
 * message contains a flexpage.  If it does the unmarshalled opcode is wrong.
 * Because flexpages have to come first in the message buffer, the opcode
 * cannot be the first parameter. We have to check this condition and get the
 * opcode from behind the flexpages.
 *
 * First we get the number of flexpages of the interface. If it has none, we
 * don't need this extra code. If it has a fixed number of flexpages (either
 * none is sent or one, but if flexpages are sent it is always the same number
 * of flexpages) we can hard code the offset were to find the opcode. If we
 * have different numbers of flexpages (one function may send one, another
 * sends two) we have to use code which can deal with a variable number of
 * flexpages.
 */
void CL4BEWaitAnyFunction::WriteFlexpageOpcodePatch(CBEFile *pFile)
{
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer->GetCountAll(TYPE_FLEXPAGE, GetReceiveDirection()) == 0)
	return;
    
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = 
	m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages, DIRECTION_INOUT);
    CBESizes *pSizes = CCompiler::GetSizes();
    int nSizeFpage = pSizes->GetSizeOfType(TYPE_FLEXPAGE) /
	             pSizes->GetSizeOfType(TYPE_MWORD);
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
	// the fixed offset (where to find the opcode) is:
	// offset = 8*nMaxNumberOfFlexpages + 8
	pFile->IncIndent();
	CBETypedDeclarator *pReturn = GetReturnVariable();
	if (!pReturn)
	    return;
	CL4BEMarshaller *pMarshaller = 
	    dynamic_cast<CL4BEMarshaller*>(GetMarshaller());
	assert(pMarshaller);
	pMarshaller->MarshalParameter(pFile, this, pReturn, false, 
	    (nNumberOfFlexpages+1) * nSizeFpage);
	pFile->DecIndent();
    }
    else
    {
	// the variable offset can be determined by searching for the
	// delimiter flexpage which is two zero dwords
	*pFile << "\t{\n";
	pFile->IncIndent();
	// search for delimiter flexpage
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sTempVar = pNF->GetTempOffsetVariable();
	// init temp var
	*pFile << "\t" << sTempVar << " = 0;\n";
	*pFile << "\twhile ((";
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
	*pFile << "[" << sTempVar << "++] != 0) && (";
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
	*pFile << "[" << sTempVar << "++] != 0)) /* empty */;\n";

	// now sTempVar points to the delimiter flexpage
	// we have to add another 8 bytes to find the opcode, because
	// UnmarshalReturn does only use temp-var
	*pFile << "\t/* skip zero fpage */\n";
	*pFile << "\t" << sTempVar << " += 2;\n";
	// now unmarshal opcode
	WriteMarshalReturn(pFile, false);
	pFile->DecIndent();
	*pFile << "\t}\n";
    }
}

/** \brief writes the clean up of the function
 *  \param pFile the file to write to
 *
 * reset the scheduling bit if it could be used.
 */
void CL4BEWaitAnyFunction::WriteCleanup(CBEFile * pFile)
{
    // get environment variable
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    // if scheduling attribute is used, then reset the scheduling bit in case
    // of error
    if (m_Attributes.Find(ATTR_SCHED_DONATE))
    {
	*pFile << "\t/* reset scheduling bits */\n";
	*pFile << "\tdice_l4_sched_donate_srv(" << sEnv << ");\n";
    }
}
