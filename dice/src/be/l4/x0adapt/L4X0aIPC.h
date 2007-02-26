/**
 *	\file	dice/src/be/l4/x0adapt/L4X0aIPC.h
 *	\brief	contains the declaration of the class CL4X0aIPC
 *
 *	\date	08/14/2002
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
#ifndef L4X0AIPC_H
#define L4X0AIPC_H

#include <be/l4/L4BEIPC.h>

/** \class CL4X0aIPC
 *  \ingroup backend
 *  \brief encloses all L4 X0 adaption IPC code
 */
class CL4X0aIPC : public CL4BEIPC
{
DECLARE_DYNAMIC(CL4X0aIPC);
public:
    /** creates an IPC object */
    CL4X0aIPC();
    ~CL4X0aIPC();

public:
    virtual void WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext);
	virtual void WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext);
	virtual void WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext);
	virtual void WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext);
	virtual void WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext);

protected:
    virtual bool UseAssembler(CBEFunction* pFunction,  CBEContext* pContext);

	virtual void WriteAsmShortWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmShortFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmShortReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmShortCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmShortSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmShortReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
	virtual void WriteAsmLongReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext);
};

#endif
