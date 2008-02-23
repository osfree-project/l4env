/**
 *    \file    dice/src/be/l4/fiasco/amd64/L4FiascoAMD64ClassFactory.cpp
 *    \brief   contains the implementation of the class CL4FiascoAMD64BEClassFactory
 *
 *    \date    08/24/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4FiascoAMD64ClassFactory.h"
#include "L4FiascoAMD64Sizes.h"
#include "Compiler.h"

CL4FiascoAMD64BEClassFactory::CL4FiascoAMD64BEClassFactory()
: CL4FiascoBEClassFactory()
{ }

/**    \brief the destructor of this class */
CL4FiascoAMD64BEClassFactory::~CL4FiascoAMD64BEClassFactory()
{ }

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4FiascoAMD64BEClassFactory::GetNewSizes()
{
    CCompiler::Verbose("CL4FiascoAMD64BEClassFactory: created class CL4FiascoBESizes\n");
    return new CL4FiascoAMD64BESizes();
}

