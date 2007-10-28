/**
 *  \file    dice/src/be/BEMarshalExceptionFunction.h
 *  \brief   contains the declaration of the class CBEMarshalExceptionFunction
 *
 *  \date    03/20/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef CBEMARSHALEXCEPTIONFUNCTION_H
#define CBEMARSHALEXCEPTIONFUNCTION_H

#include <be/BEOperationFunction.h>

/** \class CBEMarshalExceptionFunction
 *  \ingroup backend
 *  \brief the marshalling function class for exceptions
 */
class CBEMarshalExceptionFunction : public CBEOperationFunction
{
	// Constructor
public:
	/** \brief constructor
	 */
	CBEMarshalExceptionFunction();
	virtual ~CBEMarshalExceptionFunction();

public:
	virtual void CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide);
	virtual bool DoWriteFunction(CBEFile* pFile);
	virtual void MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer);
	virtual CMsgStructType GetSendDirection();
	virtual CMsgStructType GetReceiveDirection();
	virtual CBETypedDeclarator* GetExceptionVariable();
	virtual CBETypedDeclarator* FindParameterType(std::string sTypeName);
	virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter,
		bool bMarshal);

protected:
	virtual void AddParameters(CFEOperation *pFEOperation);
	virtual void AddParameter(CFEIdentifier * pFEIdentifier);
	virtual void AddAfterParameters();
	virtual void WriteVariableInitialization(CBEFile& pFile);
	virtual void WriteInvocation(CBEFile& pFile);
	virtual void WriteCallParameter(CBEFile& pFile,
		CBETypedDeclarator *pParameter, bool bCallFromSameClass);
	virtual void WriteFunctionDefinition(CBEFile& pFile);
	virtual void WriteAccessSpecifier(CBEHeaderFile& pFile);
};

#endif
