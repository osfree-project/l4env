/**
 *    \file    dice/src/be/l4/v2/L4V2BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4V2BEReplyFunction
 *
 *    \date    11/23/2004
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

#ifndef L4V2BEREPLYFUNCTION_H
#define L4V2BEREPLYFUNCTION_H

#include <be/l4/L4BEReplyFunction.h>

/** \class CL4V2BEReplyFunction
 *  \ingroup backend
 *  \brief declares variables specific for V2
 */
class CL4V2BEReplyFunction : public CL4BEReplyFunction
{
public:
    /** creates the reply function object */
    CL4V2BEReplyFunction();
    virtual ~CL4V2BEReplyFunction();

protected:
    virtual void WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteMarshalling(CBEFile * pFile,  int nStartOffset,  bool & bUseConstOffset,  CBEContext * pContext);
    virtual void WriteInvocation(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteIPC(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
};

#endif
