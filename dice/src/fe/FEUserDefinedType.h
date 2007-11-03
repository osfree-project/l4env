/**
 *  \file    dice/src/fe/FEUserDefinedType.h
 *  \brief   contains the declaration of the class CFEUserDefinedType
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
#ifndef __DICE_FE_FEUSERDEFINEDTYPE_H__
#define __DICE_FE_FEUSERDEFINEDTYPE_H__

#include "fe/FETypeSpec.h"
#include <string>

/** \class CFEUserDefinedType
 *  \ingroup frontend
 *  \brief contains the alias name of a type
 *
 * The alias name of a type is set by a typedef declaration.
 */
class CFEUserDefinedType : public CFETypeSpec
{

// standard constructor/destructor
public:
    /** \brief creates a user defined type
     *  \param sName the name of the user defined type
        */
    CFEUserDefinedType(std::string sName);
    virtual ~CFEUserDefinedType();

protected:
    /** \brief copy constructor
     *  \param src the sorce to copy from
     */
    CFEUserDefinedType(CFEUserDefinedType* src);

// Operations
public:
	virtual CFEUserDefinedType* Clone();
    virtual void Accept(CVisitor&);
    virtual std::string GetName();
    virtual bool Ignore();
    virtual unsigned int GetOriginalType();
    virtual bool IsConstructedType();
    virtual bool IsPointerType();

// attributes
protected:
    /** \var std::string m_sName
     *  \brief the alias name
     */
    std::string m_sName;
};

#endif /* __DICE_FE_FEUSERDEFINEDTYPE_H__ */

