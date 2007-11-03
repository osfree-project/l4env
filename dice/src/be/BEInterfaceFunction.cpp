/**
 *  \file    dice/src/be/BEInterfaceFunction.cpp
 *  \brief   contains the implementation of the class CBEInterfaceFunction
 *
 *  \date    01/14/2002
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

#include "BEInterfaceFunction.h"
#include "BEContext.h"
#include "BEType.h"
#include "BETypedDeclarator.h"
#include "BEMsgBuffer.h"
#include "BERoot.h"
#include "fe/FEInterface.h"
#include "TypeSpec-Type.h"
#include "fe/FEStringAttribute.h"
#include "Compiler.h"
#include "Error.h"
#include <cassert>

CBEInterfaceFunction::CBEInterfaceFunction(FUNCTION_TYPE nFunctionType)
 : CBEFunction (nFunctionType)
{ }

/** \brief destructor of target class */
CBEInterfaceFunction::~CBEInterfaceFunction()
{ }

/** \brief creates the back-end function for the interface
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 *
 * Create empty return variable.
 */
void CBEInterfaceFunction::CreateBackEnd(CFEInterface *pFEInterface, bool bComponentSide)
{
	assert(pFEInterface);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
	// basic init
	CBEFunction::CreateBackEnd(pFEInterface, bComponentSide);

	// set return type
	SetNoReturnVar();

	// check if interface has error function and add its name if available
	if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION))
	{
		CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
			(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION));
		assert(pErrorFunc);
		m_sErrorFunction = pErrorFunc->GetString();
	}
	if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT) &&
		!IsComponentSide())
	{
		CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
			(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT));
		assert(pErrorFunc);
		m_sErrorFunction = pErrorFunc->GetString();
	}
	if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER) &&
		IsComponentSide())
	{
		CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
			(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER));
		assert(pErrorFunc);
		m_sErrorFunction = pErrorFunc->GetString();
	}
}

/** \brief add the typical parameters of an interface function
 */
void CBEInterfaceFunction::AddParameters()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

	// this adds the CORBA_Object
	AddBeforeParameters();

	// add message buffer
	CBETypedDeclarator *pMsgBuf = GetMessageBuffer();
	if (pMsgBuf)
		m_Parameters.Add(pMsgBuf);

	// this adds the environment
	AddAfterParameters();

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

