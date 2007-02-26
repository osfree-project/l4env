/**
 *    \file    dice/src/be/l4/L4BEIPC.h
 *    \brief   contains the declaration of the class CL4BEIPC
 *
 *    \date    02/07/2002
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
#ifndef L4BEIPC_H
#define L4BEIPC_H

#include "be/BECommunication.h"

/**    \def COMM_PROP_USE_ASM
 *    \brief check if IPC cann support asm
 */
#define COMM_PROP_USE_ASM    1

/**    \class CL4BEIPC
 *  \ingroup backend
 *  \brief combines all the IPC code writing
 *
 * This class contains the writing of IPC code -> it provide a central place
 * to change IPC bindings.
 */
class CL4BEIPC : public CBECommunication
{


public:
    /** create a new IPC object */
    CL4BEIPC();
    virtual ~CL4BEIPC();

    virtual void WriteCall(CBEFile *pFile, CBEFunction* pFunction, CBEContext *pContext);
    virtual void WriteReceive(CBEFile *pFile, CBEFunction* pFunction, CBEContext *pContext);
    virtual void WriteSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
    virtual void WriteReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
    virtual void WriteWait(CBEFile* pFile, CBEFunction *pFunction, CBEContext* pContext);

    virtual void WriteReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, bool bSendFlexpage, bool bSendShortIPC, CBEContext* pContext);

    bool CheckProperty(CBEFunction *pFunction, int nProperty, CBEContext *pContext);

protected:
    virtual bool IsShortIPC(CBEFunction *pFunction, CBEContext *pContext, int nDirection = 0);
    virtual bool UseAssembler(CBEFunction *pFunction, CBEContext *pContext);
};

#endif
