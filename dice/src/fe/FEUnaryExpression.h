/**
 *	\file	dice/src/fe/FEUnaryExpression.h 
 *	\brief	contains the declaration of the class CFEUnaryExpression
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
#ifndef __DICE_FE_FEUNARYEXPRESSION_H__
#define __DICE_FE_FEUNARYEXPRESSION_H__

enum EXPT_OPERATOR {
	EXPR_NOOPERATOR	= 0,
	EXPR_SPLUS		= 1,	// sign: plus - unary
	EXPR_SMINUS		= 2,	// sign: minus
	EXPR_TILDE		= 3,	// ~
	EXPR_EXCLAM		= 4,	// !
	EXPR_MUL		= 5,	// * - binary
	EXPR_DIV		= 6,	// /
	EXPR_MOD		= 7,	// %
	EXPR_PLUS		= 8,	// +
	EXPR_MINUS		= 9,	// -
	EXPR_LSHIFT		= 10,	// <<
	EXPR_RSHIFT		= 11,	// >>
	EXPR_LT			= 12,	// <
	EXPR_GT			= 13,	// >
	EXPR_LTEQU		= 14,	// <=
	EXPR_GTEQU		= 15,	// >=
	EXPR_EQUALS		= 16,	// ==
	EXPR_NOTEQUAL	= 17,	// !=
	EXPR_BITAND		= 18,	// &
	EXPR_BITXOR		= 19,	// ^
	EXPR_BITOR		= 20,	// |
	EXPR_LOGAND		= 21,	// &&
	EXPR_LOGOR		= 22,	// ||
	EXPR_INCR       = 23,   // ++
	EXPR_DECR       = 24    // --
};

#include "fe/FEPrimaryExpression.h"

/** \class CFEUnaryExpression
 *	\ingroup frontend
 *	\brief contains a unary expression
 *
 * A unary expression is a primary expression extended by an operator,
 * which is placed in front of the expression
 */
class CFEUnaryExpression : public CFEPrimaryExpression
{
DECLARE_DYNAMIC(CFEUnaryExpression);

// standard constructor/destructor
public:
	/** constructs a unary expression object
	 *	\param nType the type of the expression
	 *	\param Operator the operato of the expression
	 *	\param pOperand the first operand of the expression
	 */
	CFEUnaryExpression(EXPR_TYPE nType, EXPT_OPERATOR Operator, CFEExpression *pOperand);
	virtual ~CFEUnaryExpression();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEUnaryExpression(CFEUnaryExpression &src);

// Operators
public:
	virtual void Serialize(CFile *pFile);
	virtual CObject* Clone();
	virtual bool IsOfType(TYPESPEC_TYPE nType);
	virtual long GetIntValue();
	virtual EXPT_OPERATOR GetOperator();

// attributes
protected:
	/**	\var EXPT_OPERATOR m_nOperator
	 *	\brief the operator
	 */
    EXPT_OPERATOR m_nOperator;
};

#endif /* __DICE_FE_FEUNARYEXPRESSION_H__ */

