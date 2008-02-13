/**
 *  \file    dice/src/be/BEEnumType.cpp
 *  \brief   contains the implementation of the class CBEEnumType
 *
 *  \date    Tue Jul 23 2002
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

#include "BEEnumType.h"
#include "BEFile.h"
#include "BETypedef.h"
#include "BEDeclarator.h"
#include "BEExpression.h"
#include "BESizes.h"
#include "Compiler.h"
#include "fe/FEEnumType.h"
#include "fe/FEEnumDeclarator.h"
#include <cassert>
#include <limits>

CBEEnumType::CBEEnumType()
: m_Members(0, this)
{ }

/** destroys the enum type */
CBEEnumType::~CBEEnumType()
{ }

/** \brief creates the enum type
 *  \param pFEType the front-end type to use as reference
 *  \return true if successful
 */
void
CBEEnumType::CreateBackEnd(CFETypeSpec *pFEType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEEnumType::%s(fe) called\n",
		__func__);

	CBEType::CreateBackEnd(pFEType);

	// extract members
	CFEEnumType *pFEEnumType = static_cast<CFEEnumType*>(pFEType);
	vector<CFEIdentifier*>::iterator iterI;
	for (iterI =  pFEEnumType->m_Members.begin();
		iterI != pFEEnumType->m_Members.end();
		iterI++)
	{
		AddMember(dynamic_cast<CFEEnumDeclarator*>(*iterI));
	}
	// check tagged
	m_sTag = pFEEnumType->GetTag();
	// try to figure out size for enum
	// get last element and calculate it's integer value. Test this for
	// unsigned char, unsigned short, unsigned int
	if (m_Members.size() > 0)
	{
		CBESizes *pSizes = CCompiler::GetSizes();
		CBEDeclarator *pDeclarator = m_Members.back();
		unsigned long nVal = GetIntValue(pDeclarator->GetName());
		if (nVal < std::numeric_limits<unsigned char>::max())
			m_nSize = pSizes->GetSizeOfType(TYPE_BYTE);
		else if (nVal < std::numeric_limits<unsigned short>::max())
			m_nSize = pSizes->GetSizeOfType(TYPE_INTEGER, 2);
		else if (nVal < std::numeric_limits<unsigned int>::max())
			m_nSize = pSizes->GetSizeOfType(TYPE_INTEGER, 4);
		else
			// default to mword
			m_nSize = pSizes->GetSizeOfType(TYPE_MWORD);
		m_nMaxSize = m_nSize;
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEEnumType::%s(fe) returns\n",
		__func__);
}

/** \brief adds a member to this enumeration
 *  \param pFEDeclarator the enumerator
 */
void CBEEnumType::AddMember(CFEEnumDeclarator* pFEDeclarator)
{
	if (!pFEDeclarator)
		return;

	CBEDeclarator *pDeclarator = new CBEDeclarator();
	m_Members.Add(pDeclarator);
	pDeclarator->CreateBackEnd(pFEDeclarator);
}

/** \brief writes the enum type to the target file
 *  \param pFile the file to write to
 */
void CBEEnumType::Write(CBEFile& pFile)
{
	// open enum
	pFile << m_sName;
	if (!m_sTag.empty())
		pFile << " " << m_sTag;

	if (!m_Members.empty())
		pFile << "\t { ";
	// only print member if we got some
	bool bComma = false;
	vector<CBEDeclarator*>::iterator i;
	for (i = m_Members.begin(); i != m_Members.end(); i++)
	{
		if (bComma)
			pFile << ", ";
		(*i)->WriteDeclaration(pFile);
	}

	// close enum
	if (!m_Members.empty())
		pFile << "\t } ";
}

/** \brief write the initialization of a enum with the 'zero' element
 *  \param pFile the file to write to
 *
 *  The 'zero' or initial element of an enum is associated with its declarator. Therefore we could
 * only guess. What we can do is to use the first element of the enum.
 */
void CBEEnumType::WriteZeroInit(CBEFile& pFile)
{
	if (m_Members.empty())
		pFile << "0";
	else
	{
		CBEExpression *pExpr = m_Members[0]->GetInitialValue();
		if (pExpr)
			pFile << pExpr->GetIntValue();
		else
			pFile << "0";
	}
}

/** \brief tests if the enum type has the given tag
 *  \param sTag the tag to test for
 *  \return true if the given tag is the same as the local tag
 */
bool CBEEnumType::HasTag(std::string sTag)
{
	return (m_sTag == sTag);
}

/** \brief return tag on demand
 *  \return tag
 */
std::string CBEEnumType::GetTag()
{
	return m_sTag;
}

/** \brief writes a cast of this type
 *  \param str the string to write to
 *  \param bPointer true if the cast should produce a pointer
 *
 * A enum cast is '(enum tag)'.
 */
void CBEEnumType::WriteCastToStr(std::string& str,  bool bPointer)
{
	str += "(";
	if (m_sTag.empty())
	{
		// no tag -> we need a typedef to save us
		// the alias can be used for the cast
		CBETypedef *pTypedef = GetTypedef();
		assert(pTypedef);
		// get first declarator (without stars)
		vector<CBEDeclarator*>::iterator iterD;
		for (iterD = pTypedef->m_Declarators.begin();
			iterD != pTypedef->m_Declarators.end();
			iterD++)
		{
			if ((*iterD)->GetStars() <= (bPointer?1:0))
				break;
		}
		assert(iterD != pTypedef->m_Declarators.end());
		str += (*iterD)->GetName();
		if (bPointer && ((*iterD)->GetStars() == 0))
			str += "*";
	}
	else
	{
		str += m_sName + " " + m_sTag;
		if (bPointer)
			str += "*";
	}
	str += ")";
}

/** \brief calculate the enumeration value of the given member
 *  \param sName the name of the enumerator
 *  \return its enumeration value
 *
 * The enumeration value is calculated such, that an enumerator can have an
 * explicit value assigned or its one more than its predecessor. If the first
 * enumerator has no assigned value, its value is 0 (zero).
 */
long int CBEEnumType::GetIntValue(std::string sName)
{
	CBEDeclarator *pDeclarator = m_Members.Find(sName);
	if (!pDeclarator)
		return -1;

	long nValue = -1;
	vector<CBEDeclarator*>::iterator i;
	for (i = m_Members.begin(); i != m_Members.end(); i++)
	{
		CBEExpression *pValue = (*i)->GetInitialValue();
		if (pValue)
			nValue = pValue->GetIntValue();
		else
			nValue++;

		if (pDeclarator == *i)
			return nValue;
	}

	return -1;
}
