/**
 *  \file   dice/src/fe/FEUnaryExpression.h
 *  \brief  contains the declaration of the class CFEUnaryExpression
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_FE_FEUNARYEXPRESSION_H__
#define __DICE_FE_FEUNARYEXPRESSION_H__

enum EXPT_OPERATOR {
    EXPR_NOOPERATOR = 0,
    EXPR_SPLUS,             /**< sign: plus - unary */
    EXPR_SMINUS,            /**< sign: minus */
    EXPR_TILDE,             /**< ~ */
    EXPR_EXCLAM,            /**< ! */
    EXPR_MUL,               /**< * - binary */
    EXPR_DIV,               /**< / */
    EXPR_MOD,               /**< % */
    EXPR_PLUS,              /**< + */
    EXPR_MINUS,             /**< - */
    EXPR_LSHIFT,            /**< << (10) */
    EXPR_RSHIFT,            /**< >> */
    EXPR_LT,                /**< < */
    EXPR_GT,                /**< > */
    EXPR_LTEQU,             /**< <= */
    EXPR_GTEQU,             /**< >= */
    EXPR_EQUALS,            /**< == */
    EXPR_NOTEQUAL,          /**< != */
    EXPR_BITAND,            /**< & */
    EXPR_BITXOR,            /**< ^ */
    EXPR_BITOR,             /**< | */
    EXPR_LOGAND,            /**< && */
    EXPR_LOGOR,             /**< || */
    EXPR_ASSIGN,            /**< = */
    EXPR_MUL_ASSIGN,        /**< *= */
    EXPR_DIV_ASSIGN,        /**< /= */
    EXPR_MOD_ASSIGN,        /**< %= */
    EXPR_PLUS_ASSIGN,       /**< += */
    EXPR_MINUS_ASSIGN,      /**< -= */
    EXPR_LSHIFT_ASSIGN,     /**< <<= */
    EXPR_RSHIFT_ASSIGN,     /**< >>= */
    EXPR_AND_ASSIGN,        /**< &= */
    EXPR_OR_ASSIGN,         /**< |= */
    EXPR_XOR_ASSIGN,        /**< ^= */
    EXPR_INCR,              /**< ++ */
    EXPR_DECR,              /**< -- */
    EXPR_MIN,               /**< <? */
    EXPR_MAX,               /**< >? */
    EXPR_MIN_ASSIGN,        /**< <?= */
    EXPR_MAX_ASSIGN,        /**< >?= */
    EXPR_CAST,              /**< (type)expr */
    EXPR_DELETE,            /**< delete expr */
    EXPR_DELETE_ARRAY       /**< delete[] expr */
};

#include "fe/FEPrimaryExpression.h"

class CFETypeSpec;

/** \class CFEUnaryExpression
 *  \ingroup frontend
 *  \brief contains a unary expression
 *
 * A unary expression is a primary expression extended by an operator,
 * which is placed in front of the expression
 */
class CFEUnaryExpression : public CFEPrimaryExpression
{

// standard constructor/destructor
public:
    /** constructs a unary expression object
     *  \param nType the type of the expression
     *  \param Operator the operato of the expression
     *  \param pOperand the first operand of the expression
     */
    CFEUnaryExpression(EXPR_TYPE nType, EXPT_OPERATOR Operator, CFEExpression *pOperand);
    /** constructs a unary cast expression object
     *  \param pCastType the cast type of the expression
     *  \param pOperand the operand of the cast expression
     */
    CFEUnaryExpression(CFETypeSpec* pCastType, CFEExpression *pOperand);
    virtual ~CFEUnaryExpression();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEUnaryExpression(CFEUnaryExpression &src);

// Operators
public:
    virtual CObject* Clone();
    virtual bool IsOfType(unsigned int nType);
    virtual int GetIntValue();
    virtual EXPT_OPERATOR GetOperator();
    virtual CFETypeSpec* GetCastType();

// attributes
protected:
    /** \var EXPT_OPERATOR m_nOperator
     *  \brief the operator
     */
    EXPT_OPERATOR m_nOperator;
    /** \var CFETypeSpec* m_pCastType
     *  \brief contains the type of the cast operation
     */
    CFETypeSpec* m_pCastType;
};

#endif /* __DICE_FE_FEUNARYEXPRESSION_H__ */

