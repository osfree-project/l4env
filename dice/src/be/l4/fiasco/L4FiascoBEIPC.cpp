/**
 *    \file    dice/src/be/l4/fiasco/L4FiascoBEIPC.cpp
 *    \brief   contains the implementation of the class CL4FiascoBEIPC
 *
 *    \date    08/20/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "L4FiascoBEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"

#include "be/BEMarshalFunction.h"
#include "be/BEMarshalExceptionFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BECallFunction.h"
#include "be/BESndFunction.h"
#include "be/BEReplyFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BESizes.h"

#include "Compiler.h"
#include "Attribute-Type.h"
#include "TypeSpec-Type.h"

#include <cassert>

CL4FiascoBEIPC::CL4FiascoBEIPC()
{ }

/** \brief destructor of target class */
CL4FiascoBEIPC::~CL4FiascoBEIPC()
{ }

/** \brief write an IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 */
void
CL4FiascoBEIPC::WriteCall(CBEFile& pFile,
	CBEFunction* pFunction)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sServerID = pNF->GetComponentIDVariable();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
	string sScheduling = pNF->GetScheduleClientVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	CMsgStructType nDirection(pFunction->GetSendDirection());
	bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	CL4BEMarshaller *pMarshaller =
		dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
	assert(pMarshaller);

	bool bFlexpage =
		pFunction->GetParameterCount(TYPE_FLEXPAGE, nDirection) > 0;

	pFile << "\tl4_ipc_call_tag(*" << sServerID << ",\n";
	++pFile << "\t";
	if (IsShortIPC(pFunction, nDirection))
	{
		if (bScheduling)
			pFile << "(" << sMWord << "*)(";
		if (bFlexpage)
			pFile << "L4_IPC_SHORT_FPAGE";
		else
			pFile << "L4_IPC_SHORT_MSG";
		if (bScheduling)
			pFile << " | " << sScheduling << ")";
	}
	else
	{
		if (bFlexpage || bScheduling)
			pFile << "(" << sMWord << "*)((" << sMWord << ")";

		if (!pMsgBuffer->HasReference())
			pFile << "&";
		pFile << sMsgBuffer;

		if (bFlexpage)
			pFile << "|2";
		if (bScheduling)
			pFile << "|" << sScheduling;
		if (bFlexpage || bScheduling)
			pFile << ")";
	}
	pFile << ",\n";

	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			false, false))
		pFile << "0";
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			false, false))
		pFile << "0";
	pFile << ",\n";

	pFile << "\tl4_msgtag(0,0,0,0),\n";

	nDirection = pFunction->GetReceiveDirection();
	if (IsShortIPC(pFunction, nDirection))
		pFile << "\tL4_IPC_SHORT_MSG,\n";
	else
	{
		pFile << "\t";
		if (!pMsgBuffer->HasReference())
			pFile << "&";
		pFile << sMsgBuffer << ",\n";
	}

	string sDummy = pNF->GetDummyVariable();
	pFile << "\t";
	// if no member for this direction can be found, use dummy
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";

	pFile << "\t" << sTimeout << ", &" << sResult << ", ";
	WriteTag(pFile, pFunction, true);
	pFile << ");\n";

	--pFile;
}

/** \brief write an IPC receive operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 */
void
CL4FiascoBEIPC::WriteReceive(CBEFile& pFile,
	CBEFunction* pFunction)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sServerID = pNF->GetComponentIDVariable();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTimeout;
	if (pFunction->IsComponentSide())
		sTimeout = pNF->GetTimeoutServerVariable(pFunction);
	else
		sTimeout = pNF->GetTimeoutClientVariable(pFunction);
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	CL4BEMarshaller *pMarshaller =
		dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
	assert(pMarshaller);
	CMsgStructType nDirection(pFunction->GetReceiveDirection());

	pFile << "\t" << "l4_ipc_receive_tag(*(l4_threadid_t*)" << sServerID << ",\n";
	++pFile;

	if (IsShortIPC(pFunction, nDirection))
		pFile << "\tL4_IPC_SHORT_MSG,\n";
	else
	{
		pFile << "\t";
		if (!pMsgBuffer->HasReference())
			pFile << "&";
		pFile << sMsgBuffer << ",\n";
	}

	string sDummy = pNF->GetDummyVariable();
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";

	pFile << "\t" << sTimeout << ", &" << sResult << ", ";
	WriteTag(pFile, pFunction, true);
	pFile << ");\n";

	--pFile;
}

