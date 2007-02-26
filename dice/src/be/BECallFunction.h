/**
 *	\file	dice/src/be/BECallFunction.h
 *	\brief	contains the declaration of the class CBECallFunction
 *
 *	\date	01/18/2002
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
#ifndef __DICE_BECALLFUNCTION_H__
#define __DICE_BECALLFUNCTION_H__

#include "be/BEOperationFunction.h"

/**	\class CBECallFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBECallFunction : public CBEOperationFunction  
{
DECLARE_DYNAMIC(CBECallFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBECallFunction();
	virtual ~CBECallFunction();

protected:
	/**	\brief copy constructor */
	CBECallFunction(CBECallFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual int GetSize(int nDirection, CBEContext * pContext);
    virtual int GetFixedSize(int nDirection, CBEContext *pContext);

protected:
	virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
	virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteReturnVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
};

#endif // !__DICE_BECALLFUNCTION_H__
