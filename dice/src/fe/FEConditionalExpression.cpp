/**
 *    \file    dice/src/fe/FEConditionalExpression.cpp
 *  \brief   contains the implementation of the class CFEConditionalExpression
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "fe/FEConditionalExpression.h"
#include "File.h"

CFEConditionalExpression::CFEConditionalExpression(CFEExpression * pCondition, CFEExpression * pBranchTrue, CFEExpression * pBranchFalse)
:CFEBinaryExpression(EXPR_CONDITIONAL, pBranchTrue, EXPR_NOOPERATOR, pBranchFalse)
{
    m_pCondition = pCondition;
}

CFEConditionalExpression::CFEConditionalExpression(CFEConditionalExpression & src)
:CFEBinaryExpression(src)
{
    if (src.m_pCondition)
      {
      m_pCondition = (CFEExpression *) (src.m_pCondition->Clone());
      m_pCondition->SetParent(this);
      }
    else
    m_pCondition = 0;
}

/** cleans up the conditional expression (frees the condition) */
CFEConditionalExpression::~CFEConditionalExpression()
{
    if (m_pCondition)
    delete m_pCondition;
}

/** returns the integer value of this expression
 *  \return the integer value of the true branch if the condition is true, otherwise the integer value of the false branch
 */
long CFEConditionalExpression::GetIntValue()
{
    return GetCondition()->GetIntValue() ? GetOperand()->GetIntValue() : GetOperand2()->GetIntValue();
}

/** checks the type og the expression
 *  \param nType the type to check for
 *  \return true if this expression evaluates to the requested expression
 */
bool CFEConditionalExpression::IsOfType(unsigned int nType)
{
    return (GetCondition()->IsOfType(TYPE_BOOLEAN)
        || GetCondition()->IsOfType(TYPE_INTEGER)
        || GetCondition()->IsOfType(TYPE_LONG))
        && (GetOperand()->IsOfType(nType)
        && GetOperand2()->IsOfType(nType));
}

/** returns the condition expression
 *  \return the condition expression
 */
CFEExpression *CFEConditionalExpression::GetCondition()
{
    return m_pCondition;
}

/** creates a copy of this object
 *  \return a reference to the new object
 */
CObject *CFEConditionalExpression::Clone()
{
    return new CFEConditionalExpression(*this);
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
string CFEConditionalExpression::ToString()
{
    string ret;
    if (GetCondition())
        ret += GetCondition()->ToString();
    else
        ret += "(no condition)";
    ret += "?";
    if (GetOperand())
        ret += GetOperand()->ToString();
    else
        ret += "(no 1st operand)";
    ret += ":";
    if (GetOperand2())
        ret += GetOperand2()->ToString();
    else
        ret += "(no 2nd operand)";
    return ret;
}