/** \brief write an IPC wait operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 */
void
CL4FiascoBEIPC::WriteWait(CBEFile& pFile,
	CBEFunction *pFunction)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sServerID = pNF->GetComponentIDVariable();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTimeout;
	if (pFunction->IsComponentSide())
		sTimeout = pNF->GetTimeoutServerVariable(pFunction);
	else
		sTimeout = pNF->GetTimeoutClientVariable(pFunction);
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
	CMsgStructType nDirection(pFunction->GetReceiveDirection());
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	CL4BEMarshaller *pMarshaller =
		dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
	assert(pMarshaller);

	pFile << "\tl4_ipc_wait_tag( (l4_threadid_t*)" << sServerID << ",\n";
	++pFile;
	if (IsShortIPC(pFunction, nDirection))
		pFile << "\tL4_IPC_SHORT_MSG,\n";
	else
	{
		pFile << "\t";
		if (!pMsgBuffer->HasReference())
			pFile << "&";
		pFile << sMsgBuffer << ",\n";
	}

	string sDummy = pNF->GetDummyVariable();
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";

	pFile << "\t" << sTimeout << ", &" << sResult << ", ";
	WriteTag(pFile, pFunction, true);
	pFile << ");\n";

	--pFile;
}

/** \brief write an IPC reply and receive operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param bSendFlexpage true if a flexpage should be send (false, if the \
 *         message buffer should determine this)
 *  \param bSendShortIPC true if a short IPC should be send (false, if \
 *         message buffer should determine this)
 */
void
CL4FiascoBEIPC::WriteReplyAndWait(CBEFile& pFile,
	CBEFunction* pFunction,
	bool bSendFlexpage,
	bool bSendShortIPC)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTimeout;
	if (pFunction->IsComponentSide())
		sTimeout = pNF->GetTimeoutServerVariable(pFunction);
	else
		sTimeout = pNF->GetTimeoutClientVariable(pFunction);
	string sServerID = pNF->GetComponentIDVariable();
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
	CL4BEMarshaller *pMarshaller =
		dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
	assert(pMarshaller);
	bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
	string sScheduling = pNF->GetScheduleServerVariable();

	pFile << "\tl4_ipc_reply_and_wait_tag(*" << sServerID << ",\n";
	++pFile << "\t";
	if (bSendShortIPC)
	{
		pFile << "(const void*)(";
		if (bSendFlexpage && bScheduling)
			pFile << "(unsigned)";
		if (bSendFlexpage)
			pFile << "L4_IPC_SHORT_FPAGE";
		else
			pFile << "L4_IPC_SHORT_MSG";
		if (bScheduling)
			pFile << " | " << sScheduling;
		pFile << ")";
	}
	else
	{
		if (bSendFlexpage || bScheduling)
			pFile << "(" << sMWord << "*)((" << sMWord << ")";
		pFile << sMsgBuffer;
		if (bSendFlexpage)
			pFile << "|2";
		if (bScheduling)
			pFile << "|" << sScheduling;
		if (bSendFlexpage || bScheduling)
			pFile << ")";
	}
	pFile << ",\n";

	CMsgStructType nDirection(pFunction->GetSendDirection());
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			false, false))
		pFile << "0";
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			false, false))
		pFile << "0";
	pFile << ",\n";

	pFile << "\t";
	WriteTag(pFile, pFunction, false);
	pFile << ",\n";

	pFile << "\t" << sServerID << ",\n";
	pFile << "\t" << sMsgBuffer << ",\n";

	nDirection = pFunction->GetReceiveDirection();
	string sDummy = pNF->GetDummyVariable();
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			true, false))
		pFile << "&" << sDummy;
	pFile << ",\n";

	pFile << "\t" << sTimeout << ", &" << sResult << ", ";
	WriteTag(pFile, pFunction, true);
	pFile << ");\n";

	--pFile;
}

