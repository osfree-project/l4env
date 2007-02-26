/**
 *    \file    dice/src/be/l4/v4/L4V4BESndFunction.cpp
 *    \brief   contains the implementation of the class CL4V4BESndFunction
 *
 *    \date    06/11/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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

#include "L4V4BESndFunction.h"
#include "L4V4BENameFactory.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BESizes.h"
#include "be/BEMsgBuffer.h"
#include "be/BEUserDefinedType.h"
#include "TypeSpec-L4V4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4V4BESndFunction::CL4V4BESndFunction()
 : CL4BESndFunction()
{
}

/** destroy the object of this class */
CL4V4BESndFunction::~CL4V4BESndFunction()
{
}

/** \brief initialize instance of class
 *  \param pFEOperation the front-end function to use as reference
 *  \return true if successful
 */
void 
CL4V4BESndFunction::CreateBackEnd(CFEOperation *pFEOperation)
{
    // do not call direct base class (it adds the result var only)
    CBESndFunction::CreateBackEnd(pFEOperation);

    // add local variables
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
    try
    {
	AddLocalVariable(sType, sMsgTag, 0, string("L4_MsgTag()"));
    }
    catch (CBECreateException *e)
    {
	e->Print();
	delete e;

	string exc = string(__func__);
	exc += " failed, because local variable (" + sMsgTag + 
	    ") could not be added.";
	throw new CBECreateException(exc);
    }
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *
 * Simply marshal the message. V4 specific is to put the message after
 * marshalling into the message registers.
 */
void
CL4V4BESndFunction::WriteMarshalling(CBEFile * pFile)
{
    CL4BESndFunction::WriteMarshalling(pFile);

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    // first clear message tag
    *pFile << "\tL4_MsgClear ( (L4_Msg_t *) &" << sMsgBuffer << " );\n";
    // load msgtag into message buffer
    *pFile << "\tL4_Set_MsgLabel ( (L4_Msg_t*) &" << sMsgBuffer << ", " <<
        m_sOpcodeConstName << " );\n";
    // set dopes
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND, 
	GetSendDirection());
    // load the message into the UTCB
    *pFile << "\tL4_MsgLoad ( (L4_Msg_t*) &" << sMsgBuffer << " );\n";
}

/** \brief marshals the exception
 *  \param pFile the file to write to
 *  \param bMarshal true if marshalling, false if unmarshalling
 *  \return the number of bytes used to marshal the exception
 *
 * Because for V4 the exception is member of the message buffer and never
 * skipped, we make this function empty because it is called explicetly in
 * CBESndFunction::WriteUnmarshalling. The exception is (un)marshalled together
 * with the "normal" parameters
 */
void
CL4V4BESndFunction::WriteMarshalException(CBEFile* /*pFile*/,
    bool /*bMarshal*/)
{
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * Because this is the call function, we can use the IPC call of L4.
 */
void
CL4V4BESndFunction::WriteInvocation(CBEFile * pFile)
{
    // invocate
    WriteIPC(pFile);
    // check for errors
    WriteIPCErrorCheck(pFile);
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
CL4V4BESndFunction::WriteIPCErrorCheck(CBEFile * pFile)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();

    *pFile << "\tif (L4_IpcFailed (" << sResult << "))\n" <<
              "\t{\n";
    pFile->IncIndent();
    // env.major = CORBA_SYSTEM_EXCEPTION;
    // env.repos_id = DICE_IPC_ERROR;
    string sSetFunc;
    if (((CBEUserDefinedType*)GetEnvironment()->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";
    *pFile << "\t" << sSetFunc << " (";
    if (pDecl->GetStars() == 0)
        *pFile << "&";
    pDecl->WriteName(pFile);
    *pFile << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_SYSTEM_EXCEPTION,\n" <<
              "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n" <<
              "\t0);\n";
    pFile->DecIndent();
    // env.ipc_error = L4_IPC_ERROR(result);
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    *pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_ErrorCode();\n";
    // return
    WriteReturn(pFile);
    // close }
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * In V4 we do have the exception in the msgbuf but not the opcode, which is
 * in the tag's label.
 */
int CL4V4BESndFunction::GetSize(int nDirection)
{
    // get base class' size
    int nSize = CBESndFunction::GetSize(nDirection);
    if (nDirection & DIRECTION_IN)
        nSize -= CCompiler::GetSizes()->GetOpcodeSize();
    return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * In V4 we do have the exception in the msgbuf but not the opcode, which is
 * in the tag's label.
 */
int CL4V4BESndFunction::GetFixedSize(int nDirection)
{
    int nSize = CBESndFunction::GetFixedSize(nDirection);
    if (nDirection & DIRECTION_IN)
        nSize -= CCompiler::GetSizes()->GetOpcodeSize();
    return nSize;
}

