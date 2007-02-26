/**
 *	\file	dice/src/be/BEReplyFunction.cpp
 *	\brief	contains the implementation of the class CBEReplyFunction
 *
 *	\date	04/08/2002
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

#ifndef BEREPLYFUNCTION_H
#define BEREPLYFUNCTION_H


#include <be/BEOperationFunction.h>

/**
 *  \class CBEReplyFunction
 *  \brief implements a reply-only function from the server to the client
 */
class CBEReplyFunction : public CBEOperationFunction
{
DECLARE_DYNAMIC(CBEReplyFunction);
protected:
    /** protected copy constructor */
    CBEReplyFunction(CBEReplyFunction & src);

public:
    /** public constructor */
    CBEReplyFunction();
    virtual ~CBEReplyFunction();

    virtual bool CreateBackEnd(CFEOperation* pFEOperation, CBEContext* pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
	virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
	virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
	virtual int GetReceiveDirection();
	virtual int GetSendDirection();
	virtual int GetFixedSize(int nDirection,  CBEContext* pContext);
	virtual int GetSize(int nDirection,  CBEContext* pContext);

protected:
	virtual bool AddParameters(CFEOperation * pFEOperation, CBEContext * pContext);
	virtual void WriteCleanup(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
	virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
};

#endif
