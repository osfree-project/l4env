/**
 *    \file    dice/src/be/l4/fiasco/L4FiascoBEIPC.h
 *    \brief   contains the declaration of the class CL4FiascoBEIPC
 *
 *    \date    08/20/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __L4FiascoBEIPC_H
#define __L4FiascoBEIPC_H

#include <be/l4/L4BEIPC.h>

/** \class CL4FiascoBEIPC
 *  \ingroup backend
 *  \brief combines all the IPC code writing
 *
 * This class contains the writing of IPC code -> it provide a central place
 * to change IPC bindings.
 */
class CL4FiascoBEIPC : public CL4BEIPC
{
public:
    /** create a new IPC object */
    CL4FiascoBEIPC();
    virtual ~CL4FiascoBEIPC();

    virtual void WriteCall(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteReceive(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteSend(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteReply(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteWait(CBEFile& pFile, CBEFunction *pFunction);

    virtual void WriteReplyAndWait(CBEFile& pFile, CBEFunction* pFunction,
	bool bSendFlexpage, bool bSendShortIPC);
    virtual void WriteInitialization(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteBind(CBEFile& pFile, CBEFunction* pFunction);
    virtual void WriteCleanup(CBEFile& pFile, CBEFunction* pFunction);

    virtual void AddLocalVariable(CBEFunction *pFunction);

protected:
    virtual bool IsShortIPC(CBEFunction *pFunction, CMsgStructType nType);
    virtual bool UseAssembler(CBEFunction *pFunction);
    virtual void WriteTag(CBEFile& pFile, CBEFunction *pFunction, bool bReference);
};

#endif
