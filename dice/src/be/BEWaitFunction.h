/**
 *	\file	dice/src/be/BEWaitFunction.h
 *	\brief	contains the declaration of the class CBEWaitFunction
 *
 *	\date	01/14/2002
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
#ifndef __DICE_BEWAITFUNCTION_H__
#define __DICE_BEWAITFUNCTION_H__

#include "be/BEOperationFunction.h"

/**	\class CBEWaitFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBEWaitFunction : public CBEOperationFunction  
{
DECLARE_DYNAMIC(CBEWaitFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEWaitFunction();
	virtual ~CBEWaitFunction();

protected:
	/**	\brief copy constructor */
	CBEWaitFunction(CBEWaitFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();

protected:
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteOpcodeCheck(CBEFile *pFile, CBEContext *pContext);
	virtual bool HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall);
	virtual bool AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext);
};

#endif // !__DICE_BEWAITFUNCTION_H__
