/**
 *  \file    dice/src/be/BEMarshalFunction.h
 *  \brief   contains the declaration of the class CBEMarshalFunction
 *
 *  \date    10/09/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

/** \class CBEMarshalFunction
 *  \ingroup backend
 *  \brief the marshalling function class for the back-end
 *
 * This class contains a back-end function which belongs to a front-end
 * operation
 */
class CBEMarshalFunction : public CBEOperationFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBEMarshalFunction();
    virtual ~CBEMarshalFunction();

protected:
    /** \brief copy constructor */
    CBEMarshalFunction(CBEMarshalFunction &src);

public:
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual void MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer);
    virtual void WriteReturn(CBEFile& pFile);
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);
    virtual DIRECTION_TYPE GetReceiveDirection();
    virtual DIRECTION_TYPE GetSendDirection();
    virtual CBETypedDeclarator* FindParameterType(std::string sTypeName);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter,
	    bool bMarshal);
    virtual void AddParameter(CFETypedDeclarator * pFEParameter);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);

    virtual CBETypedDeclarator* GetExceptionVariable(void);

protected:
    virtual int GetReturnSize(DIRECTION_TYPE nDirection);
    virtual int GetFixedReturnSize(DIRECTION_TYPE nDirection);
    virtual int GetMaxReturnSize(DIRECTION_TYPE nDirection);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteCallParameter(CBEFile& pFile,
	CBETypedDeclarator *pParameter, bool bCallFromSameClass);
    virtual void WriteFunctionDefinition(CBEFile& pFile);
    virtual void AddAfterParameters();
    virtual void AddBeforeParameters();
};

#endif
