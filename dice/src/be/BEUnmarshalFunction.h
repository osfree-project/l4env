/**
 *  \file    dice/src/be/BEUnmarshalFunction.h
 *  \brief   contains the declaration of the class CBEUnmarshalFunction
 *
 *  \date    01/20/2002
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
#ifndef __DICE_BEUNMARSHALFUNCTION_H__
#define __DICE_BEUNMARSHALFUNCTION_H__

#include "be/BEOperationFunction.h"

class CFEInterface;

/** \class CBEUnmarshalFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains a back-end function which belongs to a front-end
 * operation
 */
class CBEUnmarshalFunction : public CBEOperationFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBEUnmarshalFunction();
    virtual ~CBEUnmarshalFunction();

protected:
    /** \brief copy constructor */
    CBEUnmarshalFunction(CBEUnmarshalFunction &src);

public:
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter,
	    bool bMarshal);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual CBETypedDeclarator * FindParameterType(std::string sTypeName);
    virtual DIRECTION_TYPE GetReceiveDirection();
    virtual DIRECTION_TYPE GetSendDirection();
    virtual void MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer);

    virtual CBETypedDeclarator* GetExceptionVariable();

protected:
    virtual void AddParameter(CFETypedDeclarator *pFEParameter);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteCallParameter(CBEFile& pFile,
	CBETypedDeclarator *pParameter, bool bCallFromSameClass);
    virtual void WriteFunctionDefinition(CBEFile& pFile);
    virtual void AddAfterParameters();
};

#endif // !__DICE_BEUNMARSHALFUNCTION_H__
