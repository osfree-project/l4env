/**
 *  \file    dice/src/be/sock/BESocket.h
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

/** preprocessing symbol to check header file */
#ifndef BESOCKET_H
#define BESOCKET_H


#include <be/BECommunication.h>

/** \class CBESocket
 *  \ingroup backend
 *  \brief encapsulates the socket communication
 */
class CBESocket : public CBECommunication
{

public:
    /** brief creates a socket object */
    CBESocket();
    virtual ~CBESocket();

public:
    virtual void WriteCall(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteWait(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteReceive(CBEFile *pFile, CBEFunction* pFunction);
    virtual void WriteSend(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteReply(CBEFile* pFile, CBEFunction* pFunction);

    virtual void WriteInitialization(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteBind(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteCleanup(CBEFile *pFile, CBEFunction *pFunction);

protected:
    virtual void WriteSocketDescriptor(CBEFile* pFile, CBEFunction* pFunction,
	bool bUseEnv);
    virtual void WriteTimeoutOptionCall(CBEFile* pFile, CBEFunction* pFunction,
	bool bUseEnv);
    virtual bool WriteEnvironmentField(CBEFile* pFile, CBEFunction* pFunction,
	const char* sFieldName);
    virtual void WriteEnvironment(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteExceptionClear(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteErrorCheck(CBEFile* pFile, CBEFunction* pFunction,
	const char* sFieldName);
    virtual void WriteZeroMsgBuffer(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteSendTo(CBEFile* pFile, CBEFunction* pFunction,
	bool bUseEnv, const char* sFunc);
    virtual void WriteReceiveFrom(CBEFile* pFile, CBEFunction* pFunction,
	bool bUseEnv);
};

#endif

