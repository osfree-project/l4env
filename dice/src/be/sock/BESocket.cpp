/**
 *  \file    dice/src/be/sock/BESocket.cpp
 *  \brief   contains the declaration of the class CBESocket
 *
 *  \date    08/18/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/sock/BESocket.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BEMsgBuffer.h"
#include "be/BEDeclarator.h"
#include "be/BENameFactory.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"

CBESocket::CBESocket()
 : CBECommunication()
{
}

/** destroys the socket object */
CBESocket::~CBESocket()
{
}

/** \brief writes the sending of the message
 *  \param pFile the file to write to
 *  \param pFunction the function to write to
 *  \param bUseEnv true if socket is stroed in environment (otherwise 'sd' is socket descriptor)
 *  \param sFunc the name of the calling function
 */
void
CBESocket::WriteSendTo(CBEFile* pFile,
    CBEFunction* pFunction,
    bool bUseEnv,
    const char* sFunc)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sCorbaObj = pNF->GetCorbaObjectVariable();

    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    string sPtrName, sSizeName;
    if (pMsgBuffer->m_Declarators.First()->GetStars() == 0)
	sPtrName = "&";
    else
	sSizeName = "*";
    sPtrName += pMsgBuffer->m_Declarators.First()->GetName();
    sSizeName += pMsgBuffer->m_Declarators.First()->GetName();

    *pFile << "\tdice_ret_size = sendto (";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << ", " << sPtrName << ", sizeof(" << sSizeName << "), 0, " <<
	"(struct sockaddr*)" << sCorbaObj << ", dice_fromlen);\n";

    *pFile << "\tif (dice_ret_size < sizeof(" << sSizeName << "))\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tperror(\"" << sFunc << "\");\n";
    pFunction->WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief writes the receiving of a message
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment contains socket descriptor
 */
void
CBESocket::WriteReceiveFrom(CBEFile* pFile,
    CBEFunction* pFunction,
    bool bUseEnv)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sCorbaObj = pNF->GetCorbaObjectVariable();
    string sOffset = pNF->GetOffsetVariable();
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    string sPtrName, sSizeName;
    if (pMsgBuffer->m_Declarators.First()->GetStars() == 0)
	sPtrName = "&";
    else
	sSizeName = "*";
    sPtrName += pMsgBuffer->m_Declarators.First()->GetName();
    sSizeName += pMsgBuffer->m_Declarators.First()->GetName();

    *pFile << "\tdice_ret_size = recvfrom(";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << ", " << sPtrName << ", sizeof(" << sSizeName << "), 0, " <<
	"(struct sockaddr*)" << sCorbaObj << ", &dice_fromlen);\n";
}

/** \brief writes code to clear the currently-set exception
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteExceptionClear(CBEFile* pFile,
    CBEFunction* pFunction)
{
    /* write exception check (clear any existing exception) */
    *pFile << "\tif (";
    WriteEnvironmentField(pFile, pFunction, "_exception._corba.major");
    *pFile << " != CORBA_NO_EXCEPTION)\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_server_exception_set(";
    WriteEnvironment(pFile, pFunction);
    *pFile << ",\n";
    *pFile << "\t\tCORBA_NO_EXCEPTION,\n";
    *pFile << "\t\tCORBA_DICE_EXCEPTION_NONE,\n";
    *pFile << "\t\t0);\n";
    pFile->DecIndent();
}

/** \brief writes error-handling, as should follow a recvfrom() call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param sFunc the name of the calling function
 */
void CBESocket::WriteErrorCheck(CBEFile* pFile,
    CBEFunction* pFunction, const char* sFunc)
{
    *pFile << "\tif (dice_ret_size < 0)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    /* set exception */
    *pFile << "\tif (";
    WriteEnvironmentField(pFile, pFunction, "_exception._corba.major");
    *pFile << " == CORBA_NO_EXCEPTION)\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_server_exception_set(";
    WriteEnvironment(pFile, pFunction);
    *pFile << ",\n";
    *pFile << "\t\tCORBA_SYSTEM_EXCEPTION,\n";
    *pFile << "\t\tCORBA_DICE_INTERNAL_IPC_ERROR,\n";
    *pFile << "\t\t0);\n";
    pFile->DecIndent();

    /* don't perror if the error was a timeout */
    *pFile << "\tif (errno != EAGAIN) perror (\"" << sFunc << "\");\n";	
    pFunction->WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief prints the socket descriptor to the target file
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment variable should be used
 */
void CBESocket::WriteSocketDescriptor(CBEFile* pFile,
    CBEFunction* pFunction,
    bool bUseEnv)
{
    if (bUseEnv)
    {
	if (WriteEnvironmentField(pFile, pFunction, "cur_socket"))
	{
	    /* WriteEnvironmentField succeeded, so we can return */
	    return;
	}
    }
    /* either bUseEnv is false, or WriteEnvironmentField failed */
    *pFile << "sd";
}

/** \brief prints an environment field expression to the target file
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param sFieldName the name of the field in the environment
 */
bool CBESocket::WriteEnvironmentField(CBEFile* pFile, CBEFunction* pFunction,
    const char* sFieldName)
{
    CBEDeclarator* pEnvDecl = 0;
    CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
    /* if we have an environment, try getting the first declarator */
    if (pEnv && (pEnvDecl = pEnv->m_Declarators.First()))
    {
	/* we have both environment and declarator, so
	 * print the field name */
	*pFile << pEnvDecl->GetName();
	*pFile << ((pEnvDecl->GetStars() != 0) ? "->" : ".");
	*pFile << sFieldName;
	return true;
    }
    else return false;
}

/** \brief prints the environment name to the target file
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteEnvironment(CBEFile* pFile, CBEFunction* pFunction)
{
    CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
    CBEDeclarator* pEnvDecl = (pEnv) ? pEnv->m_Declarators.First() : 0;
    if (pEnvDecl)
    {
	if (pEnvDecl->GetStars() == 0) *pFile << "&";
	pEnvDecl->WriteName(pFile);
    }
    else
    {
	/* there is no environment? */
    }
}

