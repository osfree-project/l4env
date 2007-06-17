/**
 *  \file   dice/src/fe/FEUnaryExpression.cpp
 *  \brief  contains the implementation of the class CFEUnaryExpression
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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

#include "fe/FEUnaryExpression.h"

CFEUnaryExpression::CFEUnaryExpression(EXPR_TYPE nType, EXPT_OPERATOR Operator, CFEExpression * pOperand)
: CFEPrimaryExpression(nType, pOperand)
{
    m_nOperator = Operator;
}

CFEUnaryExpression::CFEUnaryExpression(CFEUnaryExpression & src)
: CFEPrimaryExpression(src)
{
    m_nOperator = src.m_nOperator;
}

/** cleans up the unary expression object */
CFEUnaryExpression::~CFEUnaryExpression()
{
    // nothing to clean up
}

/** retrieves the integer value of the expression
 *  \return the integer value of the expression
 */
long CFEUnaryExpression::GetIntValue()
{
    switch (GetOperator())
      {
      case EXPR_NOOPERATOR:
      return GetOperand()->GetIntValue();
      break;
      case EXPR_SPLUS:
      return 0 + (GetOperand()->GetIntValue());
      break;
      case EXPR_SMINUS:
      return 0 - (GetOperand()->GetIntValue());
      break;
      case EXPR_TILDE:
      return ~(GetOperand()->GetIntValue());
      break;
      case EXPR_EXCLAM:
      return !(GetOperand()->GetIntValue());
      break;
      default:
      return GetOperand()->GetIntValue();
      break;
      }
    return 0;
}

/** checks the type of itself
 *  \param nType the type to compare with
 *  \return true if it is of the given type, false f not
 */
bool CFEUnaryExpression::IsOfType(unsigned int nType)
{
    switch (GetOperator())
      {
      case EXPR_SPLUS:
      case EXPR_SMINUS:
      return ((nType == TYPE_INTEGER) || (nType == TYPE_LONG)) && GetOperand()->IsOfType(nType);
      break;
      case EXPR_TILDE:
      case EXPR_EXCLAM:
      return (nType == TYPE_INTEGER || nType == TYPE_BOOLEAN || nType == TYPE_LONG)
          && GetOperand()->IsOfType(nType);
      break;
      default:
      break;
      }
    return GetOperand()->IsOfType(nType);
}

/** retrieves the operator of this expression
 *  \return the operator of this expression
 */
EXPT_OPERATOR CFEUnaryExpression::GetOperator()
{
    return m_nOperator;
}

/** creates a copy of this object
 *  \return a reference to the new object
 */
CObject *CFEUnaryExpression::Clone()
{
    return new CFEUnaryExpression(*this);
}
