/**
 *    \file    dice/src/be/BEOperationFunction.h
 *    \brief   contains the declaration of the class CBEOperationFunction
 *
 *    \date    01/14/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BEOPERATIONFUNCTION_H__
#define __DICE_BEOPERATIONFUNCTION_H__

#include "be/BEFunction.h"

class CBEContext;
class CFEOperation;
class CFETypedDeclarator;
class CFEIdentifier;
class CFEAttribute;

/**    \class CBEOperationFunction
 *    \ingroup backend
 *    \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBEOperationFunction : public CBEFunction
{
// Constructor
public:
    /**    \brief constructor
     */
    CBEOperationFunction();
    virtual ~CBEOperationFunction();

protected:
    /**    \brief copy constructor */
    CBEOperationFunction(CBEOperationFunction &src);

public:
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual CBEClass* GetClass();

protected:
    virtual bool AddAttributes(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool AddExceptions(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool AddParameters(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool AddParameter(CFETypedDeclarator *pFEParameter, CBEContext *pContext);
    virtual bool AddException(CFEIdentifier *pFEException, CBEContext *pContext);
    virtual bool AddAttribute(CFEAttribute *pFEAttribute, CBEContext *pContext);
    virtual int WriteMarshalOpcode(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
};

#endif // !__DICE_BEOPERATIONFUNCTION_H__
