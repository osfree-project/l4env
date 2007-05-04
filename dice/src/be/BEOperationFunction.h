/**
 *  \file    dice/src/be/BEOperationFunction.h
 *  \brief   contains the declaration of the class CBEOperationFunction
 *
 *  \date    01/14/2002
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
#ifndef __DICE_BEOPERATIONFUNCTION_H__
#define __DICE_BEOPERATIONFUNCTION_H__

#include "be/BEFunction.h"

class CFEOperation;
class CFETypedDeclarator;
class CFEIdentifier;
class CFEAttribute;

/** \class CBEOperationFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a
 * front-end operation
 */
class CBEOperationFunction : public CBEFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBEOperationFunction(FUNCTION_TYPE nFunctionType);
    virtual ~CBEOperationFunction();

protected:
    /** \brief copy constructor */
    CBEOperationFunction(CBEOperationFunction &src);

public:
    virtual void CreateBackEnd(CFEOperation *pFEOperation);

protected:
    virtual void AddParameters(CFEOperation *pFEOperation);
    virtual void AddParameter(CFETypedDeclarator *pFEParameter);
    void AddExceptions(CFEOperation *pFEOperation);
    void AddException(CFEIdentifier *pFEException);
    void AddAttributes(CFEOperation *pFEOperation);
    void AddAttribute(CFEAttribute *pFEAttribute);
};

#endif // !__DICE_BEOPERATIONFUNCTION_H__
