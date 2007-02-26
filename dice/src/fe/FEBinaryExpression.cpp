/**
 *	\file	dice/src/fe/FEBinaryExpression.cpp
 *	\brief	contains the implementation of the class CFEBinaryExpression
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

#include "fe/FEBinaryExpression.h"

IMPLEMENT_DYNAMIC(CFEBinaryExpression) 

CFEBinaryExpression::CFEBinaryExpression(EXPR_TYPE nType, 
					 CFEExpression * pOperand, 
					 EXPT_OPERATOR Operator, 
					 CFEExpression * pOperand2)
:CFEUnaryExpression(nType, Operator, pOperand)
{
    IMPLEMENT_DYNAMIC_BASE(CFEBinaryExpression, CFEUnaryExpression);

    m_pOperand2 = pOperand2;
}

CFEBinaryExpression::CFEBinaryExpression(CFEBinaryExpression & src)
:CFEUnaryExpression(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEBinaryExpression, CFEUnaryExpression);

    if (src.m_pOperand2)
      {
	  m_pOperand2 = (CFEExpression *) (src.m_pOperand2->Clone());
	  m_pOperand2->SetParent(this);
      }
    else
	m_pOperand2 = 0;
}

/** cleans up the binary expression (frees the second operand) */
CFEBinaryExpression::~CFEBinaryExpression()
{
    if (m_pOperand2)
	delete m_pOperand2;
}

/** returns the integer value of this expression
 *	\return the integer value of this expression
 * Depending of the operator, the values of the two operands are combined and the result
 * of this operation returned.
 */
long CFEBinaryExpression::GetIntValue()
{
    switch (GetOperator())
      {
      case EXPR_MUL:
	  return GetOperand()->GetIntValue() * GetOperand2()->GetIntValue();
	  break;
      case EXPR_DIV:
	  return GetOperand()->GetIntValue() / GetOperand2()->GetIntValue();
	  break;
      case EXPR_MOD:
	  return GetOperand()->GetIntValue() % GetOperand2()->GetIntValue();
	  break;
      case EXPR_PLUS:
	  return GetOperand()->GetIntValue() + GetOperand2()->GetIntValue();
	  break;
      case EXPR_MINUS:
	  return GetOperand()->GetIntValue() - GetOperand2()->GetIntValue();
	  break;
      case EXPR_LSHIFT:
	  return GetOperand()->GetIntValue() << GetOperand2()->GetIntValue();
	  break;
      case EXPR_RSHIFT:
	  return GetOperand()->GetIntValue() >> GetOperand2()->GetIntValue();
	  break;
      case EXPR_LT:
	  return GetOperand()->GetIntValue() < GetOperand2()->GetIntValue();
	  break;
      case EXPR_GT:
	  return GetOperand()->GetIntValue() > GetOperand2()->GetIntValue();
	  break;
      case EXPR_LTEQU:
	  return GetOperand()->GetIntValue() <= GetOperand2()->GetIntValue();
	  break;
      case EXPR_GTEQU:
	  return GetOperand()->GetIntValue() >= GetOperand2()->GetIntValue();
	  break;
      case EXPR_EQUALS:
	  return GetOperand()->GetIntValue() == GetOperand2()->GetIntValue();
	  break;
      case EXPR_NOTEQUAL:
	  return GetOperand()->GetIntValue() != GetOperand2()->GetIntValue();
	  break;
      case EXPR_BITAND:
	  return GetOperand()->GetIntValue() & GetOperand2()->GetIntValue();
	  break;
      case EXPR_BITXOR:
	  return GetOperand()->GetIntValue() ^ GetOperand2()->GetIntValue();
	  break;
      case EXPR_BITOR:
	  return GetOperand()->GetIntValue() | GetOperand2()->GetIntValue();
	  break;
      case EXPR_LOGAND:
	  return GetOperand()->GetIntValue() && GetOperand2()->GetIntValue();
	  break;
      case EXPR_LOGOR:
	  return GetOperand()->GetIntValue() || GetOperand2()->GetIntValue();
	  break;
      default:
	  break;
      }
    return 0;
}

/** checks the type of this expression
 *	\param nType the type to check for
 *	\return true if this expression is of the requested type
 */
