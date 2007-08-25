/**
 *    \file    dice/src/fe/FEConstDeclarator.h
 *  \brief   contains the declaration of the class CFEConstDeclarator
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
#ifndef __DICE_FE_FECONSTDECLARATOR_H__
#define __DICE_FE_FECONSTDECLARATOR_H__

#include "fe/FEInterfaceComponent.h"
#include <string>

class CFEExpression;
class CFETypeSpec;

/**    \class CFEConstDeclarator
 *    \ingroup frontend
 *  \brief represent the declaration of a constant value
 */
class CFEConstDeclarator : public CFEInterfaceComponent
{

// standard constructor/destructor
public:
    /** \brief constructs a constant declarator object
     *  \param pConstType the type of the constant
     *  \param sConstName the name of the constant
     *  \param pConstValue the value of the constant (is an expression)
     */
    CFEConstDeclarator(CFETypeSpec *pConstType,
            std::string sConstName,
            CFEExpression *pConstValue);
    virtual ~CFEConstDeclarator();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEConstDeclarator(CFEConstDeclarator &src);

// Operations
public:
    virtual CObject* Clone();
    virtual CFEExpression* GetValue();
    virtual std::string GetName();
    virtual CFETypeSpec* GetType();
    bool Match(std::string sName);

    virtual void Accept(CVisitor&);

// attributes
protected:
    /**    \var CFETypeSpec *m_pConstType
     *  \brief the type of the constant value
     */
    CFETypeSpec *m_pConstType;
    /**    \var std::string m_sConstName
     *  \brief the alias name of the const expression
     */
    std::string m_sConstName;
    /**    \var CFEExpression *m_pConstValue
     *  \brief the constant expression
     */
    CFEExpression *m_pConstValue;
};

#endif /* __DICE_FE_FECONSTDECLARATOR_H__ */

