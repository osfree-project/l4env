/**
 *  \file    dice/src/fe/FEFunctionDeclarator.h
 *  \brief   contains the declaration of the class CFEFunctionDeclarator
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
#ifndef __DICE_FE_FEFUNCTIONDECLARATOR_H__
#define __DICE_FE_FEFUNCTIONDECLARATOR_H__

#include "fe/FEDeclarator.h"
#include "template.h"
#include <vector>

class CFETypedDeclarator;

/** \class CFEFunctionDeclarator
 *  \ingroup frontend
 *  \brief a class representing a function declarator
 *
 * This class is used to represent a function declarator
 */
class CFEFunctionDeclarator : public CFEDeclarator
{

// standard constructor/destructor
public:
    /** constructs a function declarator
     *  \param pDecl the name of the function
     *  \param pParams the parameters of the function
     */
    CFEFunctionDeclarator(CFEDeclarator *pDecl, vector<CFETypedDeclarator*> *pParams);
    virtual ~CFEFunctionDeclarator();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEFunctionDeclarator(CFEFunctionDeclarator* src);

public:
	virtual CObject* Clone();

// attributes
public:
    /** \var CCollection<CFETypedDeclarator> m_Parameters
     *  \brief contains all parameters of this function
     */
    CCollection<CFETypedDeclarator> m_Parameters;
};

#endif /* __DICE_FE_FEFUNCTIONDECLARATOR_H__ */

