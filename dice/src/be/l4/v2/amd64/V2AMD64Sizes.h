/**
 *    \file    dice/src/be/l4/v2/amd64/V2AMD64Sizes.h
 *    \brief   contains the declaration of the class CL4V2AMD64BESizes
 *
 *    \date    12/08/2005
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

#ifndef __DICE__V2AMD64SIZES_H__
#define __DICE__V2AMD64SIZES_H__

#include <be/l4/v2/L4V2BESizes.h>

/** \class CL4V2AMD64BESizes
 *  \ingroup backend
 *  \brief contains the L4 version 2 specific sizes for AMD64
 */
class CL4V2AMD64BESizes : public CL4V2BESizes
{
public:
    /** constructs a new object of this class */
    CL4V2AMD64BESizes();
    virtual ~CL4V2AMD64BESizes();

public: // Public methods
    virtual int GetSizeOfType(int nFEType, int nFESize = 0);
};

#endif
