/**
 *	\file	dice/src/be/l4/L4BECallFunction.h
 *	\brief	contains the declaration of the class CL4BECallFunction
 *
 *	\date	02/07/2002
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
#ifndef __DICE_L4BECALLFUNCTION_H__
#define __DICE_L4BECALLFUNCTION_H__

#include "be/BECallFunction.h"

/**	\class CL4BECallFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CL4BECallFunction:public CBECallFunction
{
DECLARE_DYNAMIC(CL4BECallFunction);
// Constructor
public:
      /**	\brief constructor
       */
      CL4BECallFunction();
      virtual ~ CL4BECallFunction();
      
protected:
    /**	\brief copy constructor */
    CL4BECallFunction(CL4BECallFunction & src);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual void WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual int WriteFlexpageReturnPatch(CBEFile *pFile, int nStartOffset, bool & bUseConstOffset, CBEContext *pContext);
    virtual bool DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext);
    virtual void WriteIPC(CBEFile* pFile, CBEContext* pContext);
};

#endif				// !__DICE_L4BECALLFUNCTION_H__
