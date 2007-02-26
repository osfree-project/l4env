/**
 *  \file    dice/src/be/l4/x0/L4X0BESizes.h
 *  \brief   contains the declaration of the class CL4X0BESizes
 *
 *  \date    01/08/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef L4X0BESIZES_H
#define L4X0BESIZES_H


#include "be/l4/L4BESizes.h"

/** \class CL4X0BESizes
 *  \brief implementes L4 X0 specific sizes
 *
 * Ronald Aigner
 **/
class CL4X0BESizes : public CL4BESizes
{

public:
    /** creates a size object */
    CL4X0BESizes();
    virtual ~CL4X0BESizes();

public: // Public methods
    virtual int GetMaxShortIPCSize(void);
    virtual int GetSizeOfType(int nFEType, int nFESize = 0);
    virtual int GetMaxSizeOfType(int nFEType);
};

#endif
