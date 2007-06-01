/**
 *  \file    dice/src/be/l4/L4BESizes.h
 *  \brief   contains the declaration of the class CL4BESizes
 *
 *  \date    10/10/2002
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

#ifndef L4BESIZES_H
#define L4BESIZES_H

#include <be/BESizes.h>

/** \class CL4BESizes
 *  \ingroup backend
 *  \brief contains L4 specific functions to query for values and general L4 values
 */
class CL4BESizes : public CBESizes
{
public:
    /** constructs a sizes object */
    CL4BESizes();
    virtual ~CL4BESizes();

public: // Public methods
    /** abstract method, which shall determine the maximum number of bytes of
     * a message for a short IPC */
    virtual int GetMaxShortIPCSize(void) = 0;
    using CBESizes::GetSizeOfType;
    virtual int GetSizeOfType(int nFEType, int nFESize = 0);
};

#endif
