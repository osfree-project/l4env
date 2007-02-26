/**
 *	\file	dice/src/fe/FEEnumType.cpp
 *	\brief	contains the implementation of the class CFEEnumType
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

#include "fe/FEEnumType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEIdentifier.h"
#include "Vector.h"
#include "Compiler.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEEnumType) 

CFEEnumType::CFEEnumType(Vector * pMembers)
:CFEConstructedType(TYPE_ENUM)
{
    IMPLEMENT_DYNAMIC_BASE(CFEEnumType, CFEConstructedType);

    m_pMembers = pMembers;
}

CFEEnumType::CFEEnumType(CFEEnumType & src)
:CFEConstructedType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEEnumType, CFEConstructedType);

    if (src.m_pMembers)
    {
        m_pMembers = src.m_pMembers->Clone();
        m_pMembers->SetParentOfElements(this);
    }
    else
        m_pMembers = 0;
}

/** clean up the enum type (delete the members) */
CFEEnumType::~CFEEnumType()
{
    if (m_pMembers)
	delete m_pMembers;
}

/** retrieves a pointer to the first member
 *	\return an iterator, which points to the first member
 */
VectorElement *CFEEnumType::GetFirstMember()
{
    if (!m_pMembers)
        return 0;
    return m_pMembers->GetFirst();
}

/** retrieves the next member
 *	\param iter the iterator, which points to the next member
 *	\return a reference to the next memeber
 */
CFEIdentifier *CFEEnumType::GetNextMember(VectorElement * &iter)
{
    if (!m_pMembers)
        return 0;
    if (!iter)
        return 0;
    CFEIdentifier *pRet = (CFEIdentifier *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** copies the object
 *	\return a reference to a new enumeration type object
 */
CObject *CFEEnumType::Clone()
{
    return new CFEEnumType(*this);
}

/** \brief checks consitency
 *  \return false if error occurs, true if everything is fine
 *
 * A enum is consistent if it contains at least one member.
 */
bool CFEEnumType::CheckConsistency()
{
    if (m_pMembers)
      {
	  if (m_pMembers->GetSize() > 0)
	      return true;
      }
    // no members:
    CCompiler::GccError(this, 0, "An enum should contain at least one member.");
    return false;
}

/** serializes this object
 *	\param pFile the file to serialize to/from
 */
void CFEEnumType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<enum_type>\n");
	  pFile->IncIndent();
	  if (IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
	    {
		pFile->PrintIndent("<tag>%s</tag>\n",
				   (const char
				    *) (((CFETaggedEnumType *) this)->GetTag()));
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
	  pFile->PrintIndent("</enum_type>\n");
      }
}
