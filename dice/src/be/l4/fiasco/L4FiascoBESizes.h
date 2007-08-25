/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBESizes.h
 *  \brief   contains the declaration of the class CL4FiascoBESizes
 *
 *  \date    08/20/2007
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

#ifndef L4FIASCOBESIZES_H
#define L4FIASCOBESIZES_H

#include <be/l4/L4BESizes.h>
#include <string>

/** \class CL4FiascoBESizes
 *  \ingroup backend
 *  \brief contains the L4 version 2 specific sizes
 */
class CL4FiascoBESizes : public CL4BESizes
{
public:
    /** constructs a new object of this class */
    CL4FiascoBESizes();
    virtual ~CL4FiascoBESizes();

public: // Public methods
    virtual int GetMaxShortIPCSize(void);
    virtual int GetMaxSizeOfType(int nFEType);
    using CL4BESizes::GetSizeOfType;
    virtual int GetSizeOfType(std::string sUserType);
};

#endif
