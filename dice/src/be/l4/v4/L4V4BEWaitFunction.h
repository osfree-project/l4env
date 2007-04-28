/**
 *    \file    dice/src/be/l4/v4/L4V4BEWaitFunction.h
 *    \brief   contains the declaration of the class CL4V4BEWaitFunction
 *
 *    \date    06/11/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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
#ifndef L4V4BEWAITFUNCTION_H
#define L4V4BEWAITFUNCTION_H

#include <be/l4/L4BEWaitFunction.h>

/** \class CL4V4BEWaitFunction
 *  \ingroup backend
 *  \brief encapsulates V4 specifics of wait function
 */
class CL4V4BEWaitFunction : public CL4BEWaitFunction
{

public:
    /** creates an instance of this class */
    CL4V4BEWaitFunction(bool bOpenWait);
    virtual ~CL4V4BEWaitFunction();

public:
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetSize(DIRECTION_TYPE nDirection);
    virtual void CreateBackEnd(CFEOperation *pFEOperation);

protected:
    virtual void WriteUnmarshalling(CBEFile * pFile);
    virtual void WriteIPCErrorCheck(CBEFile * pFile);
    virtual void WriteOpcodeCheck(CBEFile *pFile);
};

#endif
