/**
 *  \file   dice/src/be/l4/v4/ia32/L4V4IA32ClassFactory.cpp
 *  \brief  contains the implementation of the class CL4V4IA32ClassFactory
 *
 *  \date   02/08/2004
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/l4/v4/ia32/L4V4IA32ClassFactory.h"
#include "be/l4/v4/ia32/L4V4IA32IPC.h"
#include "Compiler.h"
#include <iostream>

CL4V4IA32ClassFactory::CL4V4IA32ClassFactory()
 : CL4V4BEClassFactory()
{
}

/** destroys this class factory */
CL4V4IA32ClassFactory::~CL4V4IA32ClassFactory()
{
}

/** \brief create a new instance of the CL4V4BEIPC class
 *  \return a reference to the new instance
 */
CBECommunication* CL4V4IA32ClassFactory::GetNewCommunication()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CL4V4IA32ClassFactory: created class CL4V4IA32IPC\n");
    return new CL4V4IA32IPC();
}
