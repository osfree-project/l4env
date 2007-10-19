/**
 *  \file    dice/src/be/l4/L4BEMsgBufferType.cpp
 *  \brief   contains the implementation of the class CL4BEMsgBufferType
 *
 *  \date    02/10/2005
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4BEMsgBufferType.h"
#include "L4BENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEStructType.h"
#include "be/BEFunction.h"
#include "be/BEAttribute.h"

#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FETypeSpec.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BEMsgBufferType::CL4BEMsgBufferType()
: CBEMsgBufferType()
{ }

CL4BEMsgBufferType::CL4BEMsgBufferType(CL4BEMsgBufferType* src)
: CBEMsgBufferType(src)
{ }

/** \brief destructor of this instance */
CL4BEMsgBufferType::~CL4BEMsgBufferType()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CL4BEMsgBufferType::Clone()
{
	return new CL4BEMsgBufferType(this);
}

/** \brief adds the elements of the structs
 *  \param pFEOperation the front-end operation to extract the elements from
 *  \param nType the type of the struct
 *  \return true if successful
 *
 *  This method calls the base class and then checks if any flexpages have
 *  been used in the message buffer. If so it adds the delimiter flexpage.
 */
void
CL4BEMsgBufferType::AddElements(CFEOperation *pFEOperation,
	CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s(%s, %d) called\n",
		__func__, pFEOperation->GetName().c_str(), (int)nType);

	CBEMsgBufferType::AddElements(pFEOperation, nType);

	AddZeroFlexpage(pFEOperation, nType);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s returns\n", __func__);
}

/** \brief adds elements for a single parameter to the message buffer
 *  \param pFEParameter the parameter to add the elements for
 *  \param nType the type of the struct
 */
void
CL4BEMsgBufferType::AddElement(CFETypedDeclarator *pFEParameter,
	CMsgStructType nType)
{
	assert(pFEParameter);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s(%s, %d) called\n",
		__func__, pFEParameter->m_Declarators.First()->GetName().c_str(), (int)nType);

	if (pFEParameter->m_Attributes.Find(ATTR_REF))
	{
		AddRefstringElement(pFEParameter, nType);
		return;
	}
	if (pFEParameter->GetType()->GetType() == TYPE_FLEXPAGE)
	{
		AddFlexpageElement(pFEParameter, nType);
		return;
	}

	CBEMsgBufferType::AddElement(pFEParameter, nType);
}

/** \brief adds elements for a refstring parameter to the message buffer
 *  \param pFEParameter the parameter to add the elements for
 *  \param nType the type of the struct
 */
void
CL4BEMsgBufferType::AddRefstringElement(CFETypedDeclarator *pFEParameter,
	CMsgStructType nType)
{
	assert(pFEParameter);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s(%s, %d) called\n",
		__func__, pFEParameter->m_Declarators.First()->GetName().c_str(), (int)nType);
	// get struct
	CFEOperation *pFEOperation =
		pFEParameter->GetSpecificParent<CFEOperation>();
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CBEStructType *pStruct = GetStruct(pFEOperation->GetName(),
		pFEInterface->GetName(), nType);
	assert(pStruct);

	// we have to create a new element for the indirect string part
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pMember = pCF->GetNewTypedDeclarator();
	pMember->SetParent(this);
	pMember->CreateBackEnd(pFEParameter);
	// set the type to refstring (remember the restring type is only used in
	// the message buffer -- up to here its a char*)
	CBEType *pType = pCF->GetNewType(TYPE_REFSTRING);
	pType->CreateBackEnd(true, 0, TYPE_REFSTRING);
	pMember->ReplaceType(pType);
	// set the pointer of the declarator to zero
	pMember->m_Declarators.First()->SetStars(0);

	// add C language property to avoid const qualifier
	// in struct
	pMember->AddLanguageProperty(string("noconst"), string());
	// add to struct
	pStruct->m_Members.Add(pMember);
}

/** \brief adds elements for a flexpage parameter to the message buffer
 *  \param pFEParameter the parameter to add the elements for
 *  \param nType the type of the struct
 */
void
CL4BEMsgBufferType::AddFlexpageElement(CFETypedDeclarator *pFEParameter,
	CMsgStructType nType)
{
	assert(pFEParameter);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s(%s, %d) called\n",
		__func__, pFEParameter->m_Declarators.First()->GetName().c_str(), (int)nType);
	// get struct
	CFEOperation *pFEOperation =
		pFEParameter->GetSpecificParent<CFEOperation>();
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CBEStructType *pStruct = GetStruct(pFEOperation->GetName(),
		pFEInterface->GetName(), nType);
	assert(pStruct);

	// we have to create a new element for the indirect string part
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pMember = pCF->GetNewTypedDeclarator();
	pMember->SetParent(this);
	pMember->CreateBackEnd(pFEParameter);
	// set the pointer of the declarator to zero
	pMember->m_Declarators.First()->SetStars(0);

	// add C language property to avoid const qualifier
	// in struct
	pMember->AddLanguageProperty(string("noconst"), string());
	// add to struct
	pStruct->m_Members.Add(pMember);
}

/** \brief adds a zero flexpage if neccessary
 *  \param pFEOperation the front-end operation containing the parameters
 *  \param nType the type of the struct
 */
void
CL4BEMsgBufferType::AddZeroFlexpage(CFEOperation *pFEOperation,
	CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBufferType::%s(%s, %d) called\n",
		__func__, pFEOperation->GetName().c_str(), (int)nType);
	bool bFlexpage = false;

	vector<CFETypedDeclarator*>::iterator iter;
	for (iter = pFEOperation->m_Parameters.begin();
		iter != pFEOperation->m_Parameters.end();
		iter++)
	{
		if ((*iter)->m_Attributes.Find(ATTR_IGNORE))
			continue;
		if ((CMsgStructType::In == nType) &&
			!(*iter)->m_Attributes.Find(ATTR_IN))
			continue;
		if ((CMsgStructType::Out == nType) &&
			!(*iter)->m_Attributes.Find(ATTR_OUT))
			continue;
		if ((*iter)->GetType()->GetType() == TYPE_FLEXPAGE)
			bFlexpage = true;
	}
	if (!bFlexpage)
		return;

	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CBEStructType *pStruct = GetStruct(pFEOperation->GetName(),
		pFEInterface->GetName(), nType);
	assert(pStruct);

	// we have to create a new element for the indirect string part
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pMember = pCF->GetNewTypedDeclarator();
	pMember->SetParent(this);
	// create flexpage type
	CBEType *pType = pCF->GetNewType(TYPE_FLEXPAGE);
	pType->SetParent(pMember);
	pType->CreateBackEnd(true, 0, TYPE_FLEXPAGE);
	// get name for zero fpage
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetString(CL4BENameFactory::STR_ZERO_FPAGE);
	// now create member
	pMember->CreateBackEnd(pType, sName);
	delete pType; /* cloned in CreateBackEnd */
	// add directional attribute
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pMember);
	if (nType == CMsgStructType::In)
		pAttr->CreateBackEnd(ATTR_IN);
	else
		pAttr->CreateBackEnd(ATTR_OUT);
	pMember->m_Attributes.Add(pAttr);

	// add C language property to avoid const qualifier
	// in struct
	pMember->AddLanguageProperty(string("noconst"), string());
	// add to struct
	pStruct->m_Members.Add(pMember);
}

