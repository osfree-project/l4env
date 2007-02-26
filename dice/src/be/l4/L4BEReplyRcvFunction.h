/**
 *	\file	dice/src/be/l4/L4BEReplyRcvFunction.h
 *	\brief	contains the declaration of the class CL4BEReplyRcvFunction
 *
 *	\date	03/07/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#ifndef __DICE_L4BEREPLYRCVFUNCTION_H__
#define __DICE_L4BEREPLYRCVFUNCTION_H__

#include "be/BEReplyRcvFunction.h"

/**	\class CL4BEReplyRcvFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class implements a function, which replies to
 * a specific client with a specific message and waits
 * for a any message from this client.
 */
class CL4BEReplyRcvFunction : public CBEReplyRcvFunction  
{
DECLARE_DYNAMIC(CL4BEReplyRcvFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CL4BEReplyRcvFunction();
	virtual ~CL4BEReplyRcvFunction();

protected:
	/**	\brief copy constructor */
	CL4BEReplyRcvFunction(CL4BEReplyRcvFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);

protected:
	virtual void WriteIPCReceiveOnly(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteIPCErrorCheck(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
	virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFlexpageOpcodePatch(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual bool HasVariableSizedParameters();
    virtual int GetReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetFixedReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetMaxReturnSize(int nDirection, CBEContext * pContext);
    virtual bool DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext);
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);

protected:
};

#endif // !__DICE_L4BEREPLYRCVFUNCTION_H__
