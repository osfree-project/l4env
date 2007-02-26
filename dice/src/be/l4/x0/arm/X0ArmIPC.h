/**
 *    \file    dice/src/be/l4/x0/arm/X0ArmIPC.h
 *    \brief   contains the declaration of the class CX0ArmIPC
 *
 *    \date    08/13/2002
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
#ifndef X0ARMIPC_H
#define X0ARMIPC_H


#include <be/l4/x0/L4X0BEIPC.h>

/** \class CX0ArmIPC
 *  \ingroup backend
 *  \brief encapsulates the L4 X0 specific IPC code for the ARM architecture
 */
class CX0ArmIPC : public CL4X0BEIPC
{

public:
    /** creates an IPC object */
    CX0ArmIPC();
    virtual ~CX0ArmIPC();

public:
    virtual void WriteCall(CBEFile* pFile,  CBEFunction* pFunction,
	bool bSendFlexpage, bool bSendShortIPC);
    virtual void WriteReceive(CBEFile* pFile,  CBEFunction* pFunction);
    virtual void WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,
	bool bSendFlexpage,  bool bSendShortIPC);
    virtual void WriteWait(CBEFile* pFile,  CBEFunction* pFunction);
    virtual void WriteSend(CBEFile* pFile,  CBEFunction* pFunction);
    virtual void WriteReply(CBEFile* pFile,  CBEFunction* pFunction);
};

#endif
