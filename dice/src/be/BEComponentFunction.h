/**
 *	\file	dice/src/be/BEComponentFunction.h
 *	\brief	contains the declaration of the class CBEComponentFunction
 *
 *	\date	01/22/2002
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
#ifndef __DICE_BECOMPONENTFUNCTION_H__
#define __DICE_BECOMPONENTFUNCTION_H__

#include "be/BEOperationFunction.h"

class CFEOperation;

/**	\class CBEComponentFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class resembles the component function skeleton.
 */
class CBEComponentFunction : public CBEOperationFunction  
{
DECLARE_DYNAMIC(CBEComponentFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEComponentFunction();
	virtual ~CBEComponentFunction();

protected:
	/**	\brief copy constructor */
	CBEComponentFunction(CBEComponentFunction &src);

public:
	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
	virtual bool AddToFile(CBEImplementationFile * pImpl, CBEContext * pContext);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);

protected:
    virtual void WriteGlobalVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFunctionDefinition(CBEFile *pFile, CBEContext *pContext);
   	virtual void WriteReturn(CBEFile *pFile, CBEContext *pContext);
   	virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
   	virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
	virtual void SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext);
   	virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
   	virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
   	virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);

protected:
	/**	\var CBEFunction *m_pFunction
	 *	\brief a reference to the function which is tested (if we test at all)
	 */
	CBEFunction *m_pFunction;
};

#endif // !__DICE_BECOMPONENTFUNCTION_H__
