/**
 *    \file    dice/src/fe/FEInterface.h
 *    \brief   contains the declaration of the class CFEInterface
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
#ifndef __DICE_FE_FEINTERFACE_H__
#define __DICE_FE_FEINTERFACE_H__

#include "fe/FEFileComponent.h"
#include "Attribute-Type.h" // needed for ATTR_TYPE
#include <string>
#include <vector>
using namespace std;

class CFEInterface;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEIdentifier;
class CFEAttributeDeclarator;
class CFEAttribute;
class CFEInterfaceComponent;

/** \class CFEInterface
 *    \ingroup frontend
 *    \brief the representation of an interface
 *
 * This class is used to represent an interface
 */
class CFEInterface : public CFEFileComponent
{

// standard constructor/destructor
public:
    /**
     *    \brief creates an interface representation
     *    \param pIAttributes the attributes of the interface
     *    \param sIName the name of the interface
     *    \param pIBaseNames the names of the base interfaces
     *    \param pComponents the components of the statement
     */
    CFEInterface(vector<CFEAttribute*> *pIAttributes,
        string sIName,
        vector<CFEIdentifier*> *pIBaseNames,
        vector<CFEInterfaceComponent*> *pComponents);
    virtual ~CFEInterface();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CFEInterface(CFEInterface &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual void Dump();

    virtual CFEAttribute* FindAttribute(ATTR_TYPE nType);
    virtual CFEAttribute* GetNextAttribute(vector<CFEAttribute*>::iterator &iter);
    virtual vector<CFEAttribute*>::iterator GetFirstAttribute();
    virtual CObject* Clone();

    virtual CFEInterface* FindBaseInterface(string sName);
    virtual CFEIdentifier* GetNextBaseInterfaceName(vector<CFEIdentifier*>::iterator &iter);
    virtual vector<CFEIdentifier*>::iterator GetFirstBaseInterfaceName();

    virtual bool CheckConsistency();

    virtual CFETypedDeclarator* FindUserDefinedType(string sName);
    virtual CFETypedDeclarator* GetNextTypeDef(vector<CFETypedDeclarator*>::iterator &iter);
    virtual vector<CFETypedDeclarator*>::iterator GetFirstTypeDef();
    virtual void AddTypedef(CFETypedDeclarator *pFETypedef);

    virtual CFEConstDeclarator* FindConstant(string sName);
    virtual CFEConstDeclarator* GetNextConstant(vector<CFEConstDeclarator*>::iterator &iter);
    virtual vector<CFEConstDeclarator*>::iterator GetFirstConstant();
    virtual void AddConstant(CFEConstDeclarator *pFEConstant);

    virtual void AddOperation(CFEOperation *pOperation);
    virtual int GetOperationCount(bool bCountBase = true);
    virtual CFEOperation* GetNextOperation(vector<CFEOperation*>::iterator &iter);
    virtual vector<CFEOperation*>::iterator GetFirstOperation();

    virtual void AddBaseInterface(CFEInterface* pBaseInterface);
    virtual CFEInterface* GetNextBaseInterface(vector<CFEInterface*>::iterator &iter);
    virtual vector<CFEInterface*>::iterator GetFirstBaseInterface();

    virtual void AddDerivedInterface(CFEInterface* pDerivedInterface);
    virtual CFEInterface* GetNextDerivedInterface(vector<CFEInterface*>::iterator &iter);
    virtual vector<CFEInterface*>::iterator GetFirstDerivedInterface();

    virtual string GetName();

    virtual void AddTaggedDecl(CFEConstructedType *pFETaggedDecl);
    virtual CFEConstructedType* GetNextTaggedDecl(vector<CFEConstructedType*>::iterator &iter);
    virtual vector<CFEConstructedType*>::iterator GetFirstTaggedDecl();
    virtual CFEConstructedType* FindTaggedDecl(string sName);

    virtual void AddAttributeDeclarator(CFEAttributeDeclarator* pAttrDecl);
    virtual CFEAttributeDeclarator* GetNextAttributeDeclarator(vector<CFEAttributeDeclarator*>::iterator &iter);
    virtual vector<CFEAttributeDeclarator*>::iterator GetFirstAttributeDeclarator();
    virtual CFEAttributeDeclarator* FindAttributeDeclarator(string sName);

    virtual bool IsForward();
    virtual void AddBaseInterfaceNames(vector<CFEIdentifier*> *pSrcNames);
    virtual void AddAttributes(vector<CFEAttribute*> *pSrcAttributes);


// attributes
protected:
    /** \var vector<CFEInterface*> m_vBaseInterfaces
     *  \brief an array of reference to the base interfaces
     */
    vector<CFEInterface*> m_vBaseInterfaces;
    /** \var vector<CFEInterface*> m_vDerivedInterfaces
     *  \brief an array of references to derived interfaces
     */
    vector<CFEInterface*> m_vDerivedInterfaces;
    /**    \var string m_sInterfaceName
     *    \brief holds a reference to the name of the interface
     */
    string m_sInterfaceName;
    /** \var vector<CFEIdentifier*> m_vBaseInterfaceNames
     *  \brief holds the base interface names
     */
    vector<CFEIdentifier*> m_vBaseInterfaceNames;
    /** \var vector<CFEAttributeDeclarator*> m_vAttributeDeclarators
     *  \brief holds interface attribute member declarators
     */
    vector<CFEAttributeDeclarator*> m_vAttributeDeclarators;
    /** \var vector<CFEAttribute*> m_vAttributes
     *  \brief the interface's attributes
     */
    vector<CFEAttribute*> m_vAttributes;
    /** \var vector<CFEConstDeclarator*> m_vConstants
     *  \brief the interface's constants
     */
    vector<CFEConstDeclarator*> m_vConstants;
    /** \var vector<CFETypedDeclarator*> m_vTypedefs
     *  \brief the interface's type definitions
     */
    vector<CFETypedDeclarator*> m_vTypedefs;
    /** \var vector<CFEOperation*> m_vOperations
     *  \brief the interface's operations
     */
    vector<CFEOperation*> m_vOperations;
    /** \var vector<CFEConstructedType*> m_vTaggedDeclarators
     *  \brief the interface's constructed types
     */
    vector<CFEConstructedType*> m_vTaggedDeclarators;
};

#endif /* __DICE_FE_FEINTERFACE_H__ */
