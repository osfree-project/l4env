/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aBECallFunction.h
 *    \brief   contains the declaration of the class CL4X0aBECallFunction
 *
 *    \date    01/07/2004
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#ifndef L4X0ABECALLFUNCTION_H
#define L4X0ABECALLFUNCTION_H


#include <be/l4/L4BECallFunction.h>

/** \class CL4X0aBECallFunction
 *  \ingroup backend
 *  \brief implements L4 X0 specific elements of the call function using the X0 adpation code
 *
 *  X0 adaption is the use of the L4 X0 microkernel with 64bit thread-IDs in user-land
 **/
class CL4X0aBECallFunction : public CL4BECallFunction
{

public:
    /** creates a call function object */
    CL4X0aBECallFunction();
    virtual ~CL4X0aBECallFunction();

protected:
    virtual void WriteIPC(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext);
};

#endif
