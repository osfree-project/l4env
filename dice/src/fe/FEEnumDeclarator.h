/**
 *    \file    dice/src/fe/FEEnumDeclarator.h
 *    \brief   contains the declaration of the class CFEEnumDeclarator
 *
 *    \date    06/08/2001
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
#ifndef __DICE_FE_FEENUMDECLARATOR_H__
#define __DICE_FE_FEENUMDECLARATOR_H__

#include "fe/FEDeclarator.h"

class CFEExpression;

/** \class CFEEnumDeclarator
 *    \ingroup frontend
 *    \brief describes an enumeration's declarator
 *
 * This class is used to describe a enumeration's declarator. A declarator can be, for instance,
 * a variable declaration, or a parameter. An enumeration's declarator can be assigned a value.
 */
class CFEEnumDeclarator : public CFEDeclarator
{
protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEEnumDeclarator(CFEEnumDeclarator &src);
public:
    /**    \brief default constructor */
    CFEEnumDeclarator();
    /** \brief constructs a declarator object
     *  \param sName the name of this declarator
     *  \param pInitialValue the initial value of the enum member
     */
    CFEEnumDeclarator(string sName, CFEExpression *pInitialValue = 0);
    virtual ~CFEEnumDeclarator();

public:
    virtual void Serialize(CFile *pFile);
    virtual CObject* Clone();
    virtual CFEExpression* GetInitialValue();

protected:
    /**    \var CFEExpression* m_pInitialValue
     *    \brief a reference to the initial value
     */
    CFEExpression* m_pInitialValue;
};

#endif // __DICE_FE_FEENUMDECLARATOR_H__
