/**
 *    \file    dice/src/fe/FEAttributeDeclarator.cpp
 *  \brief   contains the implementation of the class CFEAttributeDeclarator
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

#include "fe/FEAttributeDeclarator.h"

CFEAttributeDeclarator::CFEAttributeDeclarator(CFETypeSpec * pType,
                       vector<CFEDeclarator*> * pDeclarators,
                       vector<CFEAttribute*> * pTypeAttributes)
 : CFETypedDeclarator(TYPEDECL_ATTRIBUTE, pType, pDeclarators, pTypeAttributes)
{
}

CFEAttributeDeclarator::CFEAttributeDeclarator(CFEAttributeDeclarator& src)
 : CFETypedDeclarator(src)
{
}

/** deletes this object */
CFEAttributeDeclarator::~CFEAttributeDeclarator()
{
}

/** \brief clones this object
 *  \return a reference to a new instance of this class
 */
CObject* CFEAttributeDeclarator::Clone()
{
    return new CFEAttributeDeclarator(*this);
}
