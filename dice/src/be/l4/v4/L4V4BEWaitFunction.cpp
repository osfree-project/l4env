/**
 *    \file    dice/src/be/l4/v4/L4V4BEWaitFunction.cpp
 *    \brief   contains the implementation of the class CL4V4BEWaitFunction
 *
 *    \date    06/11/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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

#include "be/l4/v4/L4V4BEWaitFunction.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BESizes.h"
#include "be/BEMsgBuffer.h"
#include "be/BEUserDefinedType.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "Compiler.h"

CL4V4BEWaitFunction::CL4V4BEWaitFunction(bool bOpenWait)
 : CL4BEWaitFunction(bOpenWait)
{ }

/** destroy the object of this class */
CL4V4BEWaitFunction::~CL4V4BEWaitFunction()
{ }

/** \brief initialize instance of class
 *  \param pFEOperation the front-end function to use as reference
 *  \return true if successful
 */
void
CL4V4BEWaitFunction::CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide)
{
    // do not call direct base class (it adds the result var only)
    CBEWaitFunction::CreateBackEnd(pFEOperation, bComponentSide);

    // add local variables
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
    AddLocalVariable(sType, sMsgTag, 0, string("L4_MsgTag()"));
}

/** \brief write L4 specific unmarshalling code
 *  \param pFile the file to write to
 *
 * Skip the L4 specific check for received flexpages, simply unmarshal the
 * parameter. Before that we have to load the message registers into te
 * message buffer.
 */
void
CL4V4BEWaitFunction::WriteUnmarshalling(CBEFile& pFile)
{
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    // store message
    pFile << "\tL4_MsgStore ( " << sMsgTag << ", (L4_Msg_t*) &" << sMsgBuffer
	<< " );\n";

    CBEWaitFunction::WriteUnmarshalling(pFile);
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
CL4V4BEWaitFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
    CBENameFactory *pNF = CBENameFactory::Instance();
    string sResult = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();

    pFile << "\tif (L4_IpcFailed (" << sResult << "))\n" <<
              "\t{\n";
    // env.major = CORBA_SYSTEM_EXCEPTION;
    // env.repos_id = DICE_IPC_ERROR;
    string sSetFunc;
    if (((CBEUserDefinedType*)GetEnvironment()->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";
    ++pFile << "\t" << sSetFunc << " (";
    if (pDecl->GetStars() == 0)
        pFile << "&";
    pDecl->WriteName(pFile);
    pFile << ",\n";
    ++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
	pFile << "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n";
	pFile << "\t0);\n";
    // env.ipc_error = L4_IPC_ERROR(result);
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();
    --pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_ErrorCode();\n";
    // return
    WriteReturn(pFile);
    // close }
    --pFile << "\t}\n";
}

/** \brief writes opcode check code
 *  \param pFile the file to write to
 */
void
CL4V4BEWaitFunction::WriteOpcodeCheck(CBEFile& pFile)
{
    /* if the noopcode option is set, we cannot check for the correct opcode */
    if (m_Attributes.Find(ATTR_NOOPCODE))
        return;

    // unmarshal opcode variable
    WriteMarshalOpcode(pFile, false);

    // now check if opcode in variable is our opcode
    string sSetFunc;
    if (((CBEUserDefinedType*)GetEnvironment()->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";

    CBENameFactory *pNF = CBENameFactory::Instance();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    pFile << "\tif (L4_MsgLabel ( (L4_Msg_t*) &" << sMsgBuffer << ") != "
	<< m_sOpcodeConstName << ")\n";
    pFile << "\t{\n";
    string sException = pNF->GetCorbaEnvironmentVariable();
    ++pFile << "\t" << sSetFunc << "(" << sException << ",\n";
    ++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
    pFile << "\tCORBA_DICE_EXCEPTION_WRONG_OPCODE,\n";
    pFile << "\t0);\n";
    --pFile;
    WriteReturn(pFile);
    --pFile;
    pFile << "\t}\n";
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * In V4 we do have the exception in the msgbuf but not the opcode, which is
 * in the tag's label.
 */
int CL4V4BEWaitFunction::GetSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBEWaitFunction::GetSize(nDirection);
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
int CL4V4BEWaitFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEWaitFunction::GetFixedSize(nDirection);
    if (nDirection & DIRECTION_IN)
        nSize -= CCompiler::GetSizes()->GetOpcodeSize();
    return nSize;
}

