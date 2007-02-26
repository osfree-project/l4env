/**
 *	\file	dice/src/be/BETestServerFunction.h
 *	\brief	contains the declaration of the class CBETestServerFunction
 *
 *	\date	04/04/2002
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
#ifndef __DICE_BETESTSERVERFUNCTION_H__
#define __DICE_BETESTSERVERFUNCTION_H__

#include "be/BEInterfaceFunction.h"

class CFEInterface;
class CFEOperation;
class CBEContext;
class CBETestFunction;
class CBEImplementationFile;

/**	\class CBETestServerFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBETestServerFunction : public CBEInterfaceFunction
{
DECLARE_DYNAMIC(CBETestServerFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBETestServerFunction();
	virtual ~CBETestServerFunction();

protected:
	/**	\brief copy constructor */
	CBETestServerFunction(CBETestServerFunction &src);

public:
	virtual void WriteBody(CBEFile *pFile, CBEContext *pContext);
	virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
	virtual void AddFunction(CBETestFunction *pFunction);
	virtual CBETestFunction* GetNextFunction(VectorElement* &pIter);
	virtual VectorElement* GetFirstFunction();
	virtual void RemoveFunction(CBETestFunction *pFunction);
	virtual bool DoWriteFunction(CBEFile * pFile, CBEContext * pContext);
	virtual void Write(CBEImplementationFile * pFile, CBEContext * pContext);
	virtual bool IsTargetFile(CBEImplementationFile * pFile);

protected:
    virtual bool AddTestFunction(CFEInterface *pFEInterface, CBEContext *pContext);
	virtual bool AddTestFunction(CFEOperation *pFEOperation, CBEContext *pContext);
	virtual void WriteStopServerLoop(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteTestFunction(CBEFile *pFile, CBETestFunction *pFunction, CBEContext *pContext);
	virtual void WriteTestFunctions(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteStartServerLoop(CBEFile *pFile, CBEContext *pContext);
	virtual void WriteCallParameterList(CBEFile * pFile, CBEContext * pContext);
	virtual void SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext);
	virtual void WriteParameterList(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext);

protected:
	/**	\var Vector m_vFunctions
	 *	\brief contains references to the test-functions
	 */
	Vector m_vFunctions;
	/**	\var CBEFunction *m_pFunction
	 *	\brief a reference to the function we test
	 */
	CBEFunction *m_pFunction;
};

#endif // !__DICE_BETESTSERVERFUNCTION_H__
