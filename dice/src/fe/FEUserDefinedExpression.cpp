/**
 *	\file	dice/src/fe/FEUserDefinedExpression.cpp
 *	\brief	contains the implementation of the class CFEUserDefinedExpression
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

#include "fe/FEUserDefinedExpression.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEFile.h"
#include "fe/FEInterface.h"

// needed for Error function
#include "Compiler.h"

IMPLEMENT_DYNAMIC(CFEUserDefinedExpression)

CFEUserDefinedExpression::CFEUserDefinedExpression(String sExpName)
: CFEExpression(EXPR_USER_DEFINED)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUserDefinedExpression, CFEExpression);
    m_sExpName = sExpName;
}

CFEUserDefinedExpression::CFEUserDefinedExpression(CFEUserDefinedExpression & src)
: CFEExpression(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUserDefinedExpression, CFEExpression);
    m_sExpName = src.m_sExpName;
}

/** cleans up user defined expression */
CFEUserDefinedExpression::~CFEUserDefinedExpression()
{

}

/**
 *	\brief returns the expression's name
 *	\return the expression's name
 */
String CFEUserDefinedExpression::GetExpName()
{
    return m_sExpName;
}

/**
 *	\brief calculates the integer value of this expression
 *	\return the integer value of this expression
 *
 * Searches for the definition of this expression and delegates the request to the
 * found expression.
 */
long CFEUserDefinedExpression::GetIntValue()
{
    // find const with root and interface and get its int value
    // because of scope we have to search interface first
    CFEInterface *pInterface = GetParentInterface();
    ASSERT(pInterface);
    CFEConstDeclarator *pConst;
    // for all interface to top most base interface
    while (pInterface)
      {
	  pConst = pInterface->FindConstant(GetExpName());
	  if (pConst)
	      return pConst->GetValue()->GetIntValue();
	  VectorElement *pIterI = pInterface->GetFirstBaseInterface();
	  pInterface = pInterface->GetNextBaseInterface(pIterI);
      }
    // now check if global const with name exists
    pConst = GetRoot()->FindConstDeclarator(GetExpName());
    if (pConst)
	return pConst->GetValue()->GetIntValue();
    // not found
    CCompiler::Error("The const \"%s\" is not defined in the the valid scope(s).\n",
	 (const char *) GetExpName());
    return 0;
}

/**
 *	\brief checks if this expression is of a specific type
 *	\param nType the type to check for
 *	\return true if this expression is of the given type, false otherwise
 */
bool CFEUserDefinedExpression::IsOfType(TYPESPEC_TYPE nType)
{
    // find const with root and interface and get its int value
    // because of scope we have to search interface first
    CFEInterface *pInterface = GetParentInterface();
    ASSERT(pInterface);
    CFEConstDeclarator *pConst;
    // for all interface to top most base interface
    while (pInterface)
      {
	  pConst = pInterface->FindConstant(GetExpName());
	  if (pConst)
	      return pConst->GetValue()->IsOfType(nType);
	  VectorElement *pIterI = pInterface->GetFirstBaseInterface();
	  pInterface = pInterface->GetNextBaseInterface(pIterI);
      }
    // now check if global const with name exists
    pConst = GetRoot()->FindConstDeclarator(GetExpName());
    if (pConst)
	return pConst->GetValue()->IsOfType(nType);
    // not found
    CCompiler::Error("The const \"%s\" is not defined in the valid scope(s).\n",
	      (const char *) GetExpName());
    return false;
}

/**
 *	\brief creates a perfect copy of this class
 *	\return a new versin of ths object
 */
CObject *CFEUserDefinedExpression::Clone()
{
    return new CFEUserDefinedExpression(*this);
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEUserDefinedExpression::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  if (m_nType == EXPR_USER_DEFINED)
	    {
		pFile->PrintIndent("<expression>%s</expression>\n",
				   (const char *) GetExpName());
	    }
      }
}
