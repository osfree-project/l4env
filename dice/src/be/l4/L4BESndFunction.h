/**
 *	\file	dice/src/be/l4/L4BESndFunction.h
 *	\brief	contains the declaration of the class CL4BESndFunction
 *
 *	\date	Sat Jun 1 2002
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

#ifndef L4BESNDFUNCTION_H
#define L4BESNDFUNCTION_H

#include <be/BESndFunction.h>

/**\class CL4BESndFunction
 * \brief implements the L4 version of a oneway send function
  */

class CL4BESndFunction : public CBESndFunction
{
DECLARE_DYNAMIC(CL4BESndFunction);
public:
    /** creates a new object of this class */
	CL4BESndFunction();
	~CL4BESndFunction();

public: // public methods
    virtual bool CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext);

protected: // Protected methods
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual bool DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext);
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);
};

#endif
