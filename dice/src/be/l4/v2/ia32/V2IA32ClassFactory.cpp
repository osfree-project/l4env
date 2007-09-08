/**
 *    \file    dice/src/be/l4/v2/ia32/V2IA32ClassFactory.cpp
 *    \brief   contains the implementation of the class CL4V2IA32BEClassFactory
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

#include "V2IA32ClassFactory.h"
#include "V2IA32Sizes.h"
#include "V2IA32IPC.h"
#include "be/BEContext.h"
#include "Compiler.h"
#include <iostream>

CL4V2IA32BEClassFactory::CL4V2IA32BEClassFactory()
: CL4V2BEClassFactory()
{ }

/**    \brief the destructor of this class */
CL4V2IA32BEClassFactory::~CL4V2IA32BEClassFactory()
{ }

/** \brief creates a new sizes class
 *  \return a reference to the new sizes object
 */
CBESizes * CL4V2IA32BEClassFactory::GetNewSizes()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4V2IA32BEClassFactory: created class CL4V2IA32BESizes\n");
	return new CL4V2IA32BESizes();
}

/** \brief creates new communication class
 *  \return a reference to the new IPC object
 */
CBECommunication* CL4V2IA32BEClassFactory::GetNewCommunication()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4V2IA32BEClassFactory: created class CL4V2IA32BEIPC\n");
	return new CL4V2IA32BEIPC();
}
