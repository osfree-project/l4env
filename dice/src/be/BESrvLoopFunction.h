/**
 *    \file    dice/src/be/BESrvLoopFunction.h
 *    \brief   contains the declaration of the class CBESrvLoopFunction
 *
 *    \date    01/21/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_BESRVLOOPFUNCTION_H__
#define __DICE_BESRVLOOPFUNCTION_H__

#include "be/BEInterfaceFunction.h"

class CFEInterface;

class CBEWaitAnyFunction;
class CBEDispatchFunction;

/**    \class CBESrvLoopFunction
 *    \ingroup backend
 *  \brief the wait-any function class for the back-end
 *
 * This class contains the code to write a wait-any function
 */
class CBESrvLoopFunction : public CBEInterfaceFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBESrvLoopFunction();
    virtual ~CBESrvLoopFunction();

public:
    virtual void CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual DIRECTION_TYPE GetReceiveDirection();
    virtual DIRECTION_TYPE GetSendDirection();
    virtual void MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer);

protected:
    virtual void WriteLoop(CBEFile& pFile);
    virtual void WriteVariableDeclaration(CBEFile& pFile);
    virtual void WriteDispatchInvocation(CBEFile& pFile);
    virtual bool DoWriteFunctionInline(CBEFile& pFile);
    virtual void WriteBody(CBEFile& pFile);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteEnvironmentInitialization(CBEFile& pFile);
    virtual void WriteDefaultEnvAssignment(CBEFile& pFile);
    virtual void WriteObjectInitialization(CBEFile& pFile);
    virtual void WriteFunctionAttributes(CBEFile& pFile);
    virtual void WriteReturn(CBEFile& pFile);
    virtual void AddParameters();
    virtual bool AddOpcodeVariable();
    virtual bool AddReplyVariable();
    virtual void CreateObject();

    CBEFunction* FindGlobalFunction(CFEInterface *pFEInterface,
	FUNCTION_TYPE nFunctionType);
    void SetCallVariables(CBEFunction *pFunction);

protected:
    /** \var CBEWaitAnyFunction *m_pWaitAnyFunction
     *  \brief needed to write wait any function calls
     */
    CBEWaitAnyFunction *m_pWaitAnyFunction;
    /** \var CBEReplyAnyWaitAnyFunction *m_pReplyAnyWaitAnyFunction
     *  \brief needed if default function is used
     */
    CBEWaitAnyFunction *m_pReplyAnyWaitAnyFunction;
    /** \var CBEDispatchFunction *m_pDispatchFunction
     *  \brief dioes the multiplexing of the request
     */
    CBEDispatchFunction *m_pDispatchFunction;
};

#endif // !__DICE_BESRVLOOPFUNCTION_H__
