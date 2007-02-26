/**
 *	\file	dice/src/be/l4/L4BEReplyAnyWaitAnyFunction.h
 *	\brief	contains the declaration of the class CL4BEReplyAnyWaitAnyFunction
 *
 *	\date	Wed Jun 12 2002
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

#ifndef L4BEREPLYANYWAITANYFUNCTION_H
#define L4BEREPLYANYWAITANYFUNCTION_H

#include <be/BEReplyAnyWaitAnyFunction.h>

/** \class CL4BEReplyAnyWaitAnyFunction
 *  \ingroup backend
 *  \brief implements L4 specific reply-and-wait function
 */
class CL4BEReplyAnyWaitAnyFunction : public CBEReplyAnyWaitAnyFunction
{
DECLARE_DYNAMIC(CL4BEReplyAnyWaitAnyFunction);
public:
    /** constructs an object of this class */
	CL4BEReplyAnyWaitAnyFunction();
	~CL4BEReplyAnyWaitAnyFunction();

public: // Public Methods

protected: // Protected methods
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteExceptionCheck(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteFlexpageOpcodePatch(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteLongIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteLongFlexpageIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteShortIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteShortFlexpageIPC(CBEFile *pFile, CBEContext *pContext);
};

#endif
