/**
 *	\file	dice/src/fe/FEConstDeclarator.cpp
 *	\brief	contains the implementation of the class CFEConstDeclarator
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

#include "fe/FEConstDeclarator.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEExpression.h"
#include "fe/FEFile.h"
#include "Compiler.h"

IMPLEMENT_DYNAMIC(CFEConstDeclarator)
    
CFEConstDeclarator::CFEConstDeclarator(CFETypeSpec * pConstType, String sConstName, CFEExpression * pConstValue)
{
    IMPLEMENT_DYNAMIC_BASE(CFEConstDeclarator, CFEInterfaceComponent);

    m_pConstType = pConstType;
    m_sConstName = sConstName;
    m_pConstValue = pConstValue;
}

CFEConstDeclarator::CFEConstDeclarator(CFEConstDeclarator & src)
:CFEInterfaceComponent(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEConstDeclarator, CFEInterfaceComponent);

    m_sConstName = src.m_sConstName;
    if (src.m_pConstType)
      {
	  m_pConstType = (CFETypeSpec *) (src.m_pConstType->Clone());
	  m_pConstType->SetParent(this);
      }
    else
	m_pConstType = 0;
    if (src.m_pConstValue)
      {
	  m_pConstValue = (CFEExpression *) (src.m_pConstValue->Clone());
	  m_pConstValue->SetParent(this);
      }
    else
	m_pConstValue = 0;
}

/** cleans up the constant declarator (frees all members) */
CFEConstDeclarator::~CFEConstDeclarator()
{
    if (m_pConstType)
	delete m_pConstType;
    if (m_pConstValue)
	delete m_pConstValue;
}

/** returns the type of the constant
 *	\return the type of the constant
 */
CFETypeSpec *CFEConstDeclarator::GetType()
{
    return m_pConstType;
}

/** returns the name of the constant
 *	\return the name of the constant
 */
String CFEConstDeclarator::GetName()
{
    return m_sConstName;
}

/** returns the value (the expression) of the constant
 *	\return the value (the expression) of the constant
 */
CFEExpression *CFEConstDeclarator::GetValue()
{
    return m_pConstValue;
}

/**	\brief creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEConstDeclarator::Clone()
{
    return new CFEConstDeclarator(*this);
}

/** \brief checks the consistnec of a const declaration
 *  \return true if everything is fine, false otherwise
 *
 * A const declarator if ok if the expression has the specified type. It's name
 * must be globally unique.
 */
bool CFEConstDeclarator::CheckConsistency()
{
    CFEFile *pRoot = GetRoot();
    assert(pRoot);
    // try to find me
    if (GetName().IsEmpty())
	{
	  CCompiler::GccError(this, 0, "A constant without a name has been defined.");
	  return false;
      }
    // see if this constant exists somewhere
    if (!(pRoot->FindConstDeclarator(GetName())))
      {
	  CCompiler::GccError(this, 0,
			      "The chaining of the front-end classes is wrong - please contact\ndice@os.inf.tu-dresden.de with a description of this error.");
	  return false;
      }
    // check if it is really me
    if (pRoot->FindConstDeclarator(GetName()) != this)
      {
	  CCompiler::GccError(this, 0, "The constant %s is defined twice.",
			      (const char *) GetName());
	  return false;
      }
    // found me     - now check the type
    CFETypeSpec* pType = GetType();
    while (pType && (pType->GetType() == TYPE_USER_DEFINED))
    {
        String sTypeName = ((CFEUserDefinedType*)pType)->GetName();
	CFETypedDeclarator *pTypedef = pRoot->FindUserDefinedType(sTypeName);
	if (!pTypedef)
	{
	    CCompiler::GccError(this, 0, "The type (%s) of expression \"%s\" is not defined.\n", (const char*)sTypeName, (const char*)GetName());
	    return false;
	}
	pType = pTypedef->GetType();
    }
    if (!(GetValue()->IsOfType(pType->GetType())))
    {
	CCompiler::GccError(this, 0, "The expression of %s does not match its type.", (const char *) GetName());
	return false;
    }
    // all checks done
    return true;
}

/** serialize this object
 *	\param pFile the file to serialize to/from
 */
void CFEConstDeclarator::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<constant>\n");
	  pFile->IncIndent();
	  pFile->PrintIndent("<name>%s</name>\n", (const char *) GetName());
	  GetType()->Serialize(pFile);
	  GetValue()->Serialize(pFile);
	  pFile->DecIndent();
	  pFile->PrintIndent("</constant>\n");
      }
}
