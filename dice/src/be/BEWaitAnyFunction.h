/**
 *  \file    dice/src/be/BEWaitAnyFunction.h
 *  \brief   contains the declaration of the class CBEWaitAnyFunction
 *
 *  \date    01/21/2002
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
#ifndef __DICE_BEWAITANYFUNCTION_H__
#define __DICE_BEWAITANYFUNCTION_H__

#include "be/BEInterfaceFunction.h"

/** \class CBEWaitAnyFunction
 *  \ingroup backend
 *  \brief the wait-any function class for the back-end
 *
 * This class contains the code to write a wait-any function
 */
class CBEWaitAnyFunction : public CBEInterfaceFunction
{
// Constructor
public:
    /** \brief constructor
     *  \param bOpenWait true if this waiting for any sender
     *  \param bReply true if we send a reply before waiting
     */
    CBEWaitAnyFunction(bool bOpenWait, bool bReply);
    virtual ~CBEWaitAnyFunction();

public:
    virtual void CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter,
	    bool bMarshal);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual CBETypedDeclarator * FindParameterType(std::string sTypeName);
    virtual DIRECTION_TYPE GetReceiveDirection();
    virtual DIRECTION_TYPE GetSendDirection();

protected:
    virtual void WriteUnmarshalling(CBEFile& pFile);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteParameter(CBEFile& pFile,
	CBETypedDeclarator * pParameter, bool bUseConst = true);
    virtual void WriteFunctionDefinition(CBEFile& pFile);
    virtual void WriteAccessSpecifier(CBEHeaderFile& pFile);

protected:
    /** \var bool m_bOpenWait
     *  \brief true if this is an open wait function; false if closed wait
     */
    bool m_bOpenWait;
    /** \var bool m_bReply
     *  \brief true if this is includes a reply before waiting
     */
    bool m_bReply;
};

#endif // !__DICE_BEWAITANYFUNCTION_H__
