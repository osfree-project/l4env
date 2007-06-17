/**
 *    \file    dice/src/be/l4/L4BETrace.h
 *    \brief   contains the declaration of the class CL4BETrace
 *
 *    \date    12/05/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2005
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
#ifndef __DICE_BE_L4_L4BETRACE_H__
#define __DICE_BE_L4_L4BETRACE_H__

#include "BETrace.h"

class CL4BETrace : public CBETrace
{
public:
    /** constructor of trace object */
    CL4BETrace();
    /** destructor */
    virtual ~CL4BETrace();
    
    virtual void AddLocalVariable(CBEFunction *pFunction);
    virtual void BeforeCall(CBEFile& pFile, CBEFunction *pFunction);
    virtual void AfterCall(CBEFile& pFile, CBEFunction *pFunction);
    virtual void BeforeLoop(CBEFile& pFile, CBEFunction *pFunction);
    virtual void BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction);
    virtual void AfterDispatch(CBEFile& pFile, CBEFunction *pFunction);
    virtual void BeforeReplyWait(CBEFile& pFile, CBEFunction *pFunction);
    virtual void AfterReplyWait(CBEFile& pFile, CBEFunction *pFunction);
    virtual void WaitCommError(CBEFile& pFile, CBEFunction *pFunction);
};

#endif /* __DICE_BE_L4_L4BETRACE_H__ */
