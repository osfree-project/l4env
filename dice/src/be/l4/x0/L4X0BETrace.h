/**
 *    \file    dice/src/be/l4/x0/L4X0BETrace.h
 *    \brief   contains the declaration of the class CL4X0BETrace
 *
 *    \date    12/06/2005
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
#ifndef __DICE_BE_L4_X0_L4X0BETRACE_H__
#define __DICE_BE_L4_X0_L4X0BETRACE_H__

#include "be/l4/L4BETrace.h"

class CL4X0BETrace : public CL4BETrace
{
public:
    /** constructor of trace object */
    CL4X0BETrace();
    /** destructor */
    virtual ~CL4X0BETrace();
    
    virtual void BeforeCall(CBEFile *pFile, CBEFunction *pFunction);
    virtual void AfterCall(CBEFile *pFile, CBEFunction *pFunction);
};

#endif /* __DICE_BE_L4_X0_L4X0BETRACE_H__ */
