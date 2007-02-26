/**
 *    \file    dice/src/fe/FEUserDefinedExpression.h
 *    \brief   contains the declaration of the class CFEUserDefinedExpression
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
#ifndef __DICE_FE_FEUSERDEFINEDEXPRESSION_H__
#define __DICE_FE_FEUSERDEFINEDEXPRESSION_H__

#include "fe/FEExpression.h"

/**    \class CFEUserDefinedExpression
 *    \ingroup frontend
 *    \brief represents an expression, which is user defined
 *
 * An expression is user defined if the name of another expression (const value)
 * is used.
 */
class CFEUserDefinedExpression : public CFEExpression
{

// standard constructor/destructor
public:
    /** constructs a user defined expression
     *    \param sExpName the name of the user defined expression */
    CFEUserDefinedExpression(string sExpName);
    virtual ~CFEUserDefinedExpression();

protected:
    /**    \brief copy constrcutor
     *    \param src the source to copy from
     */
    CFEUserDefinedExpression(CFEUserDefinedExpression &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual CObject* Clone();
    virtual bool IsOfType(TYPESPEC_TYPE nType);
    virtual long GetIntValue();
    virtual string GetExpName();

// attributes
protected:
    /**    \var string m_sExpName
     *    \brief the name of the expression
     */
    string m_sExpName;
};

#endif /* __DICE_FE_FEUSERDEFINEDEXPRESSION_H__ */

