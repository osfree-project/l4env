/**
 *	\file	dice/src/fe/FEBase.cpp
 *	\brief	contains the implementation of the class CFEBase
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

#include "fe/FEBase.h"
#include "fe/FEFile.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FELibrary.h"
#include "fe/FEConstructedType.h"

/////////////////////////////////////////////////////////////////////////////
// Base class

IMPLEMENT_DYNAMIC(CFEBase) 

CFEBase::CFEBase(CObject * pParent)
:CObject(pParent)
{
    IMPLEMENT_DYNAMIC_BASE(CFEBase, CObject);
    m_nSourceLineNb = 0;
}

CFEBase::CFEBase(CFEBase & src)
:CObject(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEBase, CObject);
    m_nSourceLineNb = src.m_nSourceLineNb;
}

/** cleans up the base object */
CFEBase::~CFEBase()
{
    // do not delete parent !
}

/** \brief returns the root file object
 *	\return the root object
 *
 * This function climbs the chain of parents up until it found the top level file.
 * If it is a file itself and has no parent its the top level file itself.
 */
CFEFile *CFEBase::GetRoot()
{
    CObject *pParent = this;
    while (pParent)
    {
        if (!(pParent->GetParent()) && pParent->IsKindOf(RUNTIME_CLASS(CFEFile)))
        {
            return (CFEFile *) pParent;
        }
        pParent = pParent->GetParent();
    }
    return 0;
}

/** returns the parent interface
 *	\return the parent interface
 *	 This function iterates over the chain of parents until it find a class of type
 * CFEInterface.
 */
CFEInterface *CFEBase::GetParentInterface()
{
    CObject *pParent = GetParent();
    while (pParent)
    {
        if (pParent->IsKindOf(RUNTIME_CLASS(CFEInterface)))
            return (CFEInterface *) pParent;
        pParent = pParent->GetParent();
    }
    return 0;
}

/** copies the object
 *	\return a reference to the new base object
 */
CObject *CFEBase::Clone()
{
    return new CFEBase(*this);
}

/** returns the parent operation
 *	\return the parent operation
 *	This is similar to CFEBase::GetParentInterface except this functions earches for
 * a class of type CFEOperation. This is useful for parameters etc.
 */
CFEOperation *CFEBase::GetParentOperation()
{
    CObject *pParent = GetParent();
    while (pParent)
    {
        if (pParent->IsKindOf(RUNTIME_CLASS(CFEOperation)))
            return (CFEOperation *) pParent;
        pParent = pParent->GetParent();
    }
    return 0;
}

/** \brief base method for consistency checks
 *  \return true if element is consistent, false if not
 */
bool CFEBase::CheckConsistency()
{
    return true;
}

/**
 *	\brief retrieves the reference to the parent library if any
 *	\return a reference to the parent library
 */
CFELibrary *CFEBase::GetParentLibrary()
{
    CObject *pParent = GetParent();
    while (pParent)
      {
	  if (pParent->IsKindOf(RUNTIME_CLASS(CFELibrary)))
	      return (CFELibrary *) pParent;
	  pParent = pParent->GetParent();
      }
    return 0;
}

/**
 *	\brief retrieves the reference to the parent struct/union/enum if any
 *	\return a reference to the parent struct/union/enum
 */
CFEConstructedType *CFEBase::GetParentConstructedType()
{
    CObject *pParent = GetParent();
    while (pParent)
      {
	  if (pParent->IsKindOf(RUNTIME_CLASS(CFEConstructedType)))
	      return (CFEConstructedType *) pParent;
	  pParent = pParent->GetParent();
      }
    return 0;
}

/**	returns a reference to the file this object belongs to
 *	\return a reference to the file this object belongs to
 */
CFEFile *CFEBase::GetFile()
{
    CObject *pParent = this;
    while (pParent)
    {
        if (pParent->IsKindOf(RUNTIME_CLASS(CFEFile)))
            return (CFEFile *) pParent;
        pParent = pParent->GetParent();
    }
    return 0;
}

/** for debugging purposes only */
void CFEBase::Dump()
{
    printf("Dump: class %s\n", GetClassName());
}

/**	\brief serialize object
 *	\param pFile the file to serialize from/to
 *
 * Writes the object into the specified file. The file-format is XML.
 *
 * This implementation does nothing, because we don't have information to
 * store or retrieve.
 */
void CFEBase::Serialize(CFile * pFile)
{
    // empty, because base has no data to store
	TRACE("not serializing\n");
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
String CFEBase::ToString()
{
    // empty ecause this object is nothing
	return String();
}

/**	\brief sets the source line number of this element
 *	\param nLineNb the line this elements has been declared
 */
void CFEBase::SetSourceLine(int nLineNb)
{
    m_nSourceLineNb = nLineNb;
}

/**	\brief retrieves the source code line number this elements was declared in
 *	\return the line number of declaration
 */
int CFEBase::GetSourceLine()
{
    return m_nSourceLineNb;
}
