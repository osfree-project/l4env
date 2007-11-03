/**
 *  \file    BEAttribute.h
 *  \brief   contains the declaration of the class CBEAttribute
 *
 *  \date    01/15/2002
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
#ifndef __DICE_BEATTRIBUTE_H__
#define __DICE_BEATTRIBUTE_H__

#include "be/BEObject.h"
#include "Attribute-Type.h"
#include "template.h"
#include <string>
#include <vector>

class CFEAttribute;
class CFEIntAttribute;
class CFERangeAttribute;
class CFEIsAttribute;
class CFEStringAttribute;
class CFETypeAttribute;
class CFEVersionAttribute;

class CBEType;
class CBEDeclarator;

/** \enum ATTR_CLASS
 *  \brief defines the valid attribute classes (kinds of attributes)
 */
enum ATTR_CLASS
{
    ATTR_CLASS_NONE		= 0,	/**< the attribute belongs to no attribute class */
    ATTR_CLASS_SIMPLE,			/**< the attribute is a simple attribute */
    ATTR_CLASS_INT,				/**< the attribute is an integer attribute */
    ATTR_CLASS_IS,				/**< the attribute is an IS attribute */
    ATTR_CLASS_STRING,			/**< the attribute is a string attribute */
    ATTR_CLASS_TYPE,			/**< the attribute is a type attribute */
    ATTR_CLASS_VERSION,			/**< the attribute is a version attribute */
	ATTR_CLASS_RANGE			/**< the attribute is a range of integer values */
};

/** \class CBEAttribute
 *  \ingroup backend
 *  \brief the back-end attribute
 */
class CBEAttribute : public CBEObject
{
	// Constructor
public:
	/** \brief constructor
	 */
	CBEAttribute();
	virtual ~ CBEAttribute();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CBEAttribute(CBEAttribute* src);

public:
	virtual void AddIsParameter(CBEDeclarator * pDecl);
	virtual int GetRemainingNumberOfIsAttributes(
		vector<CBEDeclarator*>::iterator iter);
	virtual CBEAttribute* Clone();
	virtual void CreateBackEnd(ATTR_TYPE nType);
	virtual void CreateBackEnd(CFEAttribute * pFEAttribute);
	virtual void CreateBackEndIs(ATTR_TYPE nType, CBEDeclarator *pDeclarator);
	virtual void CreateBackEndInt(ATTR_TYPE nType, int nValue);
	virtual void CreateBackEndRange(ATTR_TYPE nType, int nLower, int nUpper);
	virtual void CreateBackEndType(ATTR_TYPE nType, CBEType *pType);

	virtual void Write(CBEFile& pFile);
	virtual void WriteToStr(std::string &str);

	/** \brief tries to match the attribute type
	 *  \param nType the type to match
	 *  \return true if type matches
	 */
	bool Match(ATTR_TYPE nType)
	{ return m_nType == nType; }

	/** \brief returns the attribute's type
	 *  \return the member m_nType
	 */
	ATTR_TYPE GetType()
	{ return m_nType; }

	/** \brief returns class
	 *  \return the attribute's class
	 */
	ATTR_CLASS GetClass()
	{ return m_nAttrClass; }

	/** \brief checks the type of an attribute
	 *  \param nType the type to compare the own type to
	 *  \return true if the types are the same
	 */
	bool IsOfType(ATTR_CLASS nType)
	{ return (m_nAttrClass == nType); }

	/** \brief retrieves the integer value of this attribute
	 *  \return the value of m_nIntValue or -1 if m_nAttrClass != ATTR_CLASS_INT
	 */
	int GetIntValue()
	{
		return (m_nAttrClass == ATTR_CLASS_INT) ?
			m_nIntValue : -1;
	}

	/** \brief access string value
	 *  \return string member if ATTR_CLASS_STRING
	 */
	std::string GetString()
	{
		return (m_nAttrClass == ATTR_CLASS_STRING) ?
			m_sString : std::string();
	}

	/** \brief retrieve reference to the type of a type
	 *         attribute (such as transmit_as)
	 *  \return a reference to the type of a type attribute
	 */
	CBEType* GetAttrType()
	{ return m_pType; }

	/** \brief retrieve the range information
	 *  \param nLower the lower bound
	 *  \param nUpper the upper bound
	 */
	void GetRange(int& nLower, int& nUpper)
	{
		if (m_nAttrClass == ATTR_CLASS_RANGE)
		{
			nLower = m_nLowerInt;
			nUpper = m_nUpperInt;
		}
	}

	/** \brief return if the values are absolute or relative
	 *  \return true if absolute values
	 */
	bool IsValueAbsolute()
	{
		if (m_nAttrClass == ATTR_CLASS_RANGE ||
			m_nAttrClass == ATTR_CLASS_INT)
			return m_bIsAbsolute;
		return false;
	}

protected:
	virtual void CreateBackEndRange(CFERangeAttribute * pFERangeAttribute);
	virtual void CreateBackEndInt(CFEIntAttribute * pFEIntAttribute);
	virtual void CreateBackEndIs(CFEIsAttribute * pFEIsAttribute);
	virtual void CreateBackEndString(CFEStringAttribute * pFEStringAttribute);
	virtual void CreateBackEndType(CFETypeAttribute * pFETypeAttribute);
	virtual void CreateBackEndVersion(CFEVersionAttribute * pFEVersionAttribute);

protected:
	/** \var ATTR_TYPE m_nType
	 *  \brief m_nType contains the attribute's type (and helps select which
	 *    of the other members to use)
	 */
	ATTR_TYPE m_nType;
	/** \var ATTR_CLASS m_nAttrClass
	 *  \brief contains the attribute class
	 */
	ATTR_CLASS m_nAttrClass;
	/** \var vector<std::string> m_vPortSpecs
	 *  \brief contains the EndPoint Attributes specs if any
	 */
	vector<std::string> m_vPortSpecs;
	/** \var vector<std::string> m_vExceptions
	 *  \brief contains the exception attributes if any
	 */
	vector<std::string> m_vExceptions;
	/** \var int m_nIntValue
	 *  \brief contains the int attribute's value if any
	 */
	int m_nIntValue;
	/** \var CBEAttribute m_pPtrDefault
	 *  \brief the Pointer default attribute value
	 */
	CBEAttribute *m_pPtrDefault;
	/** \var std::string m_sString
	 *  \brief the value of the string attribute
	 */
	std::string m_sString;
	/** \var CBEType m_pType
	 *  \brief contains the type of the type attribute
	 */
	CBEType *m_pType;
	/** \var int m_nMinorVersion
	 *  \brief contains the minor version information if this is a version attribute
	 */
	int m_nMinorVersion;
	/** \var int m_nMajorVersion
	 *  \brief the major part of the version information
	 */
	int m_nMajorVersion;
	/** \var int m_nLowerInt;
	 *  \brief if a range, this is the lower bound
	 */
	int m_nLowerInt;
	/** \var int m_nUpperInt;
	 *  \brief if a range, this is the upper bound
	 */
	int m_nUpperInt;
	/** \var bool m_bIsAbsolute
	 *  \brief if Int or Range attribute this determines if the value is absolute
	 */
	bool m_bIsAbsolute;

public:
	/** \var CSearchableCollection<CBEDeclarator, std::string> m_Parameters
	 *  \brief contains the values of the Is attributes (if any)
	 */
	CSearchableCollection<CBEDeclarator, std::string> m_Parameters;
};

#endif                //*/ !__DICE_BEATTRIBUTE_H__
