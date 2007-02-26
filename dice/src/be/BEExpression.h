/**
 *	\file	dice/src/be/BEExpression.h
 *	\brief	contains the declaration of the class CBEExpression
 *
 *	\date	01/17/2002
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
#ifndef __DICE_BEEXPRESSION_H__
#define __DICE_BEEXPRESSION_H__

#include "be/BEObject.h"
#include "Vector.h"

class CFEExpression;
class CFEConditionalExpression;
class CFEBinaryExpression;
class CFEUnaryExpression;
class CFEPrimaryExpression;
class CBEContext;
class CBEFile;

/**	\class CBEExpression
 *	\ingroup backend
 *	\brief the back-end expression
 */
class CBEExpression : public CBEObject  
{
DECLARE_DYNAMIC(CBEExpression);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEExpression();
	virtual ~CBEExpression();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEExpression(CBEExpression &src);

public:
    virtual bool GetBoolValue();
    virtual int GetIntValue();
    virtual bool CreateBackEndBinary(CBEExpression *pOperand1, int nOperator, CBEExpression *pOperand2, CBEContext *pContext);
    virtual bool CreateBackEnd(String sValue, CBEContext *pContext);
    virtual bool CreateBackEnd(int nValue, CBEContext *pContext);
    virtual void Write(CBEFile *pFile, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEExpression *pFEExpression, CBEContext *pContext);
    virtual bool CreateBackEndUnary(int nOperator, CBEExpression *pOperand, CBEContext * pContext);
    virtual bool CreateBackEndPrimary(int nType, CBEExpression *pExpression, CBEContext *pContext);
	virtual bool IsOfType(int nFEType);

protected:
    virtual bool CreateBackEndUnary(CFEUnaryExpression *pFEExpression, CBEContext *pContext);
    virtual bool CreateBackEndPrimary(CFEPrimaryExpression *pFEExpression, CBEContext *pContext);
    virtual bool CreateBackEndBinary(CFEBinaryExpression *pFEExpression, CBEContext *pContext);
    virtual bool CreateBackEndConditional(CFEConditionalExpression *pFEExpression, CBEContext *pContext);
    virtual void WriteUnary(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteBinary(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteConditional(CBEFile *pFile, CBEContext *pContext);
    virtual bool GetBoolValueUnary();
    virtual bool GetBoolValueBinary();
    virtual int GetIntValueUnary();
    virtual int GetIntValueBinary();

protected:
	/**	\var int m_nType
	 *	\brief defines the type of the expression
	 *
	 * The type specifies which values are correct or - if there is no value - an abstract expression, 
	 * such as NULL, TRUE, FALSE
	 */
	int m_nType;
	/**	\var int m_nIntValue
	 *	\brief contains the value of the integer expression
	 */
	int m_nIntValue;
	/**	\var char m_cCharValue
	 *	\brief contains the value of the character expression
	 */
	char m_cCharValue;
	/**	\var double m_fFloatValue
	 *	\brief contains the float value of the expression
	 */
	double m_fFloatValue;
	/**	\var String m_sStringValue
	 *	\brief contains the string value of the expression
	 */
	String m_sStringValue;
	/**	\var CBEExpression m_pOperand1
	 *	\brief contains the first operand
	 */
	CBEExpression *m_pOperand1;
	/**	\var int m_nOperator
	 *	\param the operator for unary or binary expressions
	 */
	int m_nOperator;
	/**	\var CBEExpression *m_pOperand2
	 *	\brief the second operand for binary expressions
	 */
	CBEExpression *m_pOperand2;
	/**	\var CBEExpression *m_pCondition
	 *	\brief the condition for the conditional expression
	 */
	CBEExpression *m_pCondition;
};

#endif // !__DICE_BEEXPRESSION_H__
