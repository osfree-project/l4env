/**
 *  \file    dice/src/be/BEExpression.h
 *  \brief   contains the declaration of the class CBEExpression
 *
 *  \date    01/17/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_BEEXPRESSION_H__
#define __DICE_BEEXPRESSION_H__

#include "be/BEObject.h"

class CFEExpression;
class CFEConditionalExpression;
class CFEBinaryExpression;
class CFEUnaryExpression;
class CFEPrimaryExpression;
class CFESizeOfExpression;
class CBEFile;
class CBEType;

/**    \class CBEExpression
 *    \ingroup backend
 *  \brief the back-end expression
 */
class CBEExpression : public CBEObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBEExpression();
    virtual ~CBEExpression();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEExpression(CBEExpression* src);

public:
    virtual bool GetBoolValue();
    virtual int GetIntValue();
    virtual std::string GetStringValue();
    virtual bool IsOfType(int nFEType);
	virtual CBEExpression* Clone();
    virtual void Write(CBEFile& pFile);
    virtual void WriteToStr(std::string &sStr);

    virtual void CreateBackEndBinary(CBEExpression *pOperand1, int nOperator,
	CBEExpression *pOperand2);
    virtual void CreateBackEnd(std::string sValue);
    virtual void CreateBackEnd(int nValue);
    virtual void CreateBackEnd(CFEExpression *pFEExpression);
    virtual void CreateBackEndUnary(int nOperator, CBEExpression *pOperand);
    virtual void CreateBackEndPrimary(int nType, CBEExpression *pExpression);

protected:
    virtual void CreateBackEndUnary(CFEUnaryExpression *pFEExpression);
    virtual void CreateBackEndPrimary(CFEPrimaryExpression *pFEExpression);
    virtual void CreateBackEndBinary(CFEBinaryExpression *pFEExpression);
    virtual void CreateBackEndConditional(CFEConditionalExpression *pFEExpression);
    virtual void CreateBackEndSizeOf(CFESizeOfExpression *pFEExpression);

    virtual void WriteUnaryToStr(std::string &sStr);
    virtual void WriteBinaryToStr(std::string &sStr);
    virtual void WriteConditionalToStr(std::string &sStr);
    virtual bool GetBoolValueUnary();
    virtual bool GetBoolValueBinary();
    virtual int GetIntValueUnary();
    virtual int GetIntValueBinary();
    virtual std::string GetStringValueUnary();
    virtual std::string GetStringValueBinary();

protected:
    /**    \var int m_nType
     *  \brief defines the type of the expression
     *
     * The type specifies which values are correct or - if there is no value -
     * an abstract expression, such as NULL, TRUE, FALSE
     */
    int m_nType;
    /**    \var int m_nIntValue
     *  \brief contains the value of the integer expression
     */
    int m_nIntValue;
    /**    \var char m_cCharValue
     *  \brief contains the value of the character expression
     */
    char m_cCharValue;
    /**    \var long double m_fFloatValue
     *  \brief contains the float value of the expression
     */
    long double m_fFloatValue;
    /**    \var std::string m_sStringValue
     *  \brief contains the std::string value of the expression
     */
    std::string m_sStringValue;
    /**    \var CBEExpression m_pOperand1
     *  \brief contains the first operand
     */
    CBEExpression *m_pOperand1;
    /**    \var int m_nOperator
     *  \param the operator for unary or binary expressions
     */
    int m_nOperator;
    /**    \var CBEExpression *m_pOperand2
     *  \brief the second operand for binary expressions
     */
    CBEExpression *m_pOperand2;
    /**    \var CBEExpression *m_pCondition
     *  \brief the condition for the conditional expression
     */
    CBEExpression *m_pCondition;
    /** \var CBEType *m_pType
     *  \brief the type of a size-of expression
     */
    CBEType *m_pType;
};

#endif // !__DICE_BEEXPRESSION_H__
