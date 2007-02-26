/**
 *	\file	dice/src/be/l4/L4BEReplyWaitFunction.h
 *	\brief	contains the declaration of the class CL4BEReplyWaitFunction
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
#ifndef __DICE_L4BEREPLYWAITFUNCTION_H__
#define __DICE_L4BEREPLYWAITFUNCTION_H__

#include "be/BEReplyWaitFunction.h"

/**	\class CL4BEReplyWaitFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class implements the function to reply to a specific
 * client call and wait for any message.
 */
class CL4BEReplyWaitFunction : public CBEReplyWaitFunction  
{
DECLARE_DYNAMIC(CL4BEReplyWaitFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CL4BEReplyWaitFunction();
	virtual ~CL4BEReplyWaitFunction();

protected:
	/**	\brief copy constructor */
	CL4BEReplyWaitFunction(CL4BEReplyWaitFunction &src);

public:


protected:
	virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
	virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteIPCWaitOnly(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteIPCErrorCheck(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFlexpageOpcodePatch(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual bool HasVariableSizedParameters();
    virtual int GetReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetFixedReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetMaxReturnSize(int nDirection, CBEContext * pContext);
    virtual bool DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext);
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);
};

#endif // !__DICE_L4BEREPLYWAITFUNCTION_H__
