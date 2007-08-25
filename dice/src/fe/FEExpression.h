/**
 *    \file    dice/src/fe/FEExpression.h
 *  \brief   contains the declaration of the class CFEExpression
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEEXPRESSION_H__
#define __DICE_FE_FEEXPRESSION_H__

enum EXPR_TYPE {
  EXPR_NONE            = 0,
  EXPR_NULL,                // Expression
  EXPR_TRUE,
  EXPR_FALSE,
  EXPR_CHAR,
  EXPR_WCHAR,
  EXPR_STRING,
  EXPR_USER_DEFINED,
  EXPR_INT,                 // primary
  EXPR_UINT,
  EXPR_LLONG,
  EXPR_ULLONG,
  EXPR_FLOAT,
  EXPR_PAREN,
  EXPR_UNARY,               // unary
  EXPR_BINARY,              // binary
  EXPR_CONDITIONAL,         // conditional
  EXPR_SIZEOF               // sizeof
};

#include "fe/FEBase.h"
#include "TypeSpec-Type.h"
#include <string>

/** \class CFEExpression
 *    \ingroup frontend
 *  \brief represents a simple expression
 *
 * This class is used to represent a simple expression.
 */
class CFEExpression : public CFEBase
{

// standard constructor/destructor
public:
    /** standard constructor for expression */
    CFEExpression();
    /** construct an expression object
     *  \param nType the type of the expression (NULL, TRUE, FALSE, derived expressions) */
    CFEExpression(EXPR_TYPE nType); // NULL, TRUE, FALSE, derived
    /** construct expression object
     *  \param nChar the single character
     */
    CFEExpression(signed char nChar); // single char
    /** construct expression object
     *  \param nWChar the single wide character
     */
    CFEExpression(short nWChar); // single char
    /** constructs an expression
     *  \param sString the string
     */
    CFEExpression(std::string sString); // string
    virtual ~CFEExpression();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEExpression(CFEExpression &src);

// Operations
public:
    virtual std::string ToString();
    virtual CObject* Clone();
    virtual bool IsOfType(unsigned int nType);
    virtual int GetIntValue();
    virtual EXPR_TYPE GetType();
    virtual char GetChar();
    virtual short GetWChar();
    virtual std::string GetString();

// attributes
protected:
    /** \var EXPR_TYPE m_nType
     *  \brief the type of the expression
     */
    EXPR_TYPE m_nType;
    /** \var char m_Char
     *  \brief if this is a character expression: the character
     */
    char m_Char;
    /** \var short m_WChar
     *  \brief if this is a character expression: the wide character
     */
    short m_WChar;
    /** \var std::string m_String
     *  \brief if this is a string expression: the string
     */
    std::string m_String;
};

#endif /* __DICE_FE_FEEXPRESSION_H__ */