bool CFEBinaryExpression::IsOfType(TYPESPEC_TYPE nType)
{
    switch (GetOperator())
      {
      case EXPR_MUL:
      case EXPR_DIV:
      case EXPR_MOD:
      case EXPR_PLUS:
      case EXPR_MINUS:
      case EXPR_LSHIFT:
      case EXPR_RSHIFT:
	  return (nType == TYPE_INTEGER) && GetOperand()->IsOfType(nType)
	      && GetOperand2()->IsOfType(nType);
	  break;
      case EXPR_LT:
      case EXPR_GT:
      case EXPR_LTEQU:
      case EXPR_GTEQU:
	  return (nType == TYPE_BOOLEAN) && (GetOperand()->IsOfType(TYPE_INTEGER)
		  || GetOperand()->IsOfType(TYPE_CHAR)
		  || GetOperand()->IsOfType(TYPE_CHAR_ASTERISK))
	      && (GetOperand2()->IsOfType(TYPE_INTEGER)
		  || GetOperand2()->IsOfType(TYPE_CHAR)
		  || GetOperand()->IsOfType(TYPE_CHAR_ASTERISK));
	  break;
      case EXPR_EQUALS:
      case EXPR_NOTEQUAL:
	  return (nType == TYPE_BOOLEAN) && 
	  	((GetOperand()->IsOfType(TYPE_INTEGER) && GetOperand2()->IsOfType(TYPE_INTEGER))
		|| (GetOperand()->IsOfType(TYPE_CHAR) && GetOperand2()->IsOfType(TYPE_CHAR))
		|| (GetOperand()->IsOfType(TYPE_BOOLEAN) && GetOperand2()->IsOfType(TYPE_BOOLEAN))
		|| (GetOperand()->IsOfType(TYPE_VOID_ASTERISK) && GetOperand2()->IsOfType(TYPE_VOID_ASTERISK))
		|| (GetOperand()->IsOfType(TYPE_CHAR_ASTERISK) && GetOperand2()->IsOfType(TYPE_CHAR_ASTERISK)));
	  break;
      case EXPR_BITAND:
      case EXPR_BITXOR:
      case EXPR_BITOR:
	  return (nType == TYPE_INTEGER) && GetOperand()->IsOfType(nType)
	      && GetOperand2()->IsOfType(nType);
	  break;
      case EXPR_LOGAND:
      case EXPR_LOGOR:
	  return (nType == TYPE_BOOLEAN) && GetOperand()->IsOfType(nType)
	      && GetOperand2()->IsOfType(nType);
	  break;
      default:
	  break;
      }
    return false;
}

/** returns the second operand
 *	\return the second operand
 */
CFEExpression *CFEBinaryExpression::GetOperand2()
{
    return m_pOperand2;
}

/** creates a copy of this object
 *	\return a reference to the new object
 */
CObject *CFEBinaryExpression::Clone()
{
    return new CFEBinaryExpression(*this);
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEBinaryExpression::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<binary_expression>\n");
	  pFile->IncIndent();
	  GetOperand()->Serialize(pFile);
	  pFile->PrintIndent("<operator>");
	  switch (GetOperator())
	    {
	    case EXPR_MUL:
		pFile->Print("*");
		break;
	    case EXPR_DIV:
		pFile->Print("/");
		break;
	    case EXPR_MOD:
		pFile->Print("%");
		break;
	    case EXPR_PLUS:
		pFile->Print("+");
		break;
	    case EXPR_MINUS:
		pFile->Print("-");
		break;
	    case EXPR_LSHIFT:
		pFile->Print("<<");
		break;
	    case EXPR_RSHIFT:
		pFile->Print(">>");
		break;
	    case EXPR_LT:
		pFile->Print("<");
		break;
	    case EXPR_GT:
		pFile->Print(">");
		break;
	    case EXPR_LTEQU:
		pFile->Print("<=");
		break;
	    case EXPR_GTEQU:
		pFile->Print(">=");
		break;
	    case EXPR_EQUALS:
		pFile->Print("==");
		break;
	    case EXPR_NOTEQUAL:
		pFile->Print("!=");
		break;
	    case EXPR_BITAND:
		pFile->Print("&");
		break;
	    case EXPR_BITXOR:
		pFile->Print("^");
		break;
	    case EXPR_BITOR:
		pFile->Print("|");
		break;
	    case EXPR_LOGAND:
		pFile->Print("&&");
		break;
	    case EXPR_LOGOR:
		pFile->Print("||");
		break;
	    default:
		break;
	    }
	  pFile->Print("</operator>\n");
	  GetOperand2()->Serialize(pFile);
	  pFile->DecIndent();
	  pFile->PrintIndent("</binary_expression>\n");
      }
}
