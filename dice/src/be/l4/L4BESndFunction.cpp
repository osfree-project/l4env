/**
 *  \file    dice/src/be/l4/L4BESndFunction.cpp
 *  \brief   contains the implementation of the class CL4BESndFunction
 *
 *  \date    06/01/2002
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

#include "be/l4/L4BESndFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEMsgBuffer.h"
#include "be/BEClient.h"
#include "be/BEUserDefinedType.h"
#include "be/Trace.h"
#include "be/l4/L4BESizes.h"
#include "be/l4/v2/L4V2BEIPC.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BESndFunction::CL4BESndFunction()
{
}

/** destructs the send function class */
CL4BESndFunction::~CL4BESndFunction()
{
}

/** \brief initialize the instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 *
 * For L4 we need result dope for IPC
 */
void
CL4BESndFunction::CreateBackEnd(CFEOperation *pFEOperation)
{
    CBESndFunction::CreateBackEnd(pFEOperation);

    // add local variables
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sDope = pNF->GetTypeName(TYPE_MSGDOPE_SEND, false);
    AddLocalVariable(sDope, sResult, 0, string("{ msgdope: 0 }"));
}

/** \brief writes the invocation code
 *  \param pFile the file tow rite to
 */
void CL4BESndFunction::WriteInvocation(CBEFile& pFile)
{
    // after marshalling set the message dope
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
	GetSendDirection());
    // invocate
    if (!CCompiler::IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
    {
        // sometimes it's possible to abort a call of a client.
        // but the client wants his call made, so we try until
        // the call completes
        pFile << "\tdo\n";
        pFile << "\t{\n";
        ++pFile;
    }
    WriteIPC(pFile);
    if (!CCompiler::IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
    {
	CBENameFactory *pNF = CCompiler::GetNameFactory();
        // now check if call has been canceled
        string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
        --pFile << "\t} while ((L4_IPC_ERROR(" << sResult <<
            ") == L4_IPC_SEABORTED) ||\n";
        pFile << "\t         (L4_IPC_ERROR(" << sResult <<
            ") == L4_IPC_SECANCELED));\n";
    }
    WriteIPCErrorCheck(pFile);
}

/** \brief writes the IPC error check
 *  \param pFile the file to write to
 *
 * \todo write IPC error checking
 */
void
CL4BESndFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();

    pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult << ")))\n"
	<< "\t{\n";
    // env.major = CORBA_SYSTEM_EXCEPTION;
    // env.repos_id = DICE_IPC_ERROR;
    string sSetFunc;
    if (((CBEUserDefinedType*)pEnv->GetType())->GetName() ==
	"CORBA_Server_Environment")
	sSetFunc = "CORBA_server_exception_set";
    else
	sSetFunc = "CORBA_exception_set";
    ++pFile << "\t" << sSetFunc << "(";
    if (pDecl->GetStars() == 0)
        pFile << "&";
    pDecl->WriteName(pFile);
    pFile << ",\n";
    ++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n" <<
              "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n" <<
              "\t0);\n";
    // DICE_IPC_ERROR(env) = L4_IPC_ERROR(result);
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    --pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_IPC_ERROR("
	<< sResult << ");\n";
    // return
    WriteReturn(pFile);
    // close }
    --pFile << "\t}\n";
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 */
void
CL4BESndFunction::WriteVariableInitialization(CBEFile& pFile)
{
    CBESndFunction::WriteVariableInitialization(pFile);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SIZE,
	GetSendDirection());
}

/** \brief write the IPC code
 *  \param pFile the file to write to
 */
void CL4BESndFunction::WriteIPC(CBEFile& pFile)
{
    if (m_pTrace)
	m_pTrace->BeforeCall(pFile, this);

    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    pComm->WriteSend(pFile, this);

    if (m_pTrace)
	m_pTrace->AfterCall(pFile, this);
}

