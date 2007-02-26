/**
 *	\file	dice/src/fe/FEPrimaryExpression.cpp
 *	\brief	contains the implementation of the class CFEPrimaryExpression
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

#include "defines.h"
#include "fe/FEPrimaryExpression.h"

IMPLEMENT_DYNAMIC(CFEPrimaryExpression)
    
CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, long int nValue)
:CFEExpression(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPrimaryExpression, CFEExpression);

    m_nValue = nValue;
    m_fValue = 0.0f;
    m_pOperand = 0;
}

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, long double fValue)
:CFEExpression(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPrimaryExpression, CFEExpression);

    m_nValue = 0;
    m_fValue = fValue;
    m_pOperand = 0;
}

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, CFEExpression * pOperand)
:CFEExpression(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPrimaryExpression, CFEExpression);

    m_nValue = 0;
    m_fValue = 0.0f;
    m_pOperand = pOperand;
}

CFEPrimaryExpression::CFEPrimaryExpression(CFEPrimaryExpression & src)
:CFEExpression(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPrimaryExpression, CFEExpression);

    m_fValue = src.m_fValue;
    m_nValue = src.m_nValue;
    if (src.m_pOperand)
      {
	  m_pOperand = (CFEExpression *) (src.m_pOperand->Clone());
	  m_pOperand->SetParent(this);
      }
    else
	m_pOperand = 0;
}

/** cleans up the primary expression */
CFEPrimaryExpression::~CFEPrimaryExpression()
{
    if (m_pOperand)
	delete m_pOperand;
}

/** returns the integer value of the expression
 *	\return the integer value of the expression
 * Depending on which type this expression is the return value is:
 * - the return value of the operand,
 * - the integer value or
 * - the float value casted into an integer.
 */
long CFEPrimaryExpression::GetIntValue()
{
    switch (GetType())
      {
      case EXPR_PAREN:
	  return GetOperand()->GetIntValue();
	  break;
      case EXPR_INT:
	  return m_nValue;
	  break;
      case EXPR_FLOAT:
	  return (int) m_fValue;
	  break;
      default:
	  TRACE("CFEPrimaryExpression::GetIntValue unrecognized\n");
	  break;
      }
    return 0;
}

/** \brief checks if this expression is of a certain kind
 *  \param nType the type of expression to check for
 *  \return true if this expression is of the requested type, false otherwise
 *
 * This function returns true if
 * - the operand is of the requested type,
 * - the requested type is the integer type or
 * - the requested type is either float or double
 */
bool CFEPrimaryExpression::IsOfType(TYPESPEC_TYPE nType)
{
    switch (GetType())
      {
      case EXPR_PAREN:
          return GetOperand()->IsOfType(nType);
          break;
      case EXPR_INT:
          return (nType == TYPE_INTEGER);
          break;
      case EXPR_FLOAT:
          return (nType == TYPE_FLOAT || nType == TYPE_DOUBLE || nType == TYPE_LONG_DOUBLE);
          break;
      default:
          break;
      }
    return false;
}

/** returns the float value of this expression
 *	\return the float value of this expression
 */
long double CFEPrimaryExpression::GetFloatValue()
{
    return m_fValue;
}

/** returns the operand of the expression
 *	\return the operand of the expression
 */
CFEExpression *CFEPrimaryExpression::GetOperand()
{
    return m_pOperand;
}

/** create a copy of this object
 *	\return a reference to the new object
 */
CObject *CFEPrimaryExpression::Clone()
{
    return new CFEPrimaryExpression(*this);
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEPrimaryExpression::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  switch (m_nType)
	    {
	    case EXPR_INT:
		pFile->PrintIndent("<expression>%d</expression>\n", m_nValue);
		break;
	    case EXPR_FLOAT:
		pFile->PrintIndent("<expression>%f</expression>\n", m_fValue);
		break;
	    case EXPR_PAREN:
		pFile->PrintIndent("<parenthesis_expression>\n");
		pFile->IncIndent();
		GetOperand()->Serialize(pFile);
		pFile->DecIndent();
		pFile->PrintIndent("</parenthesis_expression>\n");
		break;
	    default:
		break;
	    }
      }
}
