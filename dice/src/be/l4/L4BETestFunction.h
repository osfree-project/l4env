/**
 *	\file	dice/src/be/l4/L4BETestFunction.h
 *	\brief	contains the declaration of the class CL4BETestFunction
 *
 *	\date	Tue May 28 2002
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

#ifndef L4BETESTFUNCTION_H
#define L4BETESTFUNCTION_H

#include <be/BETestFunction.h>

class CDeclaratorStack;

/** \class CL4BETestFunction
 *  \ingroup backend
 *  \brief implements L4 specifica of the test-function methods
 */

class CL4BETestFunction : public CBETestFunction
{
DECLARE_DYNAMIC(CL4BETestFunction);
public:
    /** creates a new object of this class */
	CL4BETestFunction();
	~CL4BETestFunction();

protected: // Protected methods
    virtual void WriteErrorMessage(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext);
    virtual void WriteErrorMessageThread(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext);
    virtual void WriteErrorMessageTask(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext);
    virtual void WriteSuccessMessageTask(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext);
    virtual void WriteSuccessMessageThread(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext);
    virtual void WriteSuccessMessage(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext);
	virtual void CompareDeclarator(CBEFile* pFile,  CBEType* pType,  CDeclaratorStack* pStack,  CBEContext* pContext);
	virtual void CompareFlexpages(CBEFile* pFile,  CBEType* pType,  CDeclaratorStack* pStack,  CBEContext* pContext);
};

#endif
