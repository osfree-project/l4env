/**
 *  \file     dice/src/be/l4/v4/L4V4BEWaitAnyFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BEWaitAnyFunction
 *
 *  \date     Mon Jul 5 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#include "L4V4BEWaitAnyFunction.h"
#include "L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEUserDefinedType.h"
#include "be/BECommunication.h"
#include "be/BEMsgBuffer.h"
#include "be/Trace.h"
#include "be/l4/TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4V4BEWaitAnyFunction::CL4V4BEWaitAnyFunction(bool bOpenWait, bool bReply)
: CL4BEWaitAnyFunction(bOpenWait, bReply)
{
}

/** destroys the instance of the class */
CL4V4BEWaitAnyFunction::~CL4V4BEWaitAnyFunction()
{
}

/** \brief writes the invocation call to thetarget file
 *  \param pFile the file to write to
 *
 * The wait any function simply waits for any message and unmarshals the
 * opcode. Since the message buffer is a referenced parameter, we know for sure,
 * that the "buffer" is a pointer.
 */
void
CL4V4BEWaitAnyFunction::WriteInvocation(CBEFile& pFile)
{
    // load message
    bool bVarSized = false;
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer->m_Declarators.First() &&
	((pMsgBuffer->m_Declarators.First()->GetStars() > 0) ||
	 pMsgBuffer->IsVariableSized(GetReceiveDirection())))
	bVarSized = true;

    if (m_bReply)
    {
	// load the message into the UTCB
	pFile << "\tL4_MsgLoad ( (L4_Msg_t*) " << ((bVarSized) ? "" : "&") <<
	    pMsgBuffer->m_Declarators.First()->GetName() << " );\n";
    }

    // invocate
    WriteIPC(pFile);

    // store message
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    pFile << "\tL4_MsgStore (" << sMsgTag << ", (L4_Msg_t*) " <<
	((bVarSized) ? "" : "&") << pMsgBuffer->m_Declarators.First()->GetName() <<
	");\n";

    WriteIPCErrorCheck(pFile); // set IPC exception
    if (m_bReply)
	WriteReleaseMemory(pFile);
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 */
void
CL4V4BEWaitAnyFunction::WriteIPCReplyWait(CBEFile& pFile)
{
    if (m_pTrace)
	m_pTrace->BeforeReplyWait(pFile, this);

    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    pComm->WriteReplyAndWait(pFile, this);

    // print trace code before IPC error check to have unmodified values in
    // message buffer
    if (m_pTrace)
	m_pTrace->AfterReplyWait(pFile, this);
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *
 * The wait-any function does only unmarshal the opcode. We can print this code
 * by hand. We should use a marshaller anyways.
 */
void CL4V4BEWaitAnyFunction::WriteUnmarshalling(CBEFile& pFile)
{
    /* If the option noopcode is set, we do not unmarshal anything at all.
     */
    if (m_Attributes.Find(ATTR_NOOPCODE))
	return;

    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeUnmarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }

    /* the opcode is the label in the msgtag */
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    /* get name of opcode (return variable) */
    CBETypedDeclarator *pReturn = GetReturnVariable();
    CBEDeclarator *pD = (pReturn) ? pReturn->m_Declarators.First() : 0;

    pFile << "\t// 'unmarshal' the opcode\n";
    pFile << "\t" << pD->GetName() << " = L4_Label (" << sMsgTag << ");\n";

    if (bLocalTrace)
    {
	m_pTrace->AfterUnmarshalling(pFile, this);
	m_bTraceOn = false;
    }
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 */
void
CL4V4BEWaitAnyFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
    pFile << "\t/* test for IPC errors */\n";
    pFile << "\tif ( L4_IpcFailed ( " << sMsgTag << " ))\n";
    pFile << "\t{\n";
    ++pFile;

    // set opcode to zero value
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (pReturn)
	pReturn->WriteSetZero(pFile);
    if (!m_sErrorFunction.empty())
    {
	pFile << "\t" << m_sErrorFunction << "(" << sMsgTag << ", ";
	WriteCallParameter(pFile, pEnv, true);
	pFile << ");\n";
    }
    // set label to zero and store in msgbuf
    bool bVarSized = false;
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer->m_Declarators.First() &&
	((pMsgBuffer->m_Declarators.First()->GetStars() > 0) ||
	 pMsgBuffer->IsVariableSized(GetReceiveDirection())))
	bVarSized = true;
    pFile << "\tL4_Set_MsgLabel ( (" << sType << "*) " << ((bVarSized) ? "" : "&") <<
	pMsgBuffer->m_Declarators.First()->GetName() << ", 0);\n";
    // set exception
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();
    string sSetFunc;
    if (((CBEUserDefinedType*)pEnv->GetType())->GetName() ==
	"CORBA_Server_Environment")
	sSetFunc = "CORBA_server_exception_set";
    else
	sSetFunc = "CORBA_exception_set";
    pFile << "\t" << sSetFunc << "(";
    if (pDecl->GetStars() == 0)
	pFile << "&";
    pDecl->WriteName(pFile);
    pFile << ",\n";
    ++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
    pFile << "\tCORBA_DICE_INTERNAL_IPC_ERROR,\n";
    pFile << "\t0);\n";
    --pFile;
    // returns 0 -> falls into default branch of server loop
    WriteReturn(pFile);
    --pFile << "\t}\n";
}

/** \brief initialize instance of class
 *  \param pFEInterface the front-end interface to use as reference
 *  \return true if successful
 */
void
CL4V4BEWaitAnyFunction::CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide)
{
    CBEWaitAnyFunction::CreateBackEnd(pFEInterface, bComponentSide);

    // need message tag
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
    AddLocalVariable(sType, sMsgTag, 0);
}

