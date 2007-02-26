/**
 *    \file    dice/src/be/l4/v4/L4V4BECallFunction.h
 *    \brief    contains the declaration of the class CL4V4BECallFunction
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2004
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
#ifndef L4V4BECALLFUNCTION_H
#define L4V4BECALLFUNCTION_H

#include <be/l4/L4BECallFunction.h>

/** \class CL4V4BECallFunction
 *  \ingroup backend
 *  \brief encapsulates V4 specifics of call function
 */
class CL4V4BECallFunction : public CL4BECallFunction
{

public:
    /** creates an instance of this class */
    CL4V4BECallFunction();
    virtual ~CL4V4BECallFunction();

public:
    virtual int GetFixedSize(int nDirection, CBEContext *pContext);
    virtual int GetSize(int nDirection, CBEContext *pContext);

protected:
    virtual void WriteVariableDeclaration(CBEFile* pFile,  CBEContext* pContext);
    virtual void WriteMsgTagDeclaration(CBEFile *pFile, CBEContext* pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext);
    virtual int WriteUnmarshalException(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
};

#endif
