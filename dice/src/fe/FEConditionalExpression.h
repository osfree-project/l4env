/**
 *  \file    dice/src/fe/FEConditionalExpression.h
 *  \brief   contains the declaration of the class CFEConditionalExpression
 *
 *  \date    01/31/2001
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
#ifndef __DICE_FE_FECONDITIONALEXPRESSION_H__
#define __DICE_FE_FECONDITIONALEXPRESSION_H__

#include "fe/FEBinaryExpression.h"

/** \class CFEConditionalExpression
 *  \ingroup frontend
 *  \brief represents a conditional expression
 *
 * This class is used to represent a conditional expression. This
 * kind of expression consits of an expression, which evaluates to a
 * boolean value. Is this boolen value true, contains the second expression
 * the value, which stands for the whole expression. Is the boolean expression
 * false, is the value of the whole expression the value of the third expression.
 *
 * \todo optimize: if condition is constant, we can already decide which expression
 * to use
 */
class CFEConditionalExpression : public CFEBinaryExpression
{

// standard constructor/destructor
public:
    /** constructs a conditional expression
     *  \param pCondition the expression, which is the condition
     *  \param pBranchTrue if the condition is true this expression is valid
     *  \param pBranchFalse if the condition is false thsi expression is valid
     */
    CFEConditionalExpression(CFEExpression *pCondition,
        CFEExpression *pBranchTrue,
        CFEExpression *pBranchFalse);
    virtual ~CFEConditionalExpression();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEConditionalExpression(CFEConditionalExpression* src);

// Operations
public:
	virtual CObject* Clone();
    virtual std::string ToString();
    virtual bool IsOfType(unsigned int nType);
    virtual int GetIntValue();
    virtual CFEExpression* GetCondition();

// attributes
protected:
    /**    \var CFEExpression *m_pCondition
     *  \brief the first, boolean expression
     */
    CFEExpression *m_pCondition;
};

#endif /* __DICE_FE_FECONDITIONALEXPRESSION_H__ */

