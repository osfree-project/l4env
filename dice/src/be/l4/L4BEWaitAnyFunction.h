/**
 *    \file    dice/src/be/l4/L4BEWaitAnyFunction.h
 *  \brief   contains the declaration of the class CL4BEWaitAnyFunction
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
     *  \param bOpenWait true if wait is for any sender
     *  \param bReply true if reply is sent before wait
     */
    CL4BEWaitAnyFunction(bool bOpenWait, bool bReply);
    ~CL4BEWaitAnyFunction();

protected:
    virtual void WriteIPCErrorCheck(CBEFile& pFile);
    virtual void WriteExceptionCheck(CBEFile& pFile);
    virtual void WriteUnmarshalling(CBEFile& pFile);
    virtual void WriteInvocation(CBEFile& pFile);
    virtual void WriteFlexpageOpcodePatch(CBEFile& pFile);
    virtual void WriteVariableInitialization(CBEFile& pFile);
    virtual void WriteIPC(CBEFile& pFile);
    virtual void WriteIPCReplyWait(CBEFile& pFile);
    virtual void WriteLongIPC(CBEFile& pFile);
    virtual void WriteLongFlexpageIPC(CBEFile& pFile);
    virtual void WriteShortIPC(CBEFile& pFile);
    virtual void WriteShortFlexpageIPC(CBEFile& pFile);
    virtual void WriteReleaseMemory(CBEFile& pFile);
    virtual void WriteDedicatedWait(CBEFile& pFile);
    virtual void CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide);
    virtual void CreateEnvironment();
    virtual void WriteCleanup(CBEFile& pFile);
};

#endif // !__DICE_L4BEWAITANYFUNCTION_H__
