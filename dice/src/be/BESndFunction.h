/**
 *    \file    dice/src/be/BESndFunction.h
 *    \brief   contains the declaration of the class CBESndFunction
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
#ifndef __DICE_BESNDFUNCTION_H__
#define __DICE_BESNDFUNCTION_H__

#include "be/BEOperationFunction.h"

/**    \class CBESndFunction
 *    \ingroup backend
 *    \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBESndFunction : public CBEOperationFunction
{
// Constructor
public:
    /**    \brief constructor
     */
    CBESndFunction();
    virtual ~CBESndFunction();

protected:
    /**    \brief copy constructor */
    CBESndFunction(CBESndFunction &src);

public:
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile,  CBEContext* pContext);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile,  CBEContext* pContext);
    virtual int GetSendDirection();
    virtual int GetReceiveDirection();
    virtual int GetSize(int nDirection, CBEContext * pContext);
    virtual int GetFixedSize(int nDirection, CBEContext *pContext);
    virtual void WriteReturn(CBEFile *pFile, CBEContext *pContext);

protected:
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
};

#endif // !__DICE_BESNDFUNCTION_H__
