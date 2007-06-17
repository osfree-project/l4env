/**
 *    \file    dice/src/fe/FEExpression.cpp
 *  \brief   contains the implementation of the class CFEExpression
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

#include "fe/FEExpression.h"
#include <string>

CFEExpression::CFEExpression()
{
    m_nType = EXPR_NONE;
}

CFEExpression::CFEExpression(EXPR_TYPE nType)
{
    m_nType = nType;
    m_Char = 0x00;
}

CFEExpression::CFEExpression(EXPR_TYPE nType, char nChar)
{
    m_nType = nType;
    m_Char = nChar;
}

CFEExpression::CFEExpression(EXPR_TYPE nType, string sString)
{
    m_nType = nType;
    m_Char = 0x00;
    m_String = sString;
}

CFEExpression::CFEExpression(CFEExpression & src):CFEBase(src)
{
    m_Char = src.m_Char;
    m_nType = src.m_nType;
    m_String = src.m_String;
}

/** cleans up the expression object (frees the string) */
CFEExpression::~CFEExpression()
{

}

/** returns a reference to the string
 *  \return a reference to the string
 */
string CFEExpression::GetString()
{
    return m_String;
}

/** returns the integer value
 *  \return the integer value (boolean values are casted into integer, string returns 0)
 */
long CFEExpression::GetIntValue()
{
    switch (GetType())
    {
    case EXPR_NULL:
        return 0;
        break;
    case EXPR_TRUE:
        return (int) true;
        break;
    case EXPR_FALSE:
        return (int) false;
        break;
    case EXPR_CHAR:
        return (int) m_Char;
        break;
    case EXPR_STRING:
        return 0;
        break;
    default:
        break;
    }
    return 0;
}

/** checks the type of the expression
 *  \param nType the type to check
 *  \return true if this expression is of the requested type.
 */
bool CFEExpression::IsOfType(unsigned int nType)
{
    switch (GetType())
    {
    case EXPR_NULL:
        return ((nType == TYPE_VOID_ASTERISK) || (nType == TYPE_CHAR_ASTERISK));
        break;
    case EXPR_TRUE:
    case EXPR_FALSE:
        return ((nType == TYPE_INTEGER)
            || (nType == TYPE_LONG)
            || (nType == TYPE_BOOLEAN));
        break;
    case EXPR_CHAR:
        return ((nType == TYPE_INTEGER)
            || (nType == TYPE_LONG)
            || (nType == TYPE_CHAR));
        break;
    case EXPR_STRING:
        return (nType == TYPE_CHAR_ASTERISK);
        break;
    default:
        break;
    }
    return false;
}

/** returns the type of the expression
 *  \return the type of the expression
 */
EXPR_TYPE CFEExpression::GetType()
{
    return m_nType;
}

/** retrieves the character of this expression
 *  \return the character of this expression
 */
char CFEExpression::GetChar()
{
    return m_Char;
}

/** create a copy of this object
 *  \return a reference to the new object
 */
CObject *CFEExpression::Clone()
{
    return new CFEExpression(*this);
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
string CFEExpression::ToString()
{
    string ret;
    switch (m_nType)
    {
    case EXPR_STRING:
        ret = m_String;
        break;
    case EXPR_CHAR:
        ret = string(1, m_Char);
        break;
    case EXPR_NULL:
        ret = "null";
        break;
    case EXPR_TRUE:
        ret = "true";
        break;
    case EXPR_FALSE:
        ret = "false";
        break;
    default:
        break;
    }
    return ret;
}

