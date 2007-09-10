/**
 *  \file    dice/src/be/BEUnionCase.cpp
 *  \brief   contains the implementation of the class CBEUnionCase
 *
 *  \date    01/15/2002
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

#include "BEUnionCase.h"
#include "BEContext.h"
#include "BEExpression.h"
#include "BETypedDeclarator.h"
#include "BEDeclarator.h"
#include "BEClassFactory.h"
#include "fe/FEUnionCase.h"
#include "Compiler.h"
#include <cassert>

CBEUnionCase::CBEUnionCase()
: m_Labels(0, this)
{
	m_bDefault = false;
}

CBEUnionCase::CBEUnionCase(CBEUnionCase* src)
: CBETypedDeclarator(src),
	m_Labels(src->m_Labels)
{
	m_bDefault = src->m_bDefault;
	m_Labels.Adopt(this);
}

/** \brief destructor of this instance */
CBEUnionCase::~CBEUnionCase()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CBEUnionCase::Clone()
{
	return new CBEUnionCase(this);
}

/** \brief creates the back-end structure for a union case
 *  \param pFEUnionCase the corresponding front-end union case
 *  \return true if code generation was successful
 */
void
CBEUnionCase::CreateBackEnd(CFEUnionCase * pFEUnionCase)
{
	assert(pFEUnionCase);
	// the union arm is the typed declarator we initialize the base class with:
	CBETypedDeclarator::CreateBackEnd(pFEUnionCase->GetUnionArm());
	// now init union case specific stuff
	m_bDefault = pFEUnionCase->IsDefault();
	if (m_bDefault)
		return;

	CBEClassFactory *pCF = CBEClassFactory::Instance();

	vector<CFEExpression*>::iterator iter;
	for (iter = pFEUnionCase->m_UnionCaseLabelList.begin();
		iter != pFEUnionCase->m_UnionCaseLabelList.end();
		iter++)
	{
		CBEExpression *pLabel = pCF->GetNewExpression();
		m_Labels.Add(pLabel);
		pLabel->CreateBackEnd(*iter);
	}
}

/** \brief creates the union case
 *  \param pType the type of the union arm
 *  \param sName the name of the union arm
 *  \param pCaseLabel the case label
 *  \param bDefault true if this is the default arm
 *  \return true if successful
 *
 * If neither pCaseLabel nor bDefault is set, then this is the member of a C
 * style union.
 */
void CBEUnionCase::CreateBackEnd(CBEType *pType,
	string sName,
	CBEExpression *pCaseLabel,
	bool bDefault)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEUnionCase::%s called for %s\n", __func__, sName.c_str());

	CBETypedDeclarator::CreateBackEnd(pType, sName);
	m_bDefault = bDefault;
	if (pCaseLabel)
	{
		m_Labels.Add(pCaseLabel);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEUnionCase::%s returns true\n", __func__);
}

