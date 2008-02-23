/**
 *  \file    dice/src/be/l4/v4/L4V4BEReplyFunction.h
 *  \brief   contains the declaration of the class CL4V4BEReplyFunction
 *
 *  \date    02/20/2008
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2008
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
#ifndef CL4V4BEREPLYFUNCTION_H
#define CL4V4BEREPLYFUNCTION_H


#include <be/l4/L4BEReplyFunction.h>

/** \class CL4V4BEReplyFunction
 *  \brief implements the L4 specific parts of the reply function
 */
class CL4V4BEReplyFunction : public CL4BEReplyFunction
{
public:
    /** public constructor */
    CL4V4BEReplyFunction();
    virtual ~CL4V4BEReplyFunction();

public:
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);

protected:
	virtual void WriteMarshalling(CBEFile& pFile);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteIPCErrorCheck(CBEFile& pFile);
    virtual void CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide);
};

#endif
