/**
 *	\file	dice/src/be/sock/BESocket.cpp
 *	\brief	contains the declaration of the class CBESocket
 *
 *	\date	08/18/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "be/BEMsgBufferType.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"

IMPLEMENT_DYNAMIC(CBESocket);

CBESocket::CBESocket()
 : CBECommunication()
{
    IMPLEMENT_DYNAMIC_BASE(CBESocket, CBECommunication);
}

/** destroys the socket object */
CBESocket::~CBESocket()
{
}

/** \brief creates the socket before communication
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment variable contains socket descriptor
 *  \param pContext the context of the write operation
 */
void CBESocket::CreateSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    pFile->PrintIndent("");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
	pFile->Print(" = socket(PF_INET, SOCK_DGRAM, 0);\n");
    pFile->PrintIndent("if (");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
	pFile->Print(" < 0)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"socket creation\");\n");
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the sending of the message
 *  \param pFile the file to write to
 *  \param pFunction the function to write to
 *  \param bUseEnv true if socket is stroed in environment (otherwise 'sd' is socket descriptor)
 *  \param nDirection the direction to use for calculating send sizes
 *  \param sFunc the name of the calling function
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteSendTo(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, int nDirection, const char* sFunc, CBEContext* pContext)
{
	String sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);

	CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
	int nSize = pMsgBuffer->GetFixedCount(nDirection);

	pFile->PrintIndent("dice_ret_size = sendto(");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
	pFile->Print(", %s, ", (const char*)sMsgBuffer);

	if (pMsgBuffer->IsVariableSized(nDirection))
        pFile->Print("%s", (const char*)sOffset);
    else
        pFile->Print("%d", nSize);

/*    if (pMsgBuffer->IsVariableSized())
        pFile->Print("%s", (const char*)sOffset);
    else
	    // we cannot use type->cast because this prints a cast to a simple character
        pFile->Print("sizeof(%s)", (const char*)sMsgBuffer);
 */
    pFile->Print(", 0, (struct sockaddr*)%s, dice_fromlen);\n", (const char*)sCorbaObj);

	pFile->PrintIndent("if (dice_ret_size < ");
	if (pMsgBuffer->IsVariableSized(nDirection))
        pFile->Print("%s", (const char*)sOffset);
    else
        pFile->Print("%d", nSize);
	pFile->Print(")\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"%s\");\n", sFunc);
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the receiving of a message
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment contains socket descriptor
 *  \param bUseMaxSize true if the variable sized message buffer should be ignored
 *  \param sFunc the name of the calling function
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteReceiveFrom(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, bool bUseMaxSize, const char* sFunc, CBEContext* pContext)
{
	String sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
	CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

	pFile->PrintIndent("dice_ret_size = recvfrom(");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
	pFile->Print(", %s, ", (const char*)sMsgBuffer);
    if (pMsgBuffer->IsVariableSized() && !bUseMaxSize)
        pFile->Print("%s", (const char*)sOffset);
    else
	{
	    // we cannot use type->cast because this prints a cast to a simple character
        pFile->Print("sizeof");
		if (bUseMaxSize)
			pMsgBuffer->GetType()->WriteCast(pFile, false, pContext);
		else
		    pFile->Print("(%s)", (const char*)sMsgBuffer);
	}
    pFile->Print(", 0, (struct sockaddr*)%s, &dice_fromlen);\n", (const char*)sCorbaObj);
    pFile->PrintIndent("if (dice_ret_size < 0)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"%s\");\n", sFunc);
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the code to close the socket
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment contains socket descriptor
 *  \param pContext the context of teh write operation
 */
void CBESocket::CloseSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    pFile->PrintIndent("close(");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
	pFile->Print(");\n");
}

/** \brief prints the socket descriptor to the target file
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment variable should be used
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteSocketDescriptor(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    CBEDeclarator *pEnvDecl = 0;
    if (bUseEnv)
	{
	    CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
		if (pEnv)
		{
			VectorElement *pIter = pEnv->GetFirstDeclarator();
			pEnvDecl = pEnv->GetNextDeclarator(pIter);
		}
	}
	if (pEnvDecl)
	{
		if (pEnvDecl->GetStars())
			pFile->Print("%s->cur_socket", (const char*)pEnvDecl->GetName());
		else
			pFile->Print("%s.cur_socket", (const char*)pEnvDecl->GetName());
	}
	else
		pFile->Print("sd");
}

/** \brief binds the socket to an address
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if environment contains socket descriptor
 *  \param pContext the context of the write operation
 */
void CBESocket::BindSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
	String sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);

	pFile->PrintIndent("if (bind(");
	WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
    pFile->Print(", (struct sockaddr*)%s, sizeof(struct sockaddr)) < 0)\n",
            (const char*)sCorbaObj);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"bind\");\n");
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes the call implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if socket descriptor if part of the environment
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteCall(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    // send message
	WriteSendTo(pFile, pFunction, bUseEnv, 0/* both directions*/, "call", pContext);
    // offset might have been overwritten, so it has to be reinitialized
	CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    if (pMsgBuffer->IsVariableSized())
        pMsgBuffer->WriteInitialization(pFile, pContext);
	// zero msgbuffer
	WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // receive response
	WriteReceiveFrom(pFile, pFunction, bUseEnv, false, "call", pContext);
}

/** \brief writes the reply-and-recv implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if socket descriptor if part of the environment
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteReplyAndRecv(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    // send message
	WriteSendTo(pFile, pFunction, bUseEnv, pFunction->GetSendDirection(), "reply-recv", pContext);
	// zero msgbuffer
	WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // receive response
	WriteReceiveFrom(pFile, pFunction, bUseEnv, true, "reply-recv", pContext);
}

/** \brief writes the reply-and-wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if socket descriptor if part of the environment
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    // send message
    WriteSendTo(pFile, pFunction, bUseEnv, pFunction->GetSendDirection(), "reply-wait", pContext);
    // reset msg buffer to zeros
	WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // want to receive from any client again
	String sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("%s->sin_addr.s_addr = INADDR_ANY;\n",
        (const char*)sCorbaObj);
    // wait for new request
	WriteReceiveFrom(pFile, pFunction, bUseEnv, true, "reply-wait", pContext);
}

/** \brief zeros the message buffer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteZeroMsgBuffer(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
	CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    // zero out msg buffer for response
	// msgbuffer is always a pointer: either variable sized or char[]
    pFile->PrintIndent("bzero(%s, ", (const char*)sMsgBuffer);
    if (pMsgBuffer->IsVariableSized())
        pFile->Print("%s);\n", (const char*)sOffset);
    else
        pFile->Print("sizeof(%s));\n", (const char*)sMsgBuffer);
}

/** \brief writes the wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bUseEnv true if socket descriptor if part of the environment
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteWait(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext)
{
    WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // wait for new request
	WriteReceiveFrom(pFile, pFunction, bUseEnv, true, "wait", pContext);
}
