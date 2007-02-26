/**
 *    \file    dice/src/fe/FEPrimaryExpression.cpp
 *    \brief   contains the implementation of the class CFEPrimaryExpression
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

#include "fe/FEPrimaryExpression.h"
#include "File.h"

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, long int nValue)
:CFEExpression(nType)
{
    m_nValue = nValue;
    m_nuValue = 0UL;
    m_nlValue = 0LL;
    m_nulValue = 0ULL;
    m_fValue = 0.0f;
    m_pOperand = 0;
}

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, unsigned long int nValue)
:CFEExpression(nType)
{
    m_nuValue = nValue;
    m_nValue = 0L;
    m_nlValue = 0LL;
    m_nulValue = 0ULL;
    m_fValue = 0.0f;
    m_pOperand = 0;
}

/** only provide 'long long' constructors if 'long long' is supported */
#if SIZEOF_LONG_LONG > 0

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, long long nValue)
:CFEExpression(nType)
{
    m_nValue = 0L;
    m_nuValue = 0UL;
    m_nlValue = nValue;
    m_nulValue = 0ULL;
    m_fValue = 0.0f;
    m_pOperand = 0;
}

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, unsigned long long nValue)
:CFEExpression(nType)
{
    m_nValue = 0L;
    m_nuValue = 0UL;
    m_nlValue = 0LL;
    m_nulValue = nValue;
    m_fValue = 0.0f;
    m_pOperand = 0;
}

#endif

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, long double fValue)
:CFEExpression(nType)
{
    m_nValue = 0L;
    m_nuValue = 0UL;
    m_nlValue = 0LL;
    m_nulValue = 0ULL;
    m_fValue = fValue;
    m_pOperand = 0;
}

CFEPrimaryExpression::CFEPrimaryExpression(EXPR_TYPE nType, CFEExpression * pOperand)
:CFEExpression(nType)
{
    m_nValue = 0L;
    m_nuValue = 0UL;
    m_nlValue = 0LL;
    m_nulValue = 0ULL;
    m_fValue = 0.0f;
    m_pOperand = pOperand;
}

CFEPrimaryExpression::CFEPrimaryExpression(CFEPrimaryExpression & src)
:CFEExpression(src)
{
    m_fValue = src.m_fValue;
    m_nValue = src.m_nValue;
    m_nuValue = src.m_nuValue;
    m_nlValue = src.m_nlValue;
    m_nulValue = src.m_nulValue;
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
 *    \return the integer value of the expression
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
    case EXPR_UINT:
        return (long)m_nuValue;
        break;
#if SIZEOF_LONG_LONG > 0
    case EXPR_LLONG:
        return (long)m_nlValue;
        break;
    case EXPR_ULLONG:
        return (long)m_nulValue;
        break;
#endif
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
    case EXPR_UINT:
#if SIZEOF_LONG_LONG > 0
    case EXPR_LLONG:
    case EXPR_ULLONG:
#endif
        return (nType == TYPE_INTEGER)
            || (nType == TYPE_LONG);
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
 *    \return the float value of this expression
 */
long double CFEPrimaryExpression::GetFloatValue()
{
    return m_fValue;
}

/** returns the operand of the expression
 *    \return the operand of the expression
 */
CFEExpression *CFEPrimaryExpression::GetOperand()
{
    return m_pOperand;
}

/** create a copy of this object
 *    \return a reference to the new object
 */
CObject *CFEPrimaryExpression::Clone()
{
    return new CFEPrimaryExpression(*this);
}

/** serialize this object
 *    \param pFile the file to serialize from/to
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
        case EXPR_UINT:
            pFile->PrintIndent("<expression>%d</expression>\n", m_nuValue);
            break;
#if SIZEOF_LONG_LONG > 0
        case EXPR_LLONG:
            pFile->PrintIndent("<expression>%d</expression>\n", m_nlValue);
            break;
        case EXPR_ULLONG:
            pFile->PrintIndent("<expression>%d</expression>\n", m_nulValue);
            break;
#endif
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

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
string CFEPrimaryExpression::ToString()
{
    string ret;
    char sNumber[12];
    switch (m_nType)
    {
    case EXPR_INT:
        snprintf(sNumber, 12, "%ld", m_nValue);
        ret += sNumber;
        break;
    case EXPR_UINT:
        snprintf(sNumber, 12, "%ld", m_nuValue);
        ret += sNumber;
        break;
#if SIZEOF_LONG_LONG > 0
    case EXPR_LLONG:
        snprintf(sNumber, 12, "%lld", m_nlValue);
        ret += sNumber;
        break;
    case EXPR_ULLONG:
        snprintf(sNumber, 12, "%lld", m_nulValue);
        ret += sNumber;
        break;
#endif
    case EXPR_FLOAT:
        snprintf(sNumber, 12, "%Lf", m_fValue);
        ret += sNumber;
        break;
    case EXPR_PAREN:
        ret += "(";
        if (GetOperand())
            ret += GetOperand()->ToString();
        else
            ret += "(no operand)";
        ret += ")";
        break;
    default:
        break;
    }
    return ret;
}

