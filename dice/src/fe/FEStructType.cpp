/**
 *	\file	dice/src/fe/FEStructType.cpp
 *	\brief	contains the implementation of the class CFEStructType
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

#include "fe/FEStructType.h"
#include "fe/FETaggedStructType.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEStructType) 

CFEStructType::CFEStructType(Vector * pMembers)
: CFEConstructedType(TYPE_STRUCT)
{
    IMPLEMENT_DYNAMIC_BASE(CFEStructType, CFEConstructedType);

    m_pMembers = pMembers;
}

CFEStructType::CFEStructType(CFEStructType & src)
: CFEConstructedType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEStructType, CFEConstructedType);

    if (src.m_pMembers)
      {
	  m_pMembers = src.m_pMembers->Clone();
	  m_pMembers->SetParentOfElements(this);
      }
    else
	m_pMembers = 0;
}

/** cleans up the struct object (delete all members) */
CFEStructType::~CFEStructType()
{
    if (m_pMembers)
	delete m_pMembers;
}

/** retrieves a pointer to the first member
 *	\return an iterator, which points to the first member
 */
VectorElement *CFEStructType::GetFirstMember()
{
    if (!m_pMembers)
	return 0;
    return m_pMembers->GetFirst();
}

/** retrieves the next member of the struct
 *	\param iter the iterator, which points to the next member
 *	\return the next member object
 */
CFETypedDeclarator *CFEStructType::GetNextMember(VectorElement * &iter)
{
    if (!m_pMembers)
	return 0;
    if (!iter)
	return 0;
    CFETypedDeclarator *pRet = (CFETypedDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** copies the struct object
 *	\return a reference to the new struct object
 */
CObject *CFEStructType::Clone()
{
    return new CFEStructType(*this);
}

/** tries to find a member by its name
 *	\param sName the name to look for
 *	\return the member if found, 0 if no such member
 */
CFETypedDeclarator *CFEStructType::FindMember(String sName)
{
    if (!m_pMembers)
	return 0;
    if (sName.IsEmpty())
	return 0;

    // check for a structural seperator ("." or "->")
    int iDot = sName.Find('.');
    int iPtr = sName.Find("->");
    int iUse = (iDot < iPtr) ? iDot : iPtr;
    String sBase,
	sMember;
    if (iUse > 0)
      {
	  sBase = sName.Left(iUse);
	  if (iUse == iDot)
	      sMember = sName.Mid(iDot + 1);
	  else
	      sMember = sName.Mid(iDot + 2);
      }
    else
	sBase = sName;

    VectorElement *pIter = GetFirstMember();
    CFETypedDeclarator *pTD;
    while ((pTD = GetNextMember(pIter)) != 0)
      {
	  if (pTD->FindDeclarator(sBase))
	    {
		if (iUse > 0)
		  {
		      // if the found typed declarator has a constructed type (struct)
		      // search for the second part of the name there
		      if (pTD->GetType()->IsKindOf(RUNTIME_CLASS(CFEStructType)))
			{
			    if (!(((CFEStructType *)(pTD->GetType()))->FindMember(sMember)))
			      {
				  // no nested member with that name found
				  return 0;
			      }
			}
		  }
		// return the found typed declarator
		return pTD;
	    }
      }

    return 0;
}

/** \brief checks consitency
 *  \return false if error occurs, true if everything is fine
 *
 * A struct is consistent if all members are consistent.
 */
bool CFEStructType::CheckConsistency()
{
    VectorElement *pIter = GetFirstMember();
    CFETypedDeclarator *pMember;
    while ((pMember = GetNextMember(pIter)) != 0)
      {
	  if (!(pMember->CheckConsistency()))
	      return false;
      }
    return true;
}

/** serialize this object
 *	\param pFile the file to serialize to/from
 */
void CFEStructType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<struct_type>\n");
	  pFile->IncIndent();
	  if (IsKindOf(RUNTIME_CLASS(CFETaggedStructType)))
	    {
		pFile->PrintIndent("<tag>%s</tag>\n",
				   (const char *) ((CFETaggedStructType *)
						   this)->GetTag());
	    }
	  VectorElement *pIter = GetFirstMember();
	  CFEBase *pElement;
	  while ((pElement = GetNextMember(pIter)) != 0)
	    {
		pFile->PrintIndent("<member>\n");
		pFile->IncIndent();
		pElement->Serialize(pFile);
		pFile->DecIndent();
		pFile->PrintIndent("</member>\n");
	    }
	  pFile->DecIndent();
	  pFile->PrintIndent("</struct_type>\n");
      }
}
