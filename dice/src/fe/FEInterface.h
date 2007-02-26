/**
 *	\file	dice/src/fe/FEInterface.h 
 *	\brief	contains the declaration of the class CFEInterface
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "fe/FEAttribute.h" // needed for ATTR_TYPE
#include "CString.h"
#include "Vector.h"

class CFEInterface;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEIdentifier;

/** \class CFEInterface
 *	\ingroup frontend
 *	\brief the representation of an interface
 *
 * This class is used to represent an interface
 */
class CFEInterface : public CFEFileComponent
{
DECLARE_DYNAMIC(CFEInterface);

// standard constructor/destructor
public:
	/**
	 *	\brief creates an interface representation
	 *	\param pIAttributes the attributes of the interface
	 *	\param sIName the name of the interface
	 *	\param pIBaseNames the names of the base interfaces
	 *	\param pComponents the components of the statement
	 */
    CFEInterface(Vector *pIAttributes, 
		String sIName,
		Vector *pIBaseNames,
		Vector *pComponents);
    virtual ~CFEInterface();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEInterface(CFEInterface &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual void Dump();
    virtual void AddOperation(CFEOperation *pOperation);
    virtual CFEAttribute* FindAttribute(ATTR_TYPE nType);
    virtual CFEAttribute* GetNextAttribute(VectorElement *& iter);
    virtual VectorElement* GetFirstAttribute();
    virtual CObject* Clone();

    virtual CFEInterface* FindBaseInterface(String sName);
    virtual CFEIdentifier* GetNextBaseInterfaceName(VectorElement* &iter);
    virtual VectorElement* GetFirstBaseInterfaceName();

    virtual bool CheckConsistency();

    virtual CFETypedDeclarator* FindUserDefinedType(String sName);
    virtual CFETypedDeclarator* GetNextTypeDef(VectorElement* &iter);
    virtual VectorElement* GetFirstTypeDef();
	virtual void AddTypedef(CFETypedDeclarator *pFETypedef);

    virtual CFEConstDeclarator* FindConstant(String sName);
    virtual CFEConstDeclarator* GetNextConstant(VectorElement* &iter);
    virtual VectorElement* GetFirstConstant();
	virtual void AddConstant(CFEConstDeclarator *pFEConstant);

    virtual int GetOperationCount(bool bCountBase = true);
    virtual CFEOperation* GetNextOperation(VectorElement* &iter);
    virtual VectorElement* GetFirstOperation();

    virtual void AddBaseInterface(CFEInterface* pBaseInterface);
    virtual CFEInterface* GetNextBaseInterface(VectorElement* &iter);
    virtual VectorElement* GetFirstBaseInterface();

    virtual void AddDerivedInterface(CFEInterface* pDerivedInterface);
    virtual CFEInterface* GetNextDerivedInterface(VectorElement* &iter);
    virtual VectorElement* GetFirstDerivedInterface();

    virtual String GetName();
    
	virtual void AddTaggedDecl(CFEConstructedType *pFETaggedDecl);
    virtual CFEConstructedType* GetNextTaggedDecl(VectorElement* &pIter);
    virtual VectorElement* GetFirstTaggedDecl();
    virtual CFEConstructedType* FindTaggedDecl(String sName);
    
    virtual bool IsForward();
    virtual void AddBaseInterfaceNames(Vector *pSrcNames);
    virtual void AddAttributes(Vector *pSrcAttributes);
  

// attributes
protected:
    /**	\var Vector* m_pBaseInterfaces
     *	\brief an array of reference to the base interfaces
     */
    Vector* m_pBaseInterfaces;
    /** \var Vector* m_pDerivedInterfaces
     *  \brief an array of references to derived interfaces
     */
    Vector* m_pDerivedInterfaces;
    /**	\var String m_sInterfaceName
     *	\brief holds a reference to the name of the interface
     */
    String m_sInterfaceName;
    /**	\var Vector *m_pBaseInterfaceNames
     *	\brief holds the base interface names
     */
    Vector *m_pBaseInterfaceNames;
    /** \var Vector m_vAttributes
     *  \brief the interface's attributes
     */
    Vector m_vAttributes;
    /** \var Vector m_vConstants
     *  \brief the interface's constants
     */
    Vector m_vConstants;
    /** \var Vector m_vTypedefs
     *  \brief the interface's type definitions
     */
    Vector m_vTypedefs;
    /** \var Vector m_vOperations
     *  \brief the interface's operations
     */
    Vector m_vOperations;
    /** \var Vector m_vTaggedDeclarators
     *  \brief the interface's constructed types
     */
    Vector m_vTaggedDeclarators;
};

#endif /* __DICE_FE_FEINTERFACE_H__ */

