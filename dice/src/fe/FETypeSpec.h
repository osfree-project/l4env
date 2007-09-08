/**
 *  \file   dice/src/fe/FETypeSpec.h
 *  \brief  contains the declaration of the class CFETypeSpec
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_FE_FETYPESPEC_H__
#define __DICE_FE_FETYPESPEC_H__

#include "fe/FEInterfaceComponent.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "template.h"
#include <vector>

class CFEAttribute;

/** \class CFETypeSpec
 *  \ingroup frontend
 *  \brief the base class for all types
 */
class CFETypeSpec : public CFEInterfaceComponent
{

// standard constructor/destructor
public:
    /** simple constructor for type spec object
     *  \param nType the type of the type spec */
    CFETypeSpec(unsigned int nType = TYPE_NONE);
    virtual ~CFETypeSpec();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFETypeSpec(CFETypeSpec* src);

// operations
public:
	virtual CObject* Clone();
    virtual bool IsConstructedType();
    virtual bool IsPointerType();

    /** retrieves the type of the type spec
     *  \return the type of the type spec
     */
    unsigned int GetType()
    { return m_nType; }

    void AddAttributes(std::vector<CFEAttribute*> *pAttributes);

// attributes
protected:
    /** \var unsigned int m_nType
     *  \brief which type is represented
     */
    unsigned int m_nType;

public:
    /** \var CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes
     *  \brief the interface's attributes
     */
    CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes;
};

#endif /* __DICE_FE_FETYPESPEC_H__ */

