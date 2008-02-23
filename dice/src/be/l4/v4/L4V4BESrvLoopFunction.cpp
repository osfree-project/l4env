/**
 *  \file    dice/src/be/l4/v4/L4V4BESrvLoopFunction.cpp
 *  \brief   contains the implementation of the class CL4V4BESrvLoopFunction
 *
 *  \date    01/18/2007
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

#include "L4V4BESrvLoopFunction.h"
#include "L4V4BENameFactory.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"

CL4V4BESrvLoopFunction::CL4V4BESrvLoopFunction()
{ }

/** \brief destructor of target class */
CL4V4BESrvLoopFunction::~CL4V4BESrvLoopFunction()
{ }

/** \brief create this instance of a server loop function
 *  \param pFEInterface the interface to use as reference
 *  \param bComponentSide true if this function is created at component side
 *  \return true if create was successful
 *
 * For the server side we have to add a temporary variable if this function
 * tries to init the indirect string members with a user-provided init
 * function.
 */
void CL4V4BESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
	CL4BESrvLoopFunction::CreateBackEnd(pFEInterface, bComponentSide);

	// init function check
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	CCompiler::Verbose("CL4V4BESrvLoopFunction::%s: for func %s and class at %p\n",
		__func__, GetName().c_str(), pClass);

	if (IsComponentSide() &&
		(CCompiler::IsOptionSet(PROGRAM_INIT_RCVSTRING) ||
		 (pClass &&
		  (pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING) ||
		   pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING_CLIENT) ||
		   pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING_SERVER)))))
	{
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sName = pNF->GetString(CL4V4BENameFactory::STR_INIT_RCVSTR_VARIABLE, 0);
		AddLocalVariable(TYPE_MWORD, true, 0, sName, 0);
	}
}
