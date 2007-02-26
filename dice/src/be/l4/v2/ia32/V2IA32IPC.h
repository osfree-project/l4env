/**
 *    \file    dice/src/be/l4/v2/ia32/V2IA32IPC.h
 *    \brief   contains the declaration of the class CL4V2IA32BEIPC
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
#ifndef __L4V2IA32BEIPC_H
#define __L4V2IA32BEIPC_H

#include "be/l4/v2/L4V2BEIPC.h"

/** \class CL4V2IA32BEIPC
 *  \ingroup backend
 *  \brief contains code writing rules for V2 specific IPC code
 */
class CL4V2IA32BEIPC : public CL4V2BEIPC
{

public:
    /** create a new IPC object */
    CL4V2IA32BEIPC();
    virtual ~CL4V2IA32BEIPC();

    virtual bool UseAssembler(CBEFunction* pFunction);
    virtual void WriteCall(CBEFile * pFile,  CBEFunction * pFunction);
    virtual void WriteSend(CBEFile* pFile,  CBEFunction* pFunction);
    virtual void WriteReply(CBEFile* pFile,  CBEFunction* pFunction);

protected:
    virtual void WriteAsmLongCall(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmLongPicCall(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmLongNonPicCall(CBEFile *pFile,
	CBEFunction *pFunction);
    
    virtual void WriteAsmShortCall(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmShortPicCall(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmShortNonPicCall(CBEFile *pFile,
	CBEFunction *pFunction);
    
    virtual void WriteAsmSend(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmPicSend(CBEFile *pFile, CBEFunction *pFunction);
    virtual void WriteAsmNonPicSend(CBEFile *pFile, CBEFunction *pFunction);
};

#endif
