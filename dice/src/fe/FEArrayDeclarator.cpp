/**
 *	\file	dice/src/fe/FEArrayDeclarator.cpp
 *	\brief	contains the implementation of the class CFEArrayDeclarator
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "fe/FEArrayDeclarator.h"
#include "fe/FEExpression.h"

IMPLEMENT_DYNAMIC(CFEArrayDeclarator)

CFEArrayDeclarator::CFEArrayDeclarator(CFEDeclarator * pDecl)
: CFEDeclarator(*pDecl),
  m_vLowerBounds(RUNTIME_CLASS(CFEExpression)),
  m_vUpperBounds(RUNTIME_CLASS(CFEExpression))
{
    IMPLEMENT_DYNAMIC_BASE(CFEArrayDeclarator, CFEDeclarator);

    m_nType = DECL_ARRAY;
}

CFEArrayDeclarator::CFEArrayDeclarator(String sName, CFEExpression * pUpper)
: CFEDeclarator(DECL_IDENTIFIER, sName),
  m_vLowerBounds(RUNTIME_CLASS(CFEExpression)),
  m_vUpperBounds(RUNTIME_CLASS(CFEExpression))
{
    IMPLEMENT_DYNAMIC_BASE(CFEArrayDeclarator, CFEDeclarator);

    m_nType = DECL_ARRAY;
    AddBounds(0, pUpper);
}

CFEArrayDeclarator::CFEArrayDeclarator(CFEArrayDeclarator & src)
: CFEDeclarator(src),
  m_vLowerBounds(RUNTIME_CLASS(CFEExpression)),
  m_vUpperBounds(RUNTIME_CLASS(CFEExpression))
{
    IMPLEMENT_DYNAMIC_BASE(CFEArrayDeclarator, CFEDeclarator);

    m_vLowerBounds.Add(&(src.m_vLowerBounds));
    m_vLowerBounds.SetParentOfElements(this);
    m_vUpperBounds.Add(&(src.m_vUpperBounds));
    m_vUpperBounds.SetParentOfElements(this);
}

/** cleans up the array declarator (deletes the bounds) */
CFEArrayDeclarator::~CFEArrayDeclarator()
{
    // vectors will be destroyed automatically
}

/** retrieves a lower bound
 *	\param nDimension the requested dimension
 *	\return the lower boudn of the requested dimension
 * If the dimension is out of range 0 is returned.
 */
CFEExpression *CFEArrayDeclarator::GetLowerBound(int nDimension)
{
    if ((nDimension < 0) || (nDimension > m_vLowerBounds.GetSize() - 1))
        return 0;
    VectorElement *pIter = m_vLowerBounds.GetFirst();
    int i = 0;
    for (; (i < nDimension) && (pIter); i++)
	pIter->GetNext();
    if ((i == nDimension) && (pIter))
      {
	  if (pIter->GetElement())
	    {
		if (((CFEExpression *) pIter->GetElement())->GetType() == EXPR_NONE)
		  {
		      return 0;
		  }
	    }
	  return (CFEExpression *) (pIter->GetElement());
      }
    return 0;
}

/** retrieves a upper bound
 *	\param nDimension the requested dimension
 *	\return the upper bound of the requested dimension
 * If the dimension is out of range 0 is returned.
 */
CFEExpression *CFEArrayDeclarator::GetUpperBound(int nDimension)
{
    if ((nDimension < 0) || (nDimension > m_vLowerBounds.GetSize() - 1))
	return 0;
    VectorElement *pIter = m_vUpperBounds.GetFirst();
    int i = 0;
    for (; (i < nDimension) && (pIter); i++)
	pIter->GetNext();
    if ((i == nDimension) && (pIter))
      {
	  if (pIter->GetElement())
	    {
		if (((CFEExpression *) pIter->GetElement())->GetType() == EXPR_NONE)
		    return 0;
	    }
	  return (CFEExpression *) (pIter->GetElement());
      }
    return 0;
}

/** adds an array bound
 *	\param pLower the lower array bound
 *	\param pUpper the upper array bound
 *	\return the dimension these two bound have been added to (-1 if some error occured)
 */
