/**
 *    \file    dice/src/fe/FEArrayDeclarator.h
 *  \brief   contains the declaration of the class CFEArrayDeclarator
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
#ifndef __DICE_FE_FEARRAYDECLARATOR_H__
#define __DICE_FE_FEARRAYDECLARATOR_H__

#include "fe/FEDeclarator.h"
#include <vector>
using std::vector;

class CFEExpression;

/** \class CFEArrayDeclarator
 *    \ingroup frontend
 *  \brief represents an array declarator (such as "int t1[]")
 *
 * This class is created to represent an array declarator, which is a simple
 * declarator with array dimensions specified.
 */
class CFEArrayDeclarator : public CFEDeclarator
{
// standard constructor/destructor
  public:
    /** constructs an array declarator
     *  \param pDecl the declarator the array bounds are added to */
    CFEArrayDeclarator(CFEDeclarator * pDecl);
    /** constructs an array declarator
     *  \param sName the name of the declarator
     *  \param pUpper its boundary
     */
    CFEArrayDeclarator(string sName, CFEExpression * pUpper = 0);
    virtual ~ CFEArrayDeclarator();

  protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEArrayDeclarator(CFEArrayDeclarator & src);

// Methods
  public:
    virtual void ReplaceUpperBound(unsigned int nIndex, CFEExpression * pUpper);
    virtual void ReplaceLowerBound(unsigned int nIndex, CFEExpression * pLower);
    virtual void RemoveBounds(unsigned int nIndex);
    CObject *Clone();
    virtual unsigned int GetDimensionCount();
    virtual int AddBounds(CFEExpression * pLower, CFEExpression * pUpper);
    virtual CFEExpression *GetUpperBound(unsigned int nDimension = 0);
    virtual CFEExpression *GetLowerBound(unsigned int nDimension = 0);

// attributes
  protected:
    /** \var vector<CFEExpression*> m_vLowerBounds
     *  \brief contains the lower bounds of the array definitions
     */
     vector<CFEExpression*> m_vLowerBounds;
    /** \var vector<CFEExpression*> m_vUpperBounds
     *  \brief contains the upper bound of the array definitions
     */
    vector<CFEExpression*> m_vUpperBounds;
};

#endif                /* __DICE_FE_FEARRAYDECLARATOR_H__ */
