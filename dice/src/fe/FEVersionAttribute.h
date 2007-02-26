/**
 *	\file	dice/src/fe/FEVersionAttribute.h 
 *	\brief	contains the declaration of the class CFEVersionAttribute
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
#ifndef __DICE_FE_FEVERSIONATTRIBUTE_H__
#define __DICE_FE_FEVERSIONATTRIBUTE_H__

#include "fe/FEAttribute.h"

/** \struct _version_t
 *	\ingroup frontend
 *	\brief helper struct for parser
 *
 * This struct contains the members of a version, defined in the IDL's grammar.
 * It has been introduced to simplify the grammar.
 */
typedef struct _version_t {
	/** \var int nMajor
	 *	\brief the major number of the version
	 */
	int nMajor;
	/** \var int nMinor
	 *	\brief the minor number of the version
	 */
	int nMinor;
} version_t; /**< alias type for struct _version_t */

/** \class CFEVersionAttribute
 *	\ingroup frontend
 *	\brief The version attribute class.
 *
 * This class represents the version attribute, which can be specified
 * with an interface or an library.
 */
class CFEVersionAttribute : public CFEAttribute
{
DECLARE_DYNAMIC(CFEVersionAttribute);

// standard constructor/destructor
public:
	/** version attribute object contructor
	 *	\param nMajor the major version number
	 *	\param nMinor the minor version number (default is -1) */
	CFEVersionAttribute(int nMajor, int nMinor = -1);
	/** version attribute object constructor
	 *	\param version a version_t structure for convenience */
	CFEVersionAttribute(version_t version);
	virtual ~CFEVersionAttribute();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEVersionAttribute(CFEVersionAttribute &src);

// operations
public:
	virtual void Serialize(CFile *pFile);
	virtual CObject* Clone();
	virtual version_t GetVersion();
	virtual void GetVersion(int& major, int& minor);

// attributes
protected:
	/** \var int m_nMajor
	 *  \brief the major version number
	 */
	int m_nMajor;
	/** \var int m_nMinor
	 * \brief the minor version number
	 */
	int m_nMinor;
};

#endif /* __DICE_FE_FEVERSIONATTRIBUTE_H__ */