int CFEArrayDeclarator::AddBounds(CFEExpression * pLower, CFEExpression * pUpper)
{
    if (m_vLowerBounds.GetSize() != m_vUpperBounds.GetSize())
        return -1;
    // if one of the arguments is 0 add a EXPR_NONE element to vector
    // vector does not accept 0 as members
    if (!pLower)
        pLower = new CFEExpression(EXPR_NONE);
    if (!pUpper)
        pUpper = new CFEExpression(EXPR_NONE);
    // add elements to vectors
    m_vLowerBounds.Add(pLower);
    m_vUpperBounds.Add(pUpper);
    // set parent relation
    pLower->SetParent(this);
    pUpper->SetParent(this);
    return m_vLowerBounds.GetSize() - 1;
}

/** retrieves the maximal dimension
 *	\return the size f the bounds collection
 */
int CFEArrayDeclarator::GetDimensionCount()
{
    if (m_vLowerBounds.GetSize() != m_vUpperBounds.GetSize())
	return -1;
    return m_vLowerBounds.GetSize();
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEArrayDeclarator::Clone()
{
    return new CFEArrayDeclarator(*this);
}

/** \brief deletes a dimension from the vounds vectors
 *	\param nIndex the dimension to erase
 *
 * This function removes the bounds without returning any reference to them, so if you
 * like to know where there are (e.g. keep your memory clean) get these expressions first.
 */
void CFEArrayDeclarator::RemoveBounds(int nIndex)
{
    if (m_vLowerBounds.GetSize() != m_vUpperBounds.GetSize())
	return;
    if ((nIndex < 0) || (nIndex > m_vLowerBounds.GetSize() - 1))
	return;
    m_vLowerBounds.RemoveAt(nIndex);
    m_vUpperBounds.RemoveAt(nIndex);
}

/** serializes this object
 *	\param pFile the file to serialize to/from
 */
void CFEArrayDeclarator::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<array_declarator>\n");
	  pFile->IncIndent();
	  pFile->PrintIndent("<name>%s</name>\n", (const char *) GetName());
	  pFile->PrintIndent("<pointers>%d</pointers>\n", GetStars());
	  pFile->PrintIndent("<bitfields>%d</bitfields>\n", GetBitfields());
	  int i;
	  for (i = 0; i < GetDimensionCount(); i++)
	    {
		CFEExpression *pLower = GetLowerBound(i);
		CFEExpression *pUpper = GetUpperBound(i);
		if (pUpper)
		  {
		      pFile->PrintIndent("<dimension>\n");
		      if (pLower)
			{
			    pFile->PrintIndent("<lower_bound>\n");
			    pLower->Serialize(pFile);
			    pFile->PrintIndent("</lower_bound>\n");
			}
		      pFile->PrintIndent("<upper_bound>\n");
		      pUpper->Serialize(pFile);
		      pFile->PrintIndent("</upper_bound>\n");
		      pFile->PrintIndent("</dimension>\n");
		  }
	    }
	  pFile->DecIndent();
	  pFile->PrintIndent("</array_declarator>\n");
      }
}

/**	\brief checks if this parameter is referenced
 *	\return true (an array is also a pointer)
 */
bool CFEArrayDeclarator::IsReference()
{
    return ((GetStars() > 0) || (GetDimensionCount() > 0));
}

/**	\brief replaces the expression at position nIndex with a new expression
 *	\param nIndex the index of the expression to replace
 *	\param pLower the new lower bound
 */
void CFEArrayDeclarator::ReplaceLowerBound(int nIndex, CFEExpression * pLower)
{
    if (m_vLowerBounds.GetSize() != m_vUpperBounds.GetSize())
	return;
    CFEExpression *pOld = (CFEExpression *) m_vLowerBounds.GetAt(nIndex);
    if (m_vLowerBounds.SetAt(nIndex, pLower))
	delete pOld;
    pLower->SetParent(this);
}

/**	\brief replaces the expression at position nIndex with a new expression
 *	\param nIndex the index of the expression to replace
 *	\param pUpper the new upper bound
 */
void CFEArrayDeclarator::ReplaceUpperBound(int nIndex, CFEExpression * pUpper)
{
    if (m_vLowerBounds.GetSize() != m_vUpperBounds.GetSize())
	return;
    CFEExpression *pOld = (CFEExpression *) m_vUpperBounds.GetAt(nIndex);
    if (m_vUpperBounds.SetAt(nIndex, pUpper))
	delete pOld;
    pUpper->SetParent(this);
}
