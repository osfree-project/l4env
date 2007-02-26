/**
 *  \file     dice/src/be/l4/v4/L4V4BEWaitAnyFunction.h
 *  \brief    contains the declaration of the class CL4V4BEWaitAnyFunction
 *
 *  \date     Mon Jul 5 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#ifndef L4V4BEWAITANYFUNCTION_H
#define L4V4BEWAITANYFUNCTION_H

#include <be/l4/L4BEWaitAnyFunction.h>

/** \class CL4V4BEWaitAnyFunction
 *    \ingroup backend
 *    \brief V4 specific implementation of wait-any function
 */
class CL4V4BEWaitAnyFunction : public CL4BEWaitAnyFunction
{
public:
    /** \brief creates a new instance of this class
     *    \param bOpenWait true if wait is for any sender
     *    \param bReply true if reply is send before wait
     */
    CL4V4BEWaitAnyFunction(bool bOpenWait, bool bReply);
    virtual ~CL4V4BEWaitAnyFunction();

protected:
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPCReplyWait(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
    virtual void WriteIPCErrorCheck(CBEFile *pFile, CBEContext *pContext);
};

#endif
