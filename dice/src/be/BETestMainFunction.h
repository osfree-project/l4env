/**
 *	\file	dice/src/be/BETestMainFunction.h
 *	\brief	contains the declaration of the class CBETestMainFunction
 *
 *	\date	03/11/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#ifndef __DICE_BETESTMAINFUNCTION_H__
#define __DICE_BETESTMAINFUNCTION_H__

#include "be/BEFunction.h"
#include "Vector.h"

class CBETestServerFunction;
class CBEContext;

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

/**	\class CBETestMainFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class represents the main function of the test-suite.
 */
class CBETestMainFunction : public CBEFunction  
{
DECLARE_DYNAMIC(CBETestMainFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBETestMainFunction();
	virtual ~CBETestMainFunction();

protected:
	/**	\brief copy constructor */
	CBETestMainFunction(CBETestMainFunction &src);

public:
	virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);
	virtual CBETestServerFunction* GetNextSrvLoop(VectorElement* &pIter);
	virtual VectorElement* GetFirstSrvLoop();
	virtual void RemoveSrvLoop(CBETestServerFunction *pFunction);
	virtual void AddSrvLoop(CBETestServerFunction *pFunction);
	virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
	virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext * pContext);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteReturn(CBEFile *pFile, CBEContext *pContext);

protected:
    virtual bool AddTestFunction(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddTestFunction(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool AddTestFunction(CFEFile *pFEFile, CBEContext *pContext);
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteTestServer(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext);

protected:
	/**	\var Vector m_vSrvLoops
	 *	\brief contains references to the server-loop test-functions
	 */
	Vector m_vSrvLoops;
};

#endif // !__DICE_BETESTMAINFUNCTION_H__