/** \brief writes the call to set the socket receive timeout
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteTimeoutOptionCall(CBEFile* pFile, CBEFunction* pFunction,
    bool bUseEnv)
{
    /* first, write the setsockopt call to enable timeouts 
     * TODO: move this code into a separate function */
    *pFile << "\tsetsockopt(";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << ", SOL_SOCKET, SO_RCVTIMEO, (void*) &";
    WriteEnvironmentField(pFile, pFunction, "receive_timeout");
    *pFile << ", sizeof (struct timeval));\n";
}

/** \brief writes the call implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteCall(CBEFile* pFile, CBEFunction* pFunction)
{
    // send message
    WriteSendTo(pFile, pFunction, false, "call");
    // offset might have been overwritten, so it has to be reinitialized
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    if (pMsgBuffer->IsVariableSized(0))
	pMsgBuffer->WriteInitialization(pFile, pFunction, 0, 0);
    // zero msgbuffer
    WriteZeroMsgBuffer(pFile, pFunction);
    // receive response
    WriteReceiveFrom(pFile, pFunction, false);
    WriteErrorCheck(pFile, pFunction, "call");
}

/** \brief writes the reply-and-wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction)
{
    // send message
    WriteSendTo(pFile, pFunction, true, "reply-wait");
    // reset msg buffer to zeros
    WriteZeroMsgBuffer(pFile, pFunction);
    // want to receive from any client again
    string sCorbaObj = CCompiler::GetNameFactory()->GetCorbaObjectVariable();
    *pFile << "\t" << sCorbaObj << "->sin_addr.s_addr = INADDR_ANY;\n";
    // wait for new request, with receive timeout
    WriteTimeoutOptionCall(pFile, pFunction, true);
    WriteReceiveFrom(pFile, pFunction, true);
    WriteExceptionClear(pFile, pFunction);
    WriteErrorCheck(pFile, pFunction, "reply-wait");
}

/** \brief zeros the message buffer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteZeroMsgBuffer(CBEFile* pFile,
    CBEFunction* pFunction)
{
    string sOffset = CCompiler::GetNameFactory()->GetOffsetVariable();
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    // msgbuffer is always a pointer: either variable sized or char[]
    string sPtrName, sSizeName;
    if (pMsgBuffer->m_Declarators.First()->GetStars() == 0)
	sPtrName = "&";
    else
	sSizeName = "*";
    sPtrName += pMsgBuffer->m_Declarators.First()->GetName();
    sSizeName += pMsgBuffer->m_Declarators.First()->GetName();

    *pFile << "\tbzero (" << sPtrName << ", sizeof(" << sSizeName << ") );\n";
}

/** \brief writes the wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBESocket::WriteWait(CBEFile* pFile, CBEFunction* pFunction)
{
    WriteZeroMsgBuffer(pFile, pFunction);
    // wait for new request, using timeout
    WriteTimeoutOptionCall(pFile, pFunction, true);
    WriteReceiveFrom(pFile, pFunction, true);
    WriteExceptionClear(pFile, pFunction);
    WriteErrorCheck(pFile, pFunction, "wait");
}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void CBESocket::WriteInitialization(CBEFile *pFile,
    CBEFunction *pFunction)
{
    bool bUseEnv = pFunction->IsComponentSide();

    *pFile << "\t";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << " = socket(PF_INET, SOCK_DGRAM, 0);\n";

    *pFile << "\tif (";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << " < 0)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tperror(\"socket creation\");\n";
    pFunction->WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief writes the assigning of a local name to a communication port
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void CBESocket::WriteBind(CBEFile *pFile,
    CBEFunction *pFunction)
{
    bool bUseEnv = pFunction->IsComponentSide();
    string sCorbaObj = CCompiler::GetNameFactory()->GetCorbaObjectVariable();

    *pFile << "\tif (bind(";
    WriteSocketDescriptor(pFile, pFunction, bUseEnv);
    *pFile << ", (struct sockaddr*)" << sCorbaObj << 
	", sizeof(struct sockaddr)) < 0)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tperror(\"bind\");\n";
    pFunction->WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief writes the clean up code
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void CBESocket::WriteCleanup(CBEFile *pFile,
    CBEFunction *pFunction)
{
    *pFile << "\tclose (";
    WriteSocketDescriptor(pFile, pFunction, pFunction->IsComponentSide());
    *pFile << ");\n";
}

/** \brief writes a send
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CBESocket::WriteSend(CBEFile* pFile,
    CBEFunction* pFunction)
{
    // send message
    WriteSendTo(pFile, pFunction, false, "send");
}

/** \brief writes a reply
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CBESocket::WriteReply(CBEFile* /*pFile*/,
    CBEFunction* /*pFunction*/)
{}

/** \brief writes a closed wait
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void 
CBESocket::WriteReceive(CBEFile* /*pFile*/,
    CBEFunction* /*pFunction*/)
{}

