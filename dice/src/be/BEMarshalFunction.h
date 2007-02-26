/**
 *	\file	dice/src/be/BEMarshalFunction.h
 *	\brief	contains the declaration of the class CBEMarshalFunction
 *
 *	\date	10/09/2003
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
#ifndef CBEMARSHALFUNCTION_H
#define CBEMARSHALFUNCTION_H

#include <be/BEOperationFunction.h>

/**	\class CBEMarshalFunction
 *	\ingroup backend
 *	\brief the marshalling function class for the back-end
 *
 * This class contains a back-end function which belongs to a front-end operation
 */
class CBEMarshalFunction : public CBEOperationFunction
{
DECLARE_DYNAMIC(CBEMarshalFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEMarshalFunction();
	virtual ~CBEMarshalFunction();

protected:
	/**	\brief copy constructor */
	CBEMarshalFunction(CBEMarshalFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual void WriteReturn(CBEFile * pFile, CBEContext * pContext);
    virtual int GetFixedSize(int nDirection,  CBEContext* pContext);
    virtual int GetSize(int nDirection, CBEContext *pContext);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();
	virtual CBETypedDeclarator* FindParameterType(String sTypeName);
	virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext);
    virtual bool AddParameters(CFEOperation * pFEOperation, CBEContext * pContext);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);

protected:
    virtual int GetReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetFixedReturnSize(int nDirection, CBEContext * pContext);
    virtual int GetMaxReturnSize(int nDirection, CBEContext * pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual void WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual void WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma);
    virtual void WriteCleanup(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
};

#endif
