/**
 *    \file    dice/src/fe/FEPrimaryExpression.h
 *    \brief   contains the declaration of the class CFEPrimaryExpression
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
#ifndef __DICE_FE_FEPRIMARYEXPRESSION_H__
#define __DICE_FE_FEPRIMARYEXPRESSION_H__

#include "fe/FEExpression.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

/**    \class CFEPrimaryExpression
 *    \ingroup frontend
 *    \brief represents a primary expression
 *
 * A primary expression is a simple expression, extended by and integer
 * or float value.
 */
class CFEPrimaryExpression : public CFEExpression
{

// standard constructor/destructor
public:
    /** construct a primary expression
     *    \param nType the type of the expression
     *    \param nValue the integer value of an integer expression
     */
    CFEPrimaryExpression(EXPR_TYPE nType, long int nValue); // simple integer value
    /** construct a primary expression
     *    \param nType the type of the expression
     *    \param nValue the integer value of an integer expression
     */
    CFEPrimaryExpression(EXPR_TYPE nType, unsigned long int nValue); // simple integer value
#if SIZEOF_LONG_LONG > 0
    /** construct a primary expression
     *    \param nType the type of the expression
     *    \param nValue the integer value of an integer expression
     */
    CFEPrimaryExpression(EXPR_TYPE nType, long long nValue); // simple integer value
    /** construct a primary expression
     *    \param nType the type of the expression
     *    \param nValue the integer value of an integer expression
     */
    CFEPrimaryExpression(EXPR_TYPE nType, unsigned long long nValue); // simple integer value
#endif
    /** constructs a primary expression
     *    \param nType the type of the expression
     *    \param fValue the floating point type if this is a floating point expression
     */
    CFEPrimaryExpression(EXPR_TYPE nType, long double fValue); // simple float value
    /** construct a primary expression
     *    \param nType the type of the expression
     *    \param pOperand the operand if this is a operand in parenthesis
     */
    CFEPrimaryExpression(EXPR_TYPE nType, CFEExpression *pOperand); // means '('exp')'
    virtual ~CFEPrimaryExpression();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CFEPrimaryExpression(CFEPrimaryExpression &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual string ToString();
    virtual CObject* Clone();
    virtual long double GetFloatValue();
    virtual bool IsOfType(TYPESPEC_TYPE nType);
    virtual long GetIntValue();
    virtual CFEExpression* GetOperand();

// attributes
protected:
    /**    \var CFEExpression *m_pOperand
     *    \brief the contained expression " '(' expr ')' "
     */
    CFEExpression *m_pOperand;
    /**    \var long int m_nValue
     *    \brief the integer value
     */
    long int m_nValue;
    /**    \var unsigned long int m_nuValue
     *    \brief the integer value
     */
    unsigned long int m_nuValue;
#if SIZEOF_LONG_LONG > 0
    /**    \var long long m_nlValue
     *    \brief the integer value
     */
    long long m_nlValue;
    /**    \var unsigned long long m_nulValue
     *    \brief the integer value
     */
    unsigned long long int m_nulValue;
#endif
    /**    \var long double m_fValue
     *    \brief the float value
     */
    long double m_fValue;
};

#endif /* __DICE_FE_FEPRIMARYEXPRESSION_H__ */

