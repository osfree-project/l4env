/**
 *    \file    dice/src/be/l4/v4/L4V4BEIPC.h
 *  \brief   contains the declaration of the class CL4V4BEIPC
 *
 *    \date    01/08/2004
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

/** preprocessing symbol to check header file */
#ifndef L4V4BEIPC_H
#define L4V4BEIPC_H


#include <be/l4/L4BEIPC.h>

/** \class CL4V4BEIPC
 *  \ingroup backend
 *  \brief encapsulates the V4 specific IPC invocation code
 */
class CL4V4BEIPC : public CL4BEIPC
{

public:
    /** constructor of CL4V4BEIPC class */
    CL4V4BEIPC();
    virtual ~CL4V4BEIPC();

public:
    virtual void WriteCall(CBEFile& pFile,  CBEFunction* pFunction);
    virtual void WriteReceive(CBEFile& pFile,  CBEFunction* pFunction);
    virtual void WriteReplyAndWait(CBEFile& pFile,  CBEFunction* pFunction,
	bool bSendFlexpage,  bool bSendShortIPC);
    virtual void WriteSend(CBEFile& pFile,  CBEFunction* pFunction);
    virtual void WriteWait(CBEFile& pFile,  CBEFunction* pFunction);
    virtual void WriteReply(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteInitialization(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteBind(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteCleanup(CBEFile& pFile, CBEFunction* pFunction);
};

#endif
