/**
 *	\file	dice/src/be/lsock/BESocket.h
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
DECLARE_DYNAMIC(CBESocket);

public:
    /** brief creates a socket object */
    CBESocket();
    ~CBESocket();

public:
    virtual void CreateSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
	virtual void BindSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
    virtual void CloseSocket(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);

	virtual void WriteCall(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
	virtual void WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
	virtual void WriteReplyAndRecv(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
	virtual void WriteWait(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);

protected:
    virtual void WriteSocketDescriptor(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, CBEContext* pContext);
	virtual void WriteZeroMsgBuffer(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteSendTo(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, int nDirection, const char* sFunc, CBEContext* pContext);
	virtual void WriteReceiveFrom(CBEFile* pFile, CBEFunction* pFunction, bool bUseEnv, bool bUseMaxSize, const char* sFunc, CBEContext* pContext);
};

#endif
