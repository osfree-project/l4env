/**
 *  \file    dice/src/fe/FETypedDeclarator.h
 *  \brief   contains the declaration of the class CFETypedDeclarator
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
#ifndef __DICE_FE_FETYPEDDECLARATOR_H__
#define __DICE_FE_FETYPEDDECLARATOR_H__

// Typed declarator types
enum TYPEDDECL_TYPE {
    TYPEDECL_NONE,        /**< empty typed declarator (invalid) */
    TYPEDECL_VOID,        /**< void typed declarator (empty member branch, etc.) */
    TYPEDECL_EXCEPTION,    /**< exception declarator */
    TYPEDECL_PARAM,        /**< parameter declarator */
    TYPEDECL_FIELD,        /**< field declarator */
    TYPEDECL_TYPEDEF,    /**< typedef declarator */
    TYPEDECL_MSGBUF,    /**< is a message buffer type */
    TYPEDECL_ATTRIBUTE  /**< an interface attribute member */
};

#include "FEInterfaceComponent.h"
#include "FEAttribute.h"
#include "FEDeclarator.h"
#include "Attribute-Type.h" // needed for the declaration of ATTR_TYPE
#include "template.h"
#include <vector>

class CFETypeSpec;

/** \class CFETypedDeclarator
 *  \ingroup frontend
 *  \brief represents a typed declarator
 *
 * A typed declarator is a declared variable with a type and attributes.
 */
class CFETypedDeclarator : public CFEInterfaceComponent
{

// standard constructor/destructor
public:
    /** \brief constructor for the typed declarator
     *  \param nType the type of the declarator (parameter, exception, etc)
     *  \param pType the type of the declared variables
     *  \param pDeclarators the variables belonging to this declaration
     *  \param pTypeAttributes the attributes, associated with the type of this declaration
        */
    CFETypedDeclarator(TYPEDDECL_TYPE nType,
            CFETypeSpec *pType,
            vector<CFEDeclarator*> *pDeclarators,
            vector<CFEAttribute*> *pTypeAttributes = 0);
    virtual ~CFETypedDeclarator();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFETypedDeclarator(CFETypedDeclarator &src);

// operations
public:
    virtual void Accept(CVisitor&);
    virtual CObject* Clone();
    virtual CFETypeSpec* ReplaceType(CFETypeSpec *pNewType);
    virtual TYPEDDECL_TYPE GetTypedDeclType();
    virtual CFETypeSpec* GetType();
    virtual bool IsTypedef();
    bool Match(string sName);

// attributes
protected:
    /** \var TYPEDDECL_TYPE m_nType
     *  \brief the type of the declarator
     */
    TYPEDDECL_TYPE m_nType;
    /** \var CFETypeSpec *m_pType
     *  \brief the type of the declarator
     */
    CFETypeSpec *m_pType;

public:
    /** \var CCollection<CFEAttribute> m_Attributes
     *  \brief the attributes of the declarator
     */
    CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CCollection<CFEDeclarator> m_Declarators
     *   \brief the variable names
     */
    CSearchableCollection<CFEDeclarator, string> m_Declarators;
};

#endif /* __DICE_FE_FETYPEDDECLARATOR_H__ */

