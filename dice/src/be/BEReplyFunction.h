/**
 *    \file    dice/src/be/BEReplyFunction.h
 *    \brief   contains the declaration of the class CBEReplyFunction
 *
 *    \date    04/08/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
protected:
    /** protected copy constructor */
    CBEReplyFunction(CBEReplyFunction & src);

public:
    /** public constructor */
    CBEReplyFunction();
    virtual ~CBEReplyFunction();

    virtual void CreateBackEnd(CFEOperation* pFEOperation);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, 
	    bool bMarshal);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();
    virtual int GetFixedSize(int nDirection);
    virtual int GetSize(int nDirection);
    virtual bool MsgBufferInitialization(CBEMsgBuffer * pMsgBuffer);

    virtual CBETypedDeclarator* GetExceptionVariable(void);

protected:
    virtual void AddBeforeParameters(void);
    virtual void WriteCleanup(CBEFile * pFile);
    virtual void WriteUnmarshalling(CBEFile * pFile);
    virtual void WriteInvocation(CBEFile * pFile);
    virtual void WriteVariableInitialization(CBEFile * pFile);
    virtual void AddParameter(CFETypedDeclarator * pFEParameter);
};

#endif
