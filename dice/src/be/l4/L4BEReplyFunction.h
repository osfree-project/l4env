/**
 *  \file    dice/src/be/l4/L4BEReplyFunction.h
 *  \brief   contains the declaration of the class CL4BEReplyFunction
 *
 *  \date    02/07/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2007
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
#ifndef CL4BEREPLYFUNCTION_H
#define CL4BEREPLYFUNCTION_H


#include <be/BEReplyFunction.h>

/** \class CL4BEReplyFunction
 *  \brief implements the L4 specific parts of the reply function
 */
class CL4BEReplyFunction : public CBEReplyFunction
{
protected:
    /** copy constructor */
    CL4BEReplyFunction(CL4BEReplyFunction& src);

public:
    /** public constructor */
    CL4BEReplyFunction();
    virtual ~CL4BEReplyFunction();

public:
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);

protected:
    virtual void WriteInvocation(CBEFile *pFile);
    virtual void WriteIPC(CBEFile *pFile);
    virtual void WriteIPCErrorCheck(CBEFile *pFile);
    virtual void WriteVariableInitialization(CBEFile * pFile);
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
};

#endif
