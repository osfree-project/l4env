/**
 *	\file	dice/src/be/l4/v2/L4V2BEIPC.h
 *	\brief	contains the declaration of the class CL4V2BEIPC
 *
 *	\date	08/13/2003
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
#ifndef L4V2BEIPC_H
#define L4V2BEIPC_H

#include "be/l4/L4BEIPC.h"

/** \class CL4V2BEIPC
 *  \ingroup backend
 *  \brief contains code writing rules for V2 specific IPC code
 */
class CL4V2BEIPC : public CL4BEIPC
{
DECLARE_DYNAMIC(CL4V2BEIPC);
public:
	CL4V2BEIPC();
	virtual ~CL4V2BEIPC();

    virtual bool UseAssembler(CBEFunction* pFunction,  CBEContext* pContext);
	virtual void WriteCall(CBEFile * pFile,  CBEFunction * pFunction,  CBEContext * pContext);
	virtual void WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext);

protected:
	virtual void WriteAsmLongCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext);
	virtual void WriteAsmShortCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext);
	virtual void WriteAsmShortSend(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext);
	virtual void WriteAsmLongSend(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext);
};

#endif
