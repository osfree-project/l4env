/**
 *	\file	dice/src/be/BESrvLoopFunction.h
 *	\brief	contains the declaration of the class CBESrvLoopFunction
 *
 *	\date	01/21/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#include "Vector.h"

class CFEInterface;

class CBEWaitAnyFunction;
class CBEReplyAnyWaitAnyFunction;
class CBESwitchCase;

/**	\class CBESrvLoopFunction
 *	\ingroup backend
 *	\brief the wait-any function class for the back-end
 *
 * This class contains the code to write a wait-any function
 */
class CBESrvLoopFunction : public CBEInterfaceFunction  
{
DECLARE_DYNAMIC(CBESrvLoopFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBESrvLoopFunction();
	virtual ~CBESrvLoopFunction();

protected:
	/**	\brief copy constructor */
	CBESrvLoopFunction(CBESrvLoopFunction &src);

public:
	virtual CBESwitchCase* GetNextSwitchCase(VectorElement *&pIter);
	virtual VectorElement* GetFirstSwitchCase();
	virtual void RemoveSwitchCase(CBESwitchCase *pFunction);
	virtual void AddSwitchCase(CBESwitchCase *pFunction);
	virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
	virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual int GetReceiveDirection();
    virtual int GetSendDirection();
    virtual bool DoUseParameterAsEnv(CBEContext *pContext);

protected:
    virtual void WriteSwitch(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteLoop(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteAfterParameters(CBEFile *pFile, CBEContext *pContext, bool bComma);
    virtual bool WriteBeforeParameters(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteBody(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual bool AddSwitchCases(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual void WriteDefaultCase(CBEFile *pFile, CBEContext *pContext);
    virtual void SetCallVariable(CBEFunction *pFunction, CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual void SetCallVariable(CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual void WriteEnvironmentInitialization(CBEFile *pFile, CBEContext *pContext);

protected:
    /**	\var CBEWaitAnyFunction *m_pWaitAnyFunction
     *	\brief needed to write wait any function calls
     */
    CBEWaitAnyFunction *m_pWaitAnyFunction;
    /** \var CBEReplyAnyWaitAnyFunction *m_pReplyAnyWaitAnyFunction
     *  \brief needed if default function is used
     */
    CBEReplyAnyWaitAnyFunction *m_pReplyAnyWaitAnyFunction;
    /**	\var Vector m_vSwitchCases
     *	\brief contains references to the interface's functions
     */
    Vector m_vSwitchCases;
    /** \var String m_sDefaultFunction
     *  \brief contains the name of the default function
     */
    String m_sDefaultFunction;
};

#endif // !__DICE_BESRVLOOPFUNCTION_H__
