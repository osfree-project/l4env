/**
 *	\file	dice/src/fe/FEUnionType.cpp
 *	\brief	contains the implementation of the class CFEUnionType
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

#include "fe/FEUnionType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FEUnionCase.h"
#include "Vector.h"
#include "Compiler.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEUnionType) CFEUnionType::CFEUnionType(CFETypeSpec * pSwitchType, String sSwitchVar, Vector * pUnionBody, String sUnionName)
: CFEConstructedType(TYPE_UNION)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionType, CFEConstructedType);

    m_bNE = false;
    m_pSwitchType = pSwitchType;
    m_pUnionBody = pUnionBody;
}

CFEUnionType::CFEUnionType(Vector * pUnionBody)
: CFEConstructedType(TYPE_UNION)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionType, CFEConstructedType);

    m_bNE = true;
    m_pSwitchType = 0;
    m_pUnionBody = pUnionBody;
}

CFEUnionType::CFEUnionType(CFEUnionType & src)
: CFEConstructedType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUnionType, CFEConstructedType);

    m_bNE = src.m_bNE;
    m_sSwitchVar = src.m_sSwitchVar;
    m_sUnionName = src.m_sUnionName;
    if (src.m_pSwitchType != 0)
      {
	  m_pSwitchType = (CFETypeSpec *) (src.m_pSwitchType->Clone());
	  m_pSwitchType->SetParent(this);
      }
    else
	m_pSwitchType = 0;
    if (src.m_pUnionBody != 0)
      {
	  m_pUnionBody = src.m_pUnionBody->Clone();
	  m_pUnionBody->SetParentOfElements(this);
      }
    else
	m_pUnionBody = 0;
}

/** cleans up a union type object */
CFEUnionType::~CFEUnionType()
{
    if (m_pSwitchType)
	delete m_pSwitchType;
    if (m_pUnionBody)
	delete m_pUnionBody;
}

/** retrieves the type of the switch variable
 *	\return the type of the switch variable
 */
CFETypeSpec *CFEUnionType::GetSwitchType()
{
    return m_pSwitchType;
}

/** retrieves the name of the switch variable
 *	\return the name of the switch variable
 */
String CFEUnionType::GetSwitchVar()
{
    return m_sSwitchVar;
}

/** retrieves the name of the union
 *	\return an identifier containing the name of the union
 */
String CFEUnionType::GetUnionName()
{
    return m_sUnionName;
}

/** retrives a pointer to the first union case
 *	\return an iterator which points to the first union case object
 */
VectorElement *CFEUnionType::GetFirstUnionCase()
{
    if (!m_pUnionBody)
	return 0;
    return m_pUnionBody->GetFirst();
}

/** \brief retrieves the next union case object
 *  \param iter the iterator, which points to the next union case object
 *  \return the next union case object
 */
CFEUnionCase *CFEUnionType::GetNextUnionCase(VectorElement * &iter)
{
    if (!m_pUnionBody)
	return 0;
    if (!iter)
	return 0;
    CFEUnionCase *pRet = (CFEUnionCase *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** checks if this is a non-encapsulated union
 *	\return true if this object is a non-encapsulated union
 */
bool CFEUnionType::IsNEUnion()
{
    return m_bNE;
}

/** creates a copy of this object
 *	\return a reference to a new union type object
 */
CObject *CFEUnionType::Clone()
{
    return new CFEUnionType(*this);
}

/** \brief checks the integrity of an union
 *  \return true if everything is fine
 *
 * A union is consistent if it has a union body and the elements of this body are
 * consistent.
 */
bool CFEUnionType::CheckConsistency()
{
    if (!m_pUnionBody)
    {
        CCompiler::GccError(this, 0, "A union without members is not allowed.");
        return false;
    }
    VectorElement *pIter = GetFirstUnionCase();
    CFEUnionCase *pUnionCase;
    while ((pUnionCase = GetNextUnionCase(pIter)) != 0)
    {
        if (!(pUnionCase->CheckConsistency()))
            return false;
    }
    return true;
}

/** serialize this object
 *	\param pFile the file to serialize to/from
 */
void CFEUnionType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<union_type>\n");
	  pFile->IncIndent();
	  if (IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)))
	    {
		pFile->PrintIndent("<tag>%s</tag>\n",
				   (const char *) ((CFETaggedUnionType *)
						   this)->GetTag());
	    }
	  if (!GetUnionName().IsEmpty())
	      pFile->PrintIndent("<name>%s</name>\n", (const char *) GetUnionName());
	  if (GetSwitchType() != 0)
	    {
		pFile->PrintIndent("<switch_type>\n");
		pFile->IncIndent();
		GetSwitchType()->Serialize(pFile);
		pFile->DecIndent();
		pFile->PrintIndent("</switch_type>\n");
	    }
	  if (!GetSwitchVar().IsEmpty())
	    {
		pFile->PrintIndent("<switch_var>%s</switch_var>\n",
				   (const char *) GetSwitchVar());
	    }
	  VectorElement *pIter = GetFirstUnionCase();
	  CFEBase *pElement;
	  while ((pElement = GetNextUnionCase(pIter)) != 0)
	    {
		pElement->Serialize(pFile);
	    }
	  pFile->DecIndent();
	  pFile->PrintIndent("</union_type>\n");
      }
}

/**	\brief allows to differentiate between CORBA IDL and other IDLs
 */
void CFEUnionType::SetCORBA()
{
    m_bCORBA = true;
}

/**	\brief test if this was part of CORBA IDL
 *	\return true if this was part of CORBA IDL
 */
bool CFEUnionType::IsCORBA()
{
    return m_bCORBA;
}
