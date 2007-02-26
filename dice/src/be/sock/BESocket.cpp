/**
 *    \file    dice/src/be/sock/BESocket.cpp
 *    \brief   contains the declaration of the class CBESocket
 *
 *    \date    08/18/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/BEMsgBufferType.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"

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
 *  \param nDirection the direction to use for calculating send sizes
 *  \param sFunc the name of the calling function
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteSendTo(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, int nDirection, const char* sFunc, CBEContext* pContext)
{
    string sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSize = pMsgBuffer->GetCount(TYPE_FIXED, nDirection);

    pFile->PrintIndent("dice_ret_size = sendto(");
    WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
    pFile->Print(", %s, ", sMsgBuffer.c_str());

    if (pMsgBuffer->IsVariableSized(nDirection))
        pFile->Print("%s", sOffset.c_str());
    else
        pFile->Print("%d", nSize);

/*    if (pMsgBuffer->IsVariableSized())
        pFile->Print("%s", sOffset.c_str());
    else
        // we cannot use type->cast because this prints a cast to a simple character
        pFile->Print("sizeof(%s)", sMsgBuffer.c_str());
 */
    pFile->Print(", 0, (struct sockaddr*)%s, dice_fromlen);\n", sCorbaObj.c_str());

    pFile->PrintIndent("if (dice_ret_size < ");
    if (pMsgBuffer->IsVariableSized(nDirection))
        pFile->Print("%s", sOffset.c_str());
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
    string sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->PrintIndent("dice_ret_size = recvfrom(");
    WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
    pFile->Print(", %s, ", sMsgBuffer.c_str());
    if (pMsgBuffer->IsVariableSized() && !bUseMaxSize)
        pFile->Print("%s", sOffset.c_str());
    else
    {
        // we cannot use type->cast because this prints a cast to a simple character
        pFile->Print("sizeof");
        if (bUseMaxSize)
            pMsgBuffer->GetType()->WriteCast(pFile, false, pContext);
        else
            pFile->Print("(%s)", sMsgBuffer.c_str());
    }
    pFile->Print(", 0, (struct sockaddr*)%s, &dice_fromlen);\n", sCorbaObj.c_str());
    pFile->PrintIndent("if (dice_ret_size < 0)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"%s\");\n", sFunc);
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
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
            vector<CBEDeclarator*>::iterator iterE = pEnv->GetFirstDeclarator();
            pEnvDecl = *iterE;
        }
    }
    if (pEnvDecl)
    {
        if (pEnvDecl->GetStars())
            pFile->Print("%s->cur_socket", pEnvDecl->GetName().c_str());
        else
            pFile->Print("%s.cur_socket", pEnvDecl->GetName().c_str());
    }
    else
        pFile->Print("sd");
}

/** \brief writes the call implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    // send message
    WriteSendTo(pFile, pFunction, false, 0/* both directions*/, "call", pContext);
    // offset might have been overwritten, so it has to be reinitialized
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    if (pMsgBuffer->IsVariableSized())
        pMsgBuffer->WriteInitialization(pFile, pContext);
    // zero msgbuffer
    WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // receive response
    WriteReceiveFrom(pFile, pFunction, false, false, "call", pContext);
}

/** \brief writes the reply-and-wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    // send message
    WriteSendTo(pFile, pFunction, true, pFunction->GetSendDirection(), "reply-wait", pContext);
    // reset msg buffer to zeros
    WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // want to receive from any client again
    string sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("%s->sin_addr.s_addr = INADDR_ANY;\n",
        sCorbaObj.c_str());
    // wait for new request
    WriteReceiveFrom(pFile, pFunction, true, true, "reply-wait", pContext);
}

/** \brief zeros the message buffer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteZeroMsgBuffer(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    // zero out msg buffer for response
    // msgbuffer is always a pointer: either variable sized or char[]
    pFile->PrintIndent("bzero(%s, ", sMsgBuffer.c_str());
    if (pMsgBuffer->IsVariableSized())
        pFile->Print("%s);\n", sOffset.c_str());
    else
        pFile->Print("sizeof(%s));\n", sMsgBuffer.c_str());
}

/** \brief writes the wait implementation of the socket layer
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CBESocket::WriteWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    WriteZeroMsgBuffer(pFile, pFunction, pContext);
    // wait for new request
    WriteReceiveFrom(pFile, pFunction, true, true, "wait", pContext);
}

/**    \brief writes the initialization
 *    \param pFile the file to write to
 *    \param pFunction the funtion to write for
 *    \param pContext the context of the writing
 */
void CBESocket::WriteInitialization(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
    bool bUseEnv = pFunction->IsComponentSide();
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

/**    \brief writes the assigning of a local name to a communication port
 *    \param pFile the file to write to
 *    \param pFunction the funtion to write for
 *    \param pContext the context of the writing
 */
void CBESocket::WriteBind(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
    bool bUseEnv = pFunction->IsComponentSide();
    string sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);

    pFile->PrintIndent("if (bind(");
    WriteSocketDescriptor(pFile, pFunction, bUseEnv, pContext);
    pFile->Print(", (struct sockaddr*)%s, sizeof(struct sockaddr)) < 0)\n",
            sCorbaObj.c_str());
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"bind\");\n");
    pFunction->WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/**    \brief writes the clean up code
 *    \param pFile the file to write to
 *    \param pFunction the funtion to write for
 *    \param pContext the context of the writing
 */
void CBESocket::WriteCleanup(CBEFile *pFile,
    CBEFunction *pFunction,
    CBEContext *pContext)
{
    pFile->PrintIndent("close(");
    WriteSocketDescriptor(pFile, pFunction, pFunction->IsComponentSide(), pContext);
    pFile->Print(");\n");
}
