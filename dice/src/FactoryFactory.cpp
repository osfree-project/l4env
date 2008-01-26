/**
 *  \file    dice/src/FactoryFactory.cpp
 *  \brief   contains the implementation of the class CFactoryFactory
 *
 *  \date    09/04/2007
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

#include "FactoryFactory.h"
#include "Compiler.h"
// L4 specific
#include "be/l4/L4BENameFactory.h"
// L4V2
#include "be/l4/v2/L4V2BEClassFactory.h"
#include "be/l4/v2/L4V2BENameFactory.h"
// L4V2 AMD64
#include "be/l4/v2/amd64/V2AMD64ClassFactory.h"
#include "be/l4/v2/amd64/V2AMD64NameFactory.h"
// L4V2 IA32
#include "be/l4/v2/ia32/V2IA32ClassFactory.h"
// L4.Fiasco
#include "be/l4/fiasco/L4FiascoBEClassFactory.h"
#include "be/l4/fiasco/L4FiascoBENameFactory.h"
// L4.Fiasco AMD64
#include "be/l4/fiasco/amd64/L4FiascoAMD64ClassFactory.h"
#include "be/l4/fiasco/amd64/L4FiascoAMD64NameFactory.h"
// L4V4
#include "be/l4/v4/L4V4BEClassFactory.h"
#include "be/l4/v4/ia32/L4V4IA32ClassFactory.h"
#include "be/l4/v4/L4V4BENameFactory.h"
// Sockets
#include "be/sock/SockBEClassFactory.h"
// Language C
//#include "be/lang/c/LangCClassFactory.h"
// Language C++
//#include "be/lang/cxx/LangCPPClassFactory.h"
#include <cassert>

/** \brief get the appropriate instance of the class factory
 *  \return reference to class factory
 */
CBEClassFactory* CFactoryFactory::GetNewClassFactory()
{
	CBEClassFactory *pCF = 0;
	if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V2))
	{
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			pCF = new CL4V2AMD64BEClassFactory();
		else if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_IA32))
			pCF = new CL4V2IA32BEClassFactory();
		else
			pCF = new CL4V2BEClassFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_X0))
	{
		pCF = new CL4V2BEClassFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_FIASCO))
	{
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			pCF = new CL4FiascoAMD64BEClassFactory();
		else
			pCF = new CL4FiascoBEClassFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V4))
	{
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_IA32))
			pCF = new CL4V4IA32ClassFactory();
		else
			pCF = new CL4V4BEClassFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_SOCKETS))
		pCF = new CSockBEClassFactory();
	assert(pCF);
	return pCF;
}

/** \brief get the appropriate instance of the name factory
 *  \return reference to name factory
 */
CBENameFactory* CFactoryFactory::GetNewNameFactory()
{
	CBENameFactory *pNF = 0;
	if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V2))
	{
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			pNF = new CL4V2AMD64BENameFactory();
		else
			pNF = new CL4V2BENameFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_X0))
	{
		pNF = new CL4V2BENameFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_FIASCO))
	{
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			pNF = new CL4FiascoAMD64BENameFactory();
		else
			pNF = new CL4FiascoBENameFactory();
	}
	else if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V4))
		pNF = new CL4V4BENameFactory();
	else
		pNF = new CBENameFactory();
	assert(pNF);
	return pNF;
}
