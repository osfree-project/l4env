/**
 *	\file	dice/src/be/BEUnmarshalFunction.h
 *	\brief	contains the declaration of the class CBEUnmarshalFunction
 *
 *	\date	01/20/2002
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
#ifndef __DICE_BEUNMARSHALFUNCTION_H__
#define __DICE_BEUNMARSHALFUNCTION_H__

#include "be/BEOperationFunction.h"

class CFEInterface;

/**	\class CBEUnmarshalFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains a back-end function which belongs to a front-end operation
 */
class CBEUnmarshalFunction : public CBEOperationFunction  
{
DECLARE_DYNAMIC(CBEUnmarshalFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEUnmarshalFunction();
	virtual ~CBEUnmarshalFunction();

protected:
	/**	\brief copy constructor */
	CBEUnmarshalFunction(CBEUnmarshalFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext);
    virtual bool HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall = false);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual CBETypedDeclarator * FindParameterType(String sTypeName);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();

protected:
	virtual bool AddParameter(CFETypedDeclarator *pFEParameter, CBEContext *pContext);
	virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
	virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual void WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif // !__DICE_BEUNMARSHALFUNCTION_H__
