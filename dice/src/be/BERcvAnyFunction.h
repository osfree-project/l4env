/**
 *	\file	dice/src/be/BERcvAnyFunction.h
 *	\brief	contains the declaration of the class CBERcvAnyFunction
 *
 *	\date	01/21/2002
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
#ifndef __DICE_BERCVANYFUNCTION_H__
#define __DICE_BERCVANYFUNCTION_H__

#include "be/BEInterfaceFunction.h"

class CFEInterface;
class CBEContext;

/**	\class CBERcvAnyFunction
 *	\ingroup backend
 *	\brief the receive-any function class for the back-end
 *
 * This class contains the code to write a receive-any function. A receive
 * any function can receive any message from a specific component.
 */
class CBERcvAnyFunction : public CBEInterfaceFunction  
{
DECLARE_DYNAMIC(CBERcvAnyFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBERcvAnyFunction();
	virtual ~CBERcvAnyFunction();

protected:
	/**	\brief copy constructor */
	CBERcvAnyFunction(CBERcvAnyFunction &src);

public:
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual CBETypedDeclarator * FindParameterType(String sTypeName);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();

protected:
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual void WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
	virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif // !__DICE_BERCVANYFUNCTION_H__