/** \brief write an IPC send operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 */
void
CL4FiascoBEIPC::WriteSend(CBEFile& pFile,
	CBEFunction* pFunction)
{
	CMsgStructType nDirection(pFunction->GetSendDirection());
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sServerID = pNF->GetComponentIDVariable();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sTimeout;
	if (pFunction->IsComponentSide())
		sTimeout = pNF->GetTimeoutServerVariable(pFunction);
	else
		sTimeout = pNF->GetTimeoutClientVariable(pFunction);
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
	string sScheduling = pNF->GetScheduleClientVariable();
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	CL4BEMarshaller *pMarshaller =
		dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
	assert(pMarshaller);

	pFile << "\tl4_ipc_send_tag(*" << sServerID << ",\n";
	++pFile << "\t";
	bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);

	bool bFlexpage = pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0;

	if (IsShortIPC(pFunction, nDirection))
	{
		if (bScheduling)
			pFile << "(" << sMWord << "*)(";
		if (bFlexpage)
			pFile << "L4_IPC_SHORT_FPAGE";
		else
			pFile << "L4_IPC_SHORT_MSG";
		if (bScheduling)
			pFile << "|" << sScheduling << ")";
	}
	else
	{
		if (bFlexpage || bScheduling)
			pFile << "(" << sMWord << "*)((" << sMWord << ")";

		if (!pMsgBuffer->HasReference())
			pFile << "&";
		pFile << sMsgBuffer;

		if (bFlexpage)
			pFile << "|2";
		if (bScheduling)
			pFile << "|" << sScheduling;
		if (bFlexpage || bScheduling)
			pFile << ")";
	}
	pFile << ",\n";

	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			false, false))
		pFile << "0";
	pFile << ",\n";
	pFile << "\t";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			false, false))
		pFile << "0";
	pFile << ",\n";

	pFile << "\tl4_msgtag(0,0,0,0),\n";

	pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

	--pFile;
}

/** \brief write an IPC reply operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *
 * In the generic L4 case this is a send operation. We have to be careful
 * though with ASM code, which can push parameters directly into registers,
 * since the parameters for reply (exception) are not the same as for send
 * (opcode).
 */
void
CL4FiascoBEIPC::WriteReply(CBEFile& pFile,
	CBEFunction* pFunction)
{
	WriteSend(pFile, pFunction);
}

/** \brief determine if we should use assembler for the IPCs
 *  \param pFunction the function to write the call for
 *  \return true if assembler code should be written
 *
 * This implementation currently always returns false, because assembler code
 * is always ABI specific.
 */
bool
CL4FiascoBEIPC::UseAssembler(CBEFunction *)
{
	return false;
}

/** \brief helper function to test for short IPC
 *  \param pFunction the function to test
 *  \param nDirection the direction to test
 *  \return true if the function uses short IPC in the specified direction
 *
 * This is a simple helper function, which just delegates the call to the
 * function's message buffer.
 */
bool CL4FiascoBEIPC::IsShortIPC(CBEFunction *pFunction, CMsgStructType nType)
{
	if (CMsgStructType::Generic == nType)
		return IsShortIPC(pFunction, pFunction->GetSendDirection()) &&
			IsShortIPC(pFunction, pFunction->GetReceiveDirection());

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	return pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nType);
}

/** \brief add local variables required in functions
 *  \param pFunction the function to add the local variables to
 */
