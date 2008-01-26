/**
 *  \file    dice/src/be/l4/v2/L4V2BENameFactory.cpp
 *  \brief   contains the implementation of the class CL4V2BENameFactory
 *
 *  \date    03/05/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
 * echnische Universität Dresden, Operating Systems Research Group
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

#include "be/l4/v2/L4V2BENameFactory.h"
#include "be/BEFunction.h"
#include "be/BEClass.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include <cassert>
#include <iostream>

CL4V2BENameFactory::CL4V2BENameFactory()
 : CL4BENameFactory()
{
}

/** \brief destructs this instance
 */
CL4V2BENameFactory::~CL4V2BENameFactory()
{
}

/** \brief generates the variable of the client side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * Check for the timeout attribute and use the environment's timeout if given.
 * Otherwise construt the default client timeout.
 */
string CL4V2BENameFactory::GetTimeoutClientVariable(CBEFunction* pFunction)
{
	if (pFunction->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT))
		return string("L4_IPC_NEVER");

	return CL4BENameFactory::GetTimeoutClientVariable(pFunction);
}

/** \brief generates the variable of the component side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * Check for the timeout attribute and use the environment's timeout if given.
 * Otherwise construt the default server timeout.
 */
string CL4V2BENameFactory::GetTimeoutServerVariable(CBEFunction* pFunction)
{
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
	assert(pClass);

	if (pClass->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT))
	{
		if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V2))
			return string("L4_IPC_SEND_TIMEOUT_0");
		else // X0
			return string("L4_IPC_TIMEOUT(0,1,0,0,0,0)");
	}

	return CL4BENameFactory::GetTimeoutServerVariable(pFunction);
}

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param nSize the size of the type
 */
string CL4V2BENameFactory::GetTypeName(int nType, bool bUnsigned, int nSize)
{
	string sReturn;
	if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_X0) &&
		TYPE_MWORD == nType &&
		true == bUnsigned)
	{
		sReturn = "dword_t";
	}
	else
		sReturn = CL4BENameFactory::GetTypeName(nType, bUnsigned, nSize);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CL4V2BENameFactory::%s Generated type name \"%s\" for type code %d\n",
		__func__, sReturn.c_str(), nType);
	return sReturn;
}
