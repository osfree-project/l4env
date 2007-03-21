/**
 *  \file    dice/src/fe/FEInterface.h
 *  \brief   contains the declaration of the class CFEInterface
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
#ifndef __DICE_FE_FEINTERFACE_H__
#define __DICE_FE_FEINTERFACE_H__

#include "FEFileComponent.h"
#include "FEConstDeclarator.h"
#include "Attribute-Type.h" // needed for ATTR_TYPE
#include "template.h"
#include <string>
#include <vector>

class CFEInterface;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEIdentifier;
class CFEAttributeDeclarator;
class CFEAttribute;
class CFEInterfaceComponent;

/** \class CFEInterface
 *    \ingroup frontend
 *  \brief the representation of an interface
 *
 * This class is used to represent an interface
 */
class CFEInterface : public CFEFileComponent
{

// standard constructor/destructor
public:
    /**
     *  \brief creates an interface representation
     *  \param pIAttributes the attributes of the interface
     *  \param sIName the name of the interface
     *  \param pIBaseNames the names of the base interfaces
     *  \param pParent reference to the parent of the interface
     */
    CFEInterface(vector<CFEAttribute*> *pIAttributes,
        string sIName,
        vector<CFEIdentifier*> *pIBaseNames,
        CFEBase* pParent);
    virtual ~CFEInterface();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEInterface(CFEInterface &src);

// Operations
public:
    virtual CObject* Clone();
    virtual CFEInterface* FindBaseInterface(string sName);
    virtual void AddBaseInterface(CFEInterface* pBaseInterface);

    virtual void Accept(CVisitor&);

    virtual int GetOperationCount(bool bCountBase = true);

    virtual string GetName();
    bool Match(string sName);

    virtual bool IsForward();
    void AddComponents(vector<CFEInterfaceComponent*> *pComponents);

// attributes
protected:
    /** \var string m_sInterfaceName
     *  \brief holds a reference to the name of the interface
     */
    string m_sInterfaceName;

public:
    /** \var CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes
     *  \brief the interface's attributes
     */
    CSearchableCollection<CFEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CSearchableCollection<CFEConstDeclarator, string> m_Constants
     *  \brief the interface's constants
     */
    CSearchableCollection<CFEConstDeclarator, string> m_Constants;
    /** \var CSearchableCollection<CFEAttributeDeclarator, string> m_AttributeDeclarators
     *  \brief holds interface attribute member declarators
     */
    CSearchableCollection<CFEAttributeDeclarator, string> m_AttributeDeclarators;
    /** \var CSearchableCollection<CFEConstructedType, string> m_TaggedDeclarators
     *  \brief the interface's constructed types
     */
    CSearchableCollection<CFEConstructedType, string> m_TaggedDeclarators;
    /** \var CCollection<CFEOperation> m_Operations
     *  \brief the interface's operations
     */
    CCollection<CFEOperation> m_Operations;
    /** \var CSearchableCollection<CFETypedDeclarator, string> m_Typedefs
     *  \brief the interface's type definitions
     */
    CSearchableCollection<CFETypedDeclarator, string> m_Typedefs;
    /** \var CSearchableCollection<CFETypedDeclarator, string> m_Exceptions
     *  \brief the interface's exceptions
     */
    CSearchableCollection<CFETypedDeclarator, string> m_Exceptions;
    /** \var CCollection<CFEIdentifier> m_BaseInterfaceNames
     *  \brief holds the base interface names
     */
    CCollection<CFEIdentifier> m_BaseInterfaceNames;
    /** \var CCollection<CFEInterface> m_DerivedInterfaces
     *  \brief an array of references to derived interfaces
     */
    CCollection<CFEInterface> m_DerivedInterfaces;
    /** \var CCollection<CFEInterface> m_BaseInterfaces
     *  \brief an array of reference to the base interfaces
     */
    CCollection<CFEInterface> m_BaseInterfaces;
};

#endif /* __DICE_FE_FEINTERFACE_H__ */
