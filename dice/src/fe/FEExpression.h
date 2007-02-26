/**
 *	\file	dice/src/fe/FEExpression.h
 *	\brief	contains the declaration of the class CFEExpression
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEEXPRESSION_H__
#define __DICE_FE_FEEXPRESSION_H__

enum EXPR_TYPE {
  EXPR_NONE			= 0,
  EXPR_NULL,                // Expression
  EXPR_TRUE,
  EXPR_FALSE,
  EXPR_CHAR,
  EXPR_STRING,
  EXPR_USER_DEFINED,
  EXPR_INT,                 // primary
  EXPR_FLOAT,
  EXPR_PAREN,
  EXPR_UNARY,               // unary
  EXPR_BINARY,              // binary
  EXPR_CONDITIONAL,         // conditional
  EXPR_SIZEOF               // sizeof
};

#include "fe/FEBase.h"
#include "TypeSpec-Type.h"
#include "CString.h"

/** \class CFEExpression
 *	\ingroup frontend
 *	\brief represents a simple expression
 *
 * This class is used to represent a simple expression.
 */
class CFEExpression : public CFEBase
{
DECLARE_DYNAMIC(CFEExpression);

// standard constructor/destructor
public:
	/** standard constructor for expression */
	CFEExpression();
	/** construct an expression object 
	 *	\param nType the type of the expression (NULL, TRUE, FALSE, derived expressions) */
    CFEExpression(EXPR_TYPE nType); // NULL, TRUE, FALSE, derived
	/** construct expression object
	 *	\param nType the type of the expression (CHAR)
	 *	\param nChar the single character
	 */
	CFEExpression(EXPR_TYPE nType, char nChar); // single char
	/** constructs an expression
	 *	\param nType the type of the expression (string)
	 *	\param sString the string
	 */
	CFEExpression(EXPR_TYPE nType, String sString); // string
    virtual ~CFEExpression();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEExpression(CFEExpression &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual String ToString();
	virtual CObject* Clone();
	virtual bool IsOfType(TYPESPEC_TYPE nType);
	virtual long GetIntValue();
	virtual EXPR_TYPE GetType();
	virtual char GetChar();
	virtual String GetString();

// attributes
protected:
	/** \var EXPR_TYPE m_nType
	 *	\brief the type of the expression
	 */
    EXPR_TYPE m_nType;
	/**	\var char m_Char
	 *	\brief if this is a character expression: the character
	 */
	char m_Char;
	/**	\var String m_String
	 *	\brief if this is a string expression: the string
	 */
	String m_String;
};

#endif /* __DICE_FE_FEEXPRESSION_H__ */

