/**
 *    \file    dice/src/be/l4/x0/L4X0BEWaitAnyFunction.h
 *    \brief   contains the declaration of the class CL4X0BEWaitAnyFunction
 *
 *    \date    06/01/2002
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

#ifndef L4X0BEWAITANYFUNCTION_H
#define L4X0BEWAITANYFUNCTION_H


#include <be/l4/L4BEWaitAnyFunction.h>

/** \class CL4X0BEWaitAnyFunction
 *  \ingroup backend
 *  \brief implements X0 specific ipc code
 */
class CL4X0BEWaitAnyFunction : public CL4BEWaitAnyFunction
{

public:
    /** \brief creates a wait function object
     *    \param bOpenWait true if wait for any sender
     *    \param bReply true if reply sent before wait
     */
    CL4X0BEWaitAnyFunction(bool bOpenWait, bool bReply);
    virtual ~CL4X0BEWaitAnyFunction();

protected:
    virtual void WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
};

#endif
