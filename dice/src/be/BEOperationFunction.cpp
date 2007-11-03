/**
 * \file    dice/src/be/BEOperationFunction.cpp
 * \brief   contains the implementation of the class CBEOperationFunction
 *
 * \date    01/14/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BEOperationFunction.h"
#include "BEContext.h"
#include "BEAttribute.h"
#include "BEType.h"
#include "BETypedDeclarator.h"
#include "BEDeclarator.h"
#include "BEClass.h"
#include "BEMarshaller.h"
#include "BERoot.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include "Attribute-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"
#include <cassert>

CBEOperationFunction::CBEOperationFunction(FUNCTION_TYPE nFunctionType)
    : CBEFunction(nFunctionType)
{ }

/** \brief destructor of target class */
CBEOperationFunction::~CBEOperationFunction()
{ }

/** \brief prepares this class for further deployment using the front-end \
 *         operation
 *  \param pFEOperation the respective front.end operation
 *  \return true if the code generation was succesful
 *
 * This implementation adds the attributes, types, parameters, exception, etc.
 * given by the front-end function to this instance of the back-end function.
 */
void CBEOperationFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	assert(pFEOperation);

	// basic init
	CBEFunction::CreateBackEnd(pFEOperation, bComponentSide);
	// add return type
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sReturn = pNF->GetReturnVariable();
	SetReturnVar(pFEOperation->GetReturnType(), sReturn);
	// add attributes
	AddAttributes(pFEOperation);
	// add parameters
	AddParameters(pFEOperation);
	// add exceptions
	AddExceptions(pFEOperation);
	// set opcode name
	m_sOpcodeConstName = pNF->GetOpcodeConst(pFEOperation);
	// would like to test for class == parent, but this is not the case for
	// switch case: parent = srv-loop function

	// check if interface has error function and add its name if available
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	if (pFEInterface)
	{
		if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION))
		{
			CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
				(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION));
			m_sErrorFunction = pErrorFunc->GetString();
		}
		if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT) &&
			!IsComponentSide())
		{
			CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
				(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_CLIENT));
			m_sErrorFunction = pErrorFunc->GetString();
		}
		if (pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER) &&
			IsComponentSide())
		{
			CFEStringAttribute *pErrorFunc = dynamic_cast<CFEStringAttribute*>
				(pFEInterface->m_Attributes.Find(ATTR_ERROR_FUNCTION_SERVER));
			m_sErrorFunction = pErrorFunc->GetString();
		}
	}
}

/** \brief adds the parameters of a front-end function to this class
 *  \param pFEOperation the front-end function
 */
void CBEOperationFunction::AddParameters(CFEOperation * pFEOperation)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEOperationFunction::%s called for %s\n", __func__,
		pFEOperation->GetName().c_str());

	AddBeforeParameters();

	for_each(pFEOperation->m_Parameters.begin(), pFEOperation->m_Parameters.end(),
		DoCall<CBEOperationFunction, CFETypedDeclarator>(this, &CBEOperationFunction::AddParameter));

	AddAfterParameters();

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEOperationFunction::%s returns true\n", __func__);
}

/** \brief adds a single parameter to this class
 *  \param pFEParameter the parameter to add
 */
void CBEOperationFunction::AddParameter(CFETypedDeclarator * pFEParameter)
{
	CBETypedDeclarator *pParameter =
		CBEClassFactory::Instance()->GetNewTypedDeclarator();
	m_Parameters.Add(pParameter);
	pParameter->CreateBackEnd(pFEParameter);
}

/** \brief adds exceptions of a front-end function to this class
 *  \param pFEOperation the front-end function
 *  \return true if successful
 */
void CBEOperationFunction::AddExceptions(CFEOperation * pFEOperation)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEOperationFunction::%s called for %s\n", __func__,
		pFEOperation->GetName().c_str());

	for_each(pFEOperation->m_RaisesDeclarators.begin(), pFEOperation->m_RaisesDeclarators.end(),
		DoCall<CBEOperationFunction, CFEIdentifier>(this, &CBEOperationFunction::AddException));

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEOperationFunction::%s returns true\n", __func__);
}

/** \brief adds a single exception to this class
 *  \param pFEException the exception to add
 *  \return true if successful
 */
void CBEOperationFunction::AddException(CFEIdentifier * pFEException)
{
    CBEDeclarator *pException = CBEClassFactory::Instance()->GetNewDeclarator();
    m_Exceptions.Add(pException);
    pException->CreateBackEnd(pFEException);
}

/** \brief adds attributes of a front-end function this this class
 *  \param pFEOperation the front-end operation
 *  \return true if successful
 */
void CBEOperationFunction::AddAttributes(CFEOperation * pFEOperation)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEOperationFunction::%s called for %s\n", __func__,
		pFEOperation->GetName().c_str());

	for_each(pFEOperation->m_Attributes.begin(), pFEOperation->m_Attributes.end(),
		DoCall<CBEOperationFunction, CFEAttribute>(this, &CBEOperationFunction::AddAttribute));

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEOperationFunction::%s returns true\n", __func__);
}

/** \brief adds a single attribute to this function
 *  \param pFEAttribute the attribute to add
 *  \return true if successful
 */
void CBEOperationFunction::AddAttribute(CFEAttribute * pFEAttribute)
{
	// check if attribute is for function or return type
	ATTR_TYPE nType = pFEAttribute->GetAttrType();
	switch (nType)
	{
	case ATTR_IDEMPOTENT:
	case ATTR_BROADCAST:
	case ATTR_MAYBE:
	case ATTR_REFLECT_DELETIONS:
	case ATTR_UUID:
	case ATTR_UUID_RANGE:
	case ATTR_NOOPCODE:
	case ATTR_NOEXCEPTIONS:
	case ATTR_ALLOW_REPLY_ONLY:
	case ATTR_IN:
	case ATTR_OUT:
	case ATTR_STRING:
	case ATTR_CONTEXT_HANDLE:
	case ATTR_SCHED_DONATE:
	case ATTR_DEFAULT_TIMEOUT:
		/* keep attribute */
		{
			CBEAttribute *pAttribute = CBEClassFactory::Instance()->GetNewAttribute();
			m_Attributes.Add(pAttribute);
			pAttribute->CreateBackEnd(pFEAttribute);
		}
		break;
	default:
		{
			CBETypedDeclarator *pReturn = GetReturnVariable();
			pReturn->AddAttribute(pFEAttribute);
		}
		break;
	}
}

