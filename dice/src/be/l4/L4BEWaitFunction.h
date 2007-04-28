/**
 *  \file    dice/src/be/l4/L4BEWaitFunction.h
 *  \brief   contains the declaration of the class CL4BEWaitFunction
 *
 *  \date    06/01/2002
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

#ifndef L4BEWAITFUNCTION_H
#define L4BEWAITFUNCTION_H

#include <be/BEWaitFunction.h>

/** \class CL4BEWaitFunction
 *  \brief implements the L4 specific wait function
 *
 * Wait for a specific message from any sender.
 */
class CL4BEWaitFunction : public CBEWaitFunction
{

public:
    /** creates a new object of this class */
    CL4BEWaitFunction(bool bOpenWait);
    virtual ~CL4BEWaitFunction();

public: // Public methods
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);

protected: // Protected methods
    virtual bool HasVariableSizedParameters(DIRECTION_TYPE nDirection);
    virtual void WriteIPCErrorCheck(CBEFile * pFile);
    virtual void WriteInvocation(CBEFile * pFile);
    virtual void WriteVariableInitialization(CBEFile* pFile);
    virtual void WriteFlexpageOpcodePatch(CBEFile *pFile);
    virtual void WriteIPC(CBEFile *pFile);
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
};

#endif
