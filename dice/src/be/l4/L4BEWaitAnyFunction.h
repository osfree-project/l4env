/**
 *    \file    dice/src/be/l4/L4BEWaitAnyFunction.h
 *    \brief   contains the declaration of the class CL4BEWaitAnyFunction
 *
 *    \date    03/07/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
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
#ifndef __DICE_L4BEWAITANYFUNCTION_H__
#define __DICE_L4BEWAITANYFUNCTION_H__

#include "be/BEWaitAnyFunction.h"

/** \class CL4BEWaitAnyFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CL4BEWaitAnyFunction : public CBEWaitAnyFunction
{
// Constructor
public:
    /** \brief constructor
     *    \param bOpenWait true if wait is for any sender
     *    \param bReply true if reply is sent before wait
     */
    CL4BEWaitAnyFunction(bool bOpenWait, bool bReply);
    virtual ~CL4BEWaitAnyFunction();

protected:
    /** \brief copy constructor
     *    \param src the source to copy from
     */
    CL4BEWaitAnyFunction(CL4BEWaitAnyFunction &src);

protected:
    virtual void WriteIPCErrorCheck(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteExceptionCheck(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteUnmarshalling(CBEFile *pFile, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFlexpageOpcodePatch(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteIPCReplyWait(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteLongIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteLongFlexpageIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteShortIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteShortFlexpageIPC(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteReleaseMemory(CBEFile *pFile, CBEContext *pContext);
};

#endif // !__DICE_L4BEWAITANYFUNCTION_H__
