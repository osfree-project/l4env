/**
 *  \file    dice/src/be/l4/L4BECallFunction.cpp
 *  \brief   contains the implementation of the class CL4BECallFunction
 *
 *  \date    02/07/2002
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

#include "L4BECallFunction.h"
#include "L4BENameFactory.h"
#include "L4BEClassFactory.h"
#include "L4BESizes.h"
#include "L4BEIPC.h"
#include "L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "be/Trace.h"
#include "be/BEClass.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BECallFunction::CL4BECallFunction()
{
}

CL4BECallFunction::CL4BECallFunction(CL4BECallFunction & src)
 : CBECallFunction(src)
{
}

/** \brief destructor of target class */
CL4BECallFunction::~CL4BECallFunction()
{
}

/** \brief creates the call function
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 *
 * The L4 implementation also includes the result variable for the IPC
 */
void 
CL4BECallFunction::CreateBackEnd(CFEOperation *pFEOperation)
{
    CBECallFunction::CreateBackEnd(pFEOperation);

    // add local variables
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sDope = pNF->GetTypeName(TYPE_MSGDOPE_SEND, false);
    AddLocalVariable(sDope, sResult, 0, string("{ msgdope: 0 }"));
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * Because this is the call function, we can use the IPC call of L4.
 */
void 
CL4BECallFunction::WriteInvocation(CBEFile * pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BECallFunction::WriteInvocation(%s) called\n",
        GetName().c_str());

    // set dopes
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
	*pFile << "\tdo\n";
	*pFile << "\t{\n";
        pFile->IncIndent();
    }
    WriteIPC(pFile);
    if (!CCompiler::IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
    {
	CBENameFactory *pNF = CCompiler::GetNameFactory();
        // now check if call has been canceled
        string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
        pFile->DecIndent();
	*pFile << "\t} while ((L4_IPC_ERROR(" << sResult <<
	    ") == L4_IPC_SEABORTED) ||\n" <<
	    "\t         (L4_IPC_ERROR(" << sResult <<
	    ") == L4_IPC_SECANCELED));\n";
    }
    // check for errors
    WriteIPCErrorCheck(pFile);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BECallFunction::WriteInvocation(%s) finished\n",
        GetName().c_str());
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 *
 * If there was an IPC error, we write this into the environment.  This can be
 * done by checking if there was an error, then sets the major value to
 * CORBA_SYSTEM_EXCEPTION and then sets the ipc_error value to
 * L4_IPC_ERROR(result).
 */
void 
CL4BECallFunction::WriteIPCErrorCheck(CBEFile * pFile)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();

    *pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult << ")))\n" 
	<< "\t{\n";
    pFile->IncIndent();
    // env.major = CORBA_SYSTEM_EXCEPTION;
    // env.repos_id = DICE_IPC_ERROR;
    *pFile << "\tCORBA_exception_set(";
    if (pDecl->GetStars() == 0)
        *pFile << "&";
    pDecl->WriteName(pFile);
    *pFile << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_SYSTEM_EXCEPTION,\n" <<
              "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n" <<
              "\t0);\n";
    pFile->DecIndent();
    // DICE_IPC_ERROR(env) = L4_IPC_ERROR(result);
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    *pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_IPC_ERROR(" 
	<< sResult << ");\n";
    // return
    WriteReturn(pFile);
    // close }
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief initializes message size dopes
 *  \param pFile the file to write to
 */
void
CL4BECallFunction::WriteVariableInitialization(CBEFile * pFile)
{
    CBECallFunction::WriteVariableInitialization(pFile);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (CCompiler::IsOptionSet(PROGRAM_ZERO_MSGBUF))
	pMsgBuffer->WriteSetZero(pFile);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SIZE, 
	CMsgStructType::Generic);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_REFSTRING, 
	GetReceiveDirection());
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_RCV_FLEXPAGE,
	GetReceiveDirection());
}

/** \brief write L4 specific unmarshalling code
 *  \param pFile the file to write to
 *
 * We have to check for any received flexpages and fix the offset for any
 * return values.
 */
void 
CL4BECallFunction::WriteUnmarshalling(CBEFile * pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
	GetName().c_str());

    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeUnmarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }
    
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // check flexpages
    if (GetParameterCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0)
    {
        // we have to always check if this was a flexpage IPC
        string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	*pFile << "\tif (!l4_ipc_fpage_received(" << sResult << "))\n";
	*pFile << "\t{\n";
        pFile->IncIndent();
        // unmarshal exception and test if we really received an exception. If
	// so, we return
        WriteMarshalException(pFile, false, false);
        pFile->DecIndent();
	*pFile << "\t}\n";
    }
    // unmarshal rest (skip CBECallFunction)
    // unmarshals flexpages if there are any...
    CBEOperationFunction::WriteUnmarshalling(pFile);

    if (bLocalTrace)
    {
	m_pTrace->AfterUnmarshalling(pFile, this);
	m_bTraceOn = false;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s finished\n", __func__,
	GetName().c_str());
}

/** \brief writes the ipc call
 *  \param pFile the file to write to
 *
 * This implementation writes the L4 V2 IPC code.
 */
void CL4BECallFunction::WriteIPC(CBEFile* pFile)
{
    if (m_pTrace)
	m_pTrace->BeforeCall(pFile, this);
    
    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    pComm->WriteCall(pFile, this);

    if (m_pTrace)
	m_pTrace->AfterCall(pFile, this);
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BECallFunction::GetSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBECallFunction::GetSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BECallFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBECallFunction::GetFixedSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