void CL4FiascoBEIPC::AddLocalVariable(CBEFunction *pFunction)
{
	CMsgStructType nSndDir(pFunction->GetSendDirection());

	CBENameFactory *pNF = CBENameFactory::Instance();
	assert(pFunction);

	// temp offset and offset variable
	if (dynamic_cast<CBEMarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalExceptionFunction*>(pFunction) ||
		dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
		dynamic_cast<CBEReplyFunction*>(pFunction) ||
		dynamic_cast<CBESndFunction*>(pFunction) ||
		dynamic_cast<CBEWaitFunction*>(pFunction))
	{
		// check for temp
		if (pFunction->HasVariableSizedParameters(nSndDir) ||
			pFunction->HasArrayParameters(nSndDir))
		{
			string sTmpVar = pNF->GetTempOffsetVariable();
			pFunction->AddLocalVariable(TYPE_INTEGER, true, 4, sTmpVar, 0);
			CBETypedDeclarator *pVariable =
				pFunction->m_LocalVariables.Find(sTmpVar);
			pVariable->AddLanguageProperty(string("attribute"),
				string("__attribute__ ((unused))"));

			string sOffsetVar = pNF->GetOffsetVariable();
			pFunction->AddLocalVariable(TYPE_INTEGER, true, 4, sOffsetVar, 0);
			pVariable = pFunction->m_LocalVariables.Find(sOffsetVar);
			pVariable->AddLanguageProperty(string("attribute"),
				string("__attribute__ ((unused))"));
		}
	}

	// Dummy Variable
	if (dynamic_cast<CBECallFunction*>(pFunction) ||
		dynamic_cast<CBEWaitAnyFunction*>(pFunction) ||
		dynamic_cast<CBEWaitFunction*>(pFunction) ||
		dynamic_cast<CBEReplyFunction*>(pFunction) ||
		dynamic_cast<CBESndFunction*>(pFunction))
	{
		// depends on the availability of enough members for registers or
		// parameters of IPC call
		CL4BEMsgBuffer *pMsgBuffer = dynamic_cast<CL4BEMsgBuffer*>
			(pFunction->GetMessageBuffer());
		assert(pMsgBuffer);
		CMsgStructType nRcvDir(pFunction->GetReceiveDirection());
		// interface functions use generic struct, instead of using dummys
		bool bUseDummy = dynamic_cast<CBEOperationFunction*>(pFunction) &&
			!pMsgBuffer->HasWordMembers(pFunction, nRcvDir);
		// should not depend on DEFINES
		bool bUseAssembler = UseAssembler(pFunction);
		if (bUseAssembler || bUseDummy)
		{
			string sDummy = pNF->GetDummyVariable();
			pFunction->AddLocalVariable(TYPE_MWORD, true, 0, sDummy, 0);
			CBETypedDeclarator *pVariable =
				pFunction->m_LocalVariables.Find(sDummy);
			pVariable->AddLanguageProperty(string("attribute"),
				string("__attribute__ ((unused))"));
			pVariable->AddLanguageProperty(string("defined"),
				string("__PIC__"));
		}
	}

	// dummy receive tag
	if (dynamic_cast<CBEWaitAnyFunction*>(pFunction) ||
		dynamic_cast<CBECallFunction*>(pFunction) ||
		dynamic_cast<CBEWaitFunction*>(pFunction))
	{
		// if function already has a tag parameter, no need for a tag dummy...
		string sTagDummy = pNF->GetDummyVariable(string("tag"));
		string sTagType = pNF->GetTypeName(TYPE_MSGTAG, false);
		pFunction->AddLocalVariable(sTagType, sTagDummy, 0);
		CBETypedDeclarator *pParameter = pFunction->m_LocalVariables.Find(sTagDummy);
		pParameter->SetDefaultInitString(string("l4_msgtag(0,0,0,0)"));
		pParameter->AddLanguageProperty(string("attribute"), string("__attribute__ ((unused))"));
	}
}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4FiascoBEIPC::WriteInitialization(CBEFile& /*pFile*/,
	CBEFunction* /*pFunction*/)
{}

/** \brief writes the assigning of a local name to a communication port
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4FiascoBEIPC::WriteBind(CBEFile& /*pFile*/,
	CBEFunction* /*pFunction*/)
{}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4FiascoBEIPC::WriteCleanup(CBEFile& /*pFile*/,
	CBEFunction* /*pFunction*/)
{}

/** \brief write the message tag to the IPC invocation
 *  \param pFile the file to write to
 *  \param pFunction the function calling
 *  \param bReference true if pointer to tag is wanted.
 */
void
CL4FiascoBEIPC::WriteTag(CBEFile& pFile, CBEFunction *pFunction, bool bReference)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	CBETypedDeclarator *pParameter = pFunction->m_Parameters.Find(sTag);
	if (pParameter)
	{
		CBEDeclarator *pDecl = pParameter->m_Declarators.Find(sTag);
		if (bReference && pDecl->GetStars() == 0)
			pFile << "&";
		if (!bReference && pDecl->GetStars() > 0)
			pFile << "*";
		pFile << sTag;
	}
	else
	{
		string sTagDummy = pNF->GetDummyVariable(string("tag"));
		if (bReference)
			pFile << "&";
		pFile << sTagDummy;
	}
}

