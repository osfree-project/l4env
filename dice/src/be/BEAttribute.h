/**
 *	\file	BEAttribute.h
 *	\brief	contains the declaration of the class CBEAttribute
 *
 *	\date	01/15/2002
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
#ifndef __DICE_BEATTRIBUTE_H__
#define __DICE_BEATTRIBUTE_H__

#include "be/BEObject.h"
#include "Vector.h"

class CFEAttribute;
class CFEIntAttribute;
class CFEIsAttribute;
class CFEStringAttribute;
class CFETypeAttribute;
class CFEVersionAttribute;

class CBEContext;
class CBEType;
class CBEDeclarator;

enum ATTR_CLASS
{
    ATTR_CLASS_NONE    = 0,   /**< the attribute belongs to no attribute class */
    ATTR_CLASS_SIMPLE  = 1,   /**< the attribute is a simple attribute */
    ATTR_CLASS_INT     = 2,   /**< the attribute is an integer attribute */
    ATTR_CLASS_IS      = 3,   /**< the attribute is an IS attribute */
    ATTR_CLASS_STRING  = 4,   /**< the attribute is a string attribute */
    ATTR_CLASS_TYPE    = 5,   /**< the attribute is a type attribute */
    ATTR_CLASS_VERSION = 6    /**< the attribute is a version attribute */
};

/**	\class CBEAttribute
 *	\ingroup backend
 *	\brief the back-end attribute
 */
class CBEAttribute:public CBEObject
{
DECLARE_DYNAMIC(CBEAttribute);
// Constructor
public:
  /**	\brief constructor
   */
  CBEAttribute();
  virtual ~ CBEAttribute();

protected:
  /**	\brief copy constructor
   *	\param src the source to copy from
   */
  CBEAttribute(CBEAttribute & src);

public:
    virtual CBEDeclarator * GetNextIsAttribute(VectorElement * &pIter);
    virtual VectorElement *GetFirstIsAttribute();
    virtual int GetType();
    virtual void AddIsParameter(CBEDeclarator * pDecl);
    virtual bool CreateBackEnd(int nType, CBEContext * pContext);
    virtual bool CreateBackEnd(CFEAttribute * pFEAttribute, CBEContext * pContext);
    virtual bool IsOfType(ATTR_CLASS nType);
    virtual int GetIntValue();
    virtual CObject * Clone();
    virtual CBEDeclarator* FindIsParameter(String sName);
    virtual String GetString();
	virtual int GetRemainingNumberOfIsAttributes(VectorElement *pIter);
	virtual CBEType* GetAttrType();

protected:
    virtual bool CreateBackEndInt(CFEIntAttribute * pFEIntAttribute, CBEContext * pContext);
    virtual bool CreateBackEndIs(CFEIsAttribute * pFEIsAttribute, CBEContext * pContext);
    virtual bool CreateBackEndString(CFEStringAttribute * pFEStringAttribute, CBEContext * pContext);
    virtual bool CreateBackEndType(CFETypeAttribute * pFETypeAttribute, CBEContext * pContext);
    virtual bool CreateBackEndVersion(CFEVersionAttribute * pFEVersionAttribute, CBEContext * pContext);

protected:
  /**	\var int m_nType
   *	\brief m_nType contains the attribute's type (and helps select which of the other members to use)
   */
  int m_nType;
  /** \var ATTR_CLASS m_nAttrClass
   *  \brief contains the attribute class
   */
  ATTR_CLASS m_nAttrClass;
  /**	\var Vector *m_pPortSpecs
   *	\brief contains the EndPoint Attributes specs if any
   */
  Vector *m_pPortSpecs;
  /**	\var Vector *m_pExceptions
   *	\brief contains the exception attributes if any
   */
  Vector *m_pExceptions;
  /**	\var int m_nIntValue
   *	\brief contains the int attribute's value if any
   */
  int m_nIntValue;
  /**	\var Vector m_pParameters
   *	\brief contains the values of the Is attributes (if any)
   */
  Vector *m_pParameters;
  /**	\var CBEAttribute m_pPtrDefault
   *	\brief the Pointer default attribute value
   */
  CBEAttribute *m_pPtrDefault;
  /**	\var String m_sString
   *	\brief the value of the String attribute
   */
  String m_sString;
  /**	\var CBEType m_pType
   *	\brief contains the type of the type attribute
   */
  CBEType *m_pType;
  /**	\var int m_nMinorVersion
   *	\brief contains the minor version information if this is a version attribute
   */
  int m_nMinorVersion;
  /** \var int m_nMajorVersion
   *  \brief the major part of the version information
   */
  int m_nMajorVersion;
};

#endif				//*/ !__DICE_BEATTRIBUTE_H__
