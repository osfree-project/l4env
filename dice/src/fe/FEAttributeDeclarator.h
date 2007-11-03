/**
 *    \file    dice/src/fe/FEAttributeDeclarator.h
 *  \brief   contains the declaration of the class CFEAttributeDeclarator
 *
 *    \date    12/17/2003
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

#ifndef FEATTRIBUTEDECLARATOR_H
#define FEATTRIBUTEDECLARATOR_H

#include <fe/FETypedDeclarator.h>

/** \class CFEAttributeDeclarator
 *  \ingroup frontend
 *  \brief contains the code for an interface's attribute member (in contrary to an interface attribute)
 *
 */
class CFEAttributeDeclarator : public CFETypedDeclarator
{

public:
    /** \brief constructor for the attribute declarator
     *  \param pType the type of the declared variables
     *  \param pDeclarators the variables belonging to this declaration
     *  \param pTypeAttributes the attributes, associated with the type of this declaration
        */
    CFEAttributeDeclarator(CFETypeSpec *pType,
            vector<CFEDeclarator*> *pDeclarators,
            vector<CFEAttribute*> *pTypeAttributes = 0);
    virtual ~CFEAttributeDeclarator();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEAttributeDeclarator(CFEAttributeDeclarator* src);

public:
	virtual CFEAttributeDeclarator* Clone();
};

#endif
