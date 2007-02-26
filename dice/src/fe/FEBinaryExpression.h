/**
 *	\file	dice/src/fe/FEBinaryExpression.h 
 *	\brief	contains the declaration of the class CFEBinaryExpression
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

/** preprocessing symbol to check header file */
#ifndef __ILD4_FE_FEBINARYEXPRESSION_H__
#define __ILD4_FE_FEBINARYEXPRESSION_H__

#include "fe/FEUnaryExpression.h"

/** \class CFEBinaryExpression
 *	\ingroup frontend
 *	\brief represents a binary expression
 *
 * This class is used to represent a binary expression, which is an expression
 * consiting of two expressions seperated by an operator.
 */
class CFEBinaryExpression : public CFEUnaryExpression
{
DECLARE_DYNAMIC(CFEBinaryExpression);

// standard constructor/destructor
public:
	/** constructs a binary expression 
	 *	\param nType the type of the expression
	 *	\param pOperand the first operand of the expression
	 *	\param Operator the operator of the expression
	 *	\param pOperand2 the second operand
	 */
	CFEBinaryExpression(EXPR_TYPE nType, CFEExpression *pOperand, EXPT_OPERATOR Operator, CFEExpression *pOperand2);
	virtual ~CFEBinaryExpression();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEBinaryExpression(CFEBinaryExpression &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual CObject* Clone();
	virtual bool IsOfType(TYPESPEC_TYPE nType);
	virtual long GetIntValue();
	virtual CFEExpression* GetOperand2();

// attributes
protected:
	/** \var CFEExpression *m_pOperand2
	 *	\brief the second operand
	 */
	CFEExpression *m_pOperand2;
};

#endif /* __ILD4_FE_FEBINARYEXPRESSION_H__ */

