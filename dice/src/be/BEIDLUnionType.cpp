/**
 *    \file    dice/src/be/BEIDLUnionType.cpp
 *    \brief   contains the implementation of the class CBEIDLUnionType
 *
 *    \date    03/14/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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

#include "BEIDLUnionType.h"
#include "BEClassFactory.h"
#include "BETypedDeclarator.h"
#include "Compiler.h"
#include "fe/FEIDLUnionType.h"
#include <cassert>

CBEIDLUnionType::CBEIDLUnionType()
 : CBEStructType()
{ }

CBEIDLUnionType::CBEIDLUnionType(CBEIDLUnionType* src)
 : CBEStructType(src)
{
    m_sSwitchName = src->m_sSwitchName;
    m_sUnionName = src->m_sUnionName;
}

/** \brief destructor of this instance */
CBEIDLUnionType::~CBEIDLUnionType()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CBEIDLUnionType* CBEIDLUnionType::Clone()
{
	return new CBEIDLUnionType(this);
}

/** \brief prepares this instance for the code generation
 *  \param pFEType the corresponding front-end attribute
 *  \return true if the code generation was successful
 *
 * This implementation calls the base class' implementatio first to set
 * default values and then adds the members of the struct to this class.
 */
void
CBEIDLUnionType::CreateBackEnd(CFETypeSpec * pFEType)
{
    CCompiler::VerboseI("CBEIDLUnionType::%s(fe) called\n", __func__);

    // get union type
    CFEIDLUnionType *pFEUnion = dynamic_cast<CFEIDLUnionType*>(pFEType);
    assert (pFEUnion);
    // basic init for struct
    CBEStructType::CreateBackEnd(pFEUnion->GetTag(), pFEUnion);
    // create switch variable
    CBEClassFactory *pCF = CBEClassFactory::Instance();
    CBETypedDeclarator *pSwitchVar = pCF->GetNewTypedDeclarator();
    m_Members.Add(pSwitchVar);

    CFETypeSpec *pFESwitchType = pFEUnion->GetSwitchType();
    CBEType *pType = pCF->GetNewType(pFESwitchType->GetType());
    pType->SetParent(pSwitchVar);
    pType->CreateBackEnd(pFESwitchType);

    m_sSwitchName = pFEUnion->GetSwitchVar();
    if (m_sSwitchName.empty())
	m_sSwitchName = "_d";
    pSwitchVar->CreateBackEnd(pType, m_sSwitchName);

    // create union
    CBETypedDeclarator *pUnionVar = pCF->GetNewTypedDeclarator();
    m_Members.Add(pUnionVar);

    pType = pCF->GetNewType(TYPE_UNION);
    pType->SetParent(pUnionVar);
    // remove tag, because union is without tag
    pFEUnion->SetTag(string());
    pType->CreateBackEnd(pFEUnion);

    m_sUnionName = pFEUnion->GetUnionName();
    if (m_sUnionName.empty())
	m_sUnionName = "_u";
    pUnionVar->CreateBackEnd(pType, m_sUnionName);

    CCompiler::VerboseD("CBEIDLUnionType::%s(fe) returns\n", __func__);
}

/** \brief try to find the switch variable
 *  \return a reference to the switch variable
 */
CBETypedDeclarator*
CBEIDLUnionType::GetSwitchVariable()
{
    return m_Members.Find(m_sSwitchName);
}

/** \brief try to find the union member
 *  \return a reference to the union member
 */
CBETypedDeclarator*
CBEIDLUnionType::GetUnionVariable()
{
    return m_Members.Find(m_sUnionName);
}
