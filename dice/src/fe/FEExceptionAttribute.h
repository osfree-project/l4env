/**
 *  \file    dice/src/fe/FEExceptionAttribute.h
 *  \brief   contains the declaration of the class CFEExceptionAttribute
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
#ifndef __DICE_FE_FEEXCEPTIONATTRIBUTE_H__
#define __DICE_FE_FEEXCEPTIONATTRIBUTE_H__

#include "fe/FEAttribute.h"
#include "template.h"
#include <vector>

class CFEIdentifier;

/** \class CFEExceptionAttribute
 *  \ingroup frontend
 *  \brief represents an exception attribute
 *
 * This class is used to represent an exception attribute in the IDL.
 */
class CFEExceptionAttribute : public CFEAttribute
{

// standard constructor/destructor
public:
    /** constructs an exception attribute
     *  \param pExcepNames the names of the exceptions
     */
    CFEExceptionAttribute(vector<CFEIdentifier*> *pExcepNames);
    virtual ~CFEExceptionAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEExceptionAttribute(CFEExceptionAttribute &src);

// Operations
public:
    /** creates a copy of this object
     *  \return a copy of this object
     */
    virtual CObject* Clone()
    { return new CFEExceptionAttribute(*this); }

// attributes
public:
    /** \var CCollection<CFEIdentifier> m_ExceptionNames
     *  \brief contais all the exception's names
     */
    CCollection<CFEIdentifier> m_ExceptionNames;
};

#endif /* __DICE_FE_FEEXCEPTIONATTRIBUTE_H__ */

