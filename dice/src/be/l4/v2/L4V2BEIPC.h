/**
 *    \file    dice/src/be/l4/v2/L4V2BEIPC.h
 *    \brief   contains the declaration of the class CL4V2BEIPC
 *
 *    \date    04/18/2006
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

/** preprocessing symbol to check header file */
#ifndef __L4V2BEIPC_H
#define __L4V2BEIPC_H

#include <be/l4/L4BEIPC.h>

/** \class CL4V2BEIPC
 *  \ingroup backend
 *  \brief combines all the IPC code writing
 *
 * This class contains the writing of IPC code -> it provide a central place
 * to change IPC bindings.
 */
class CL4V2BEIPC : public CL4BEIPC
{
public:
    /** create a new IPC object */
    CL4V2BEIPC();
    virtual ~CL4V2BEIPC();

    virtual void WriteCall(CBEFile *pFile, CBEFunction* pFunction);
    virtual void WriteReceive(CBEFile *pFile, CBEFunction* pFunction);
    virtual void WriteSend(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteReply(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteWait(CBEFile* pFile, CBEFunction *pFunction);

    virtual void WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, 
	bool bSendFlexpage, bool bSendShortIPC);
    virtual void WriteInitialization(CBEFile* pFile, CBEFunction* pFunction);
    virtual void WriteBind(CBEFile *pFile, CBEFunction* pFunction);
    virtual void WriteCleanup(CBEFile* pFile, CBEFunction* pFunction);

    virtual bool AddLocalVariable(CBEFunction *pFunction);

protected:
    virtual bool IsShortIPC(CBEFunction *pFunction, 
	int nDirection = 0);
    virtual bool UseAssembler(CBEFunction *pFunction);
};

#endif
