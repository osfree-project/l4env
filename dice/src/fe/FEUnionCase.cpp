/**
 *	\file	dice/src/fe/FEUnionCase.cpp
 *	\brief	contains the implementation of the class CFEUnionCase
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "fe/FEUnionCase.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEExpression.h"
#include "Vector.h"
#include "Compiler.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEUnionCase) 

CFEUnionCase::CFEUnionCase()
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionCase, CFEBase);

    m_bDefault = false;
    m_pUnionArm = 0;
    m_pUnionCaseLabelList = 0;
}

CFEUnionCase::CFEUnionCase(CFETypedDeclarator * pUnionArm, Vector * pCaseLabels)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionCase, CFEBase);

    m_bDefault = (!pCaseLabels) ? true : false;
    m_pUnionArm = pUnionArm;
    m_pUnionCaseLabelList = pCaseLabels;
}

CFEUnionCase::CFEUnionCase(CFEUnionCase & src):CFEBase(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionCase, CFEBase);

    m_bDefault = src.m_bDefault;
    if (src.m_pUnionArm)
      {
	  m_pUnionArm = (CFETypedDeclarator *) (src.m_pUnionArm->Clone());
	  m_pUnionArm->SetParent(this);
      }
    else
	m_pUnionArm = 0;
    if (src.m_pUnionCaseLabelList)
      {
	  m_pUnionCaseLabelList = src.m_pUnionCaseLabelList->Clone();
	  m_pUnionCaseLabelList->SetParentOfElements(this);
      }
    else
	m_pUnionCaseLabelList = 0;
}

/** cleans up the union case object */
CFEUnionCase::~CFEUnionCase()
{
    if (m_pUnionArm)
	delete m_pUnionArm;
    if (m_pUnionCaseLabelList)
	delete m_pUnionCaseLabelList;
}

/** retrieves the union arm
 *	\return the typed declarator, which is this union case's arm
 */
CFETypedDeclarator *CFEUnionCase::GetUnionArm()
{
    return m_pUnionArm;
}

/** retrives a pointer to the first union case label
 *	\return an iterator, which points to the first union case label object
 */
VectorElement *CFEUnionCase::GetFirstUnionCaseLabel()
{
    if (!m_pUnionCaseLabelList)
	return 0;
    return m_pUnionCaseLabelList->GetFirst();
}

/** retrieves the next union case label object
 *	\param iter the iterator, which points to the next union case label object
 *	\return the next union case label object
 */
CFEExpression *CFEUnionCase::GetNextUnionCaseLabel(VectorElement * &iter)
{
    if (!m_pUnionCaseLabelList)
	return 0;
    if (!iter)
	return 0;
    CFEExpression *pRet = (CFEExpression *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEUnionCase::Clone()
{
    return new CFEUnionCase(*this);
}

/** \brief test this union case for default
 *	\return true if default case, false if not
 *
 * Returns the value of m_bDefault, which is set in the constructor. Usually
 * all, but one union arm have a case label. The C unions (if included from a
 * header file) have no case labels at all. Thus you can differentiate the
 * C unions from IDL unions by testing all union case for default.
 */
bool CFEUnionCase::IsDefault()
{
    return m_bDefault;
}

/** \brief check integrity of union case
 *  \return true if everything is alright
 *
 * A union case is o.k. if it either has at least one case label or is default
 * and has a union arm, which itself is consistent.
 */
bool CFEUnionCase::CheckConsistency()
{
    if (!IsDefault())
      {
	  if (!m_pUnionCaseLabelList)
	    {
		CCompiler::GccError(this, 0, "A Union case has to be either default or have a switch value");
		return false;
	    }
	  VectorElement *pIter = GetFirstUnionCaseLabel();
	  if (!GetNextUnionCaseLabel(pIter))
	    {
		CCompiler::GccError(this, 0, "A union case has to be either default or have a switch value");
		return false;
	    }
      }
    if (!GetUnionArm())
      {
	  CCompiler::GccError(this, 0, "A union case has to have a typed declarator.");
	  return false;
      }
    return true;
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEUnionCase::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<union_case>\n");
	  pFile->IncIndent();
	  if (IsDefault())
	      pFile->PrintIndent("<label>default</label>\n");
	  else
	    {
		VectorElement *pIter = GetFirstUnionCaseLabel();
		CFEBase *pElement;
		while ((pElement = GetNextUnionCaseLabel(pIter)) != 0)
		  {
		      pFile->PrintIndent("<label>\n");
		      pFile->IncIndent();
		      pElement->Serialize(pFile);
		      pFile->DecIndent();
		      pFile->PrintIndent("</label>\n");
		  }
	    }
	  if (GetUnionArm())
	      GetUnionArm()->Serialize(pFile);
	  pFile->DecIndent();
	  pFile->PrintIndent("</union_case>\n");
      }
}
