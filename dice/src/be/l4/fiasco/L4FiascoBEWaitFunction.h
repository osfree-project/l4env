/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEWaitFunction.h
 *  \brief   contains the declaration of the class CL4FiascoBEWaitFunction
 *
 *  \date    11/06/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#ifndef L4FIASCOBEWAITFUNCTION_H
#define L4FIASCOBEWAITFUNCTION_H

#include <be/l4/L4BEWaitFunction.h>

/** \class CL4FiascoBEWaitFunction
 *  \brief implements the L4 specific wait function
 *
 * Wait for a specific message from any sender.
 */
class CL4FiascoBEWaitFunction : public CL4BEWaitFunction
{

public:
    /** creates a new object of this class */
    CL4FiascoBEWaitFunction(bool bOpenWait);
    ~CL4FiascoBEWaitFunction();

    virtual void CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide);
};

#endif
