/**
 *  \file    dice/src/be/l4/L4BEWaitFunction.cpp
 *  \brief   contains the implementation of the class CL4BEWaitFunction
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

#include "L4BEWaitFunction.h"
#include "L4BENameFactory.h"
#include "L4BEClassFactory.h"
#include "L4BESizes.h"
#include "L4BEIPC.h"
#include "L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEComponent.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEClient.h"
#include "be/BEMsgBuffer.h"
#include "be/BEClass.h"
#include "be/BEUserDefinedType.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BEWaitFunction::CL4BEWaitFunction(bool bOpenWait)
: CBEWaitFunction(bOpenWait)
{
}

/** destroys this object */
CL4BEWaitFunction::~CL4BEWaitFunction()
{
}

/** \brief initialize instance of class
 *  \param pFEOperation the front-end function to use as reference
 *  \return true on success
 */
void
CL4BEWaitFunction::CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide)
{
    CBEWaitFunction::CreateBackEnd(pFEOperation, bComponentSide);

    // add local variables
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sDope = pNF->GetTypeName(TYPE_MSGDOPE_SEND, false);
    AddLocalVariable(sDope, sResult, 0, string("{ msgdope: 0 }"));
}

/** \brief creates the CORBA_Environment variable (and parameter)
 *  \return true if successful
 *
 * In a wait-function we always use the server environment, because it
 * contains receive flexpages, etc.
 */
void
CL4BEWaitFunction::CreateEnvironment()
{
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	// if function is at server side, this is a CORBA_Server_Environment
	string sTypeName = "CORBA_Server_Environment";
	string sName = pNF->GetCorbaEnvironmentVariable();
	CBETypedDeclarator *pEnv = pCF->GetNewTypedDeclarator();
	pEnv->SetParent(this);
	pEnv->CreateBackEnd(sTypeName, sName, 1);
	SetEnvironment(pEnv);
}

/** \brief writes the communication invocation
 *  \param pFile the fiel to write to
 */
void
CL4BEWaitFunction::WriteInvocation(CBEFile& pFile)
{
    // set size and send dope
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
	GetReceiveDirection());

    // invocate
    WriteIPC(pFile);
    WriteIPCErrorCheck(pFile);
    // write opcode check
    WriteOpcodeCheck(pFile);
}

/** \brief writes the IPC error checking code
 *  \param pFile the file to write to
 */
void
CL4BEWaitFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    if (!m_sErrorFunction.empty())
    {
	pFile << "\t/* test for IPC errors */\n";
	pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult
	    << ")))\n";
        ++pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
        WriteCallParameter(pFile, GetEnvironment(), true);
        pFile << ");\n";
        --pFile;
    }
    else if (!IsComponentSide())
    {
	CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();
	string sEnv;
	if (pDecl->GetStars() == 0)
	    sEnv = "&";
	sEnv += pDecl->GetName();

	CBEType *pType = GetEnvironment()->GetType();
	string sSetFunc;
	if (static_cast<CBEUserDefinedType*>(pType)->GetName() ==
	    "CORBA_Server_Environment")
	    sSetFunc = "CORBA_server_exception_set";
	else
	    sSetFunc = "CORBA_exception_set";

	pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult <<
	    ")))\n" <<
	    "\t{\n";
	// env.major = CORBA_SYSTEM_EXCEPTION;
	// env.repos_id = DICE_IPC_ERROR;
	++pFile << "\t" << sSetFunc << " (" << sEnv << ",\n";
	++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n" <<
	    "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n" <<
	    "\t0);\n";
	// env.ipc_error = L4_IPC_ERROR(result);
	--pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_IPC_ERROR(" <<
	    sResult << ");\n";
	// return
	WriteReturn(pFile);
	// close }
	--pFile << "\t}\n";
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
void
CL4BEWaitFunction::WriteFlexpageOpcodePatch(CBEFile& pFile)
{
    if (GetParameterCount(TYPE_FLEXPAGE, GetReceiveDirection()) == 0)
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
	++pFile;
	CBETypedDeclarator *pReturn = GetReturnVariable();
	if (!pReturn)
	    return;
	CL4BEMarshaller *pMarshaller =
	    dynamic_cast<CL4BEMarshaller*>(GetMarshaller());
	assert(pMarshaller);
	pMarshaller->MarshalParameter(pFile, this, pReturn, false,
	    (nNumberOfFlexpages+1) * nSizeFpage);
	--pFile;
    }
    else
    {
	// the variable offset can be determined by searching for the
	// delimiter flexpage which is two zero dwords
	pFile << "\t{\n";
	// search for delimiter flexpage
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sTempVar = pNF->GetTempOffsetVariable();
	// init temp var
	++pFile << "\t" << sTempVar << " = 0;\n";
	pFile << "\twhile ((";
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
	pFile << "[" << sTempVar << "++] != 0) && (";
	pMsgBuffer->WriteMemberAccess(pFile, this, CMsgStructType::Generic, TYPE_MWORD, 0);
	pFile << "[" << sTempVar << "++] != 0)) /* empty */;\n";

	// now sTempVar points to the delimiter flexpage
	// we have to add another 8 bytes to find the opcode, because
	// UnmarshalReturn does only use temp-var
	pFile << "\t/* skip zero fpage */\n";
	pFile << "\t" << sTempVar << " += 2;\n";
	// now unmarshal opcode
	WriteMarshalReturn(pFile, false);
	++pFile << "\t}\n";
    }
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 */
void
CL4BEWaitFunction::WriteIPC(CBEFile& pFile)
{
    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    if (m_bOpenWait)
        pComm->WriteWait(pFile, this);
    else
        pComm->WriteReceive(pFile, this);
}

/** \brief init message receive flexpage
 *  \param pFile the file to write to
 */
void
CL4BEWaitFunction::WriteVariableInitialization(CBEFile& pFile)
{
    CBEWaitFunction::WriteVariableInitialization(pFile);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    CMsgStructType nType = GetReceiveDirection();
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SIZE, nType);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_RCV_FLEXPAGE, nType);
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int
CL4BEWaitFunction::GetSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBEWaitFunction::GetSize(nDirection);
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
int
CL4BEWaitFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEWaitFunction::GetFixedSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief test if this function has variable sized parameters
 *  \return true if variable sized parameters are needed
 *
 * (needed to specify temp + offset var)
 */
bool
CL4BEWaitFunction::HasVariableSizedParameters(DIRECTION_TYPE nDirection)
{
    bool bRet = CBEWaitFunction::HasVariableSizedParameters(nDirection);
    // if we have indirect strings to marshal then we need the offset vars
    if (GetParameterCount(ATTR_REF, ATTR_NONE, nDirection))
        return true;
    return bRet;
}

