/**
 *	\file	dice/src/fe/FEIdentifier.h 
 *	\brief	contains the declaration of the class CFEIdentifier
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
#ifndef __DICE_FE_FEIDENTIFIER_H__
#define __DICE_FE_FEIDENTIFIER_H__

#include "fe/FEBase.h"
#include "CString.h"

/**	\class CFEIdentifier
 *	\ingroup frontend
 *	\brief represents any identifier in the IDL
 *
 * This class is used to represent any identifier in the IDL
 */
class CFEIdentifier : public CFEBase
{
DECLARE_DYNAMIC(CFEIdentifier);

// standard constructor/destructor
public:
	/**	default constructor */
	CFEIdentifier();
	/** constructs an identifier object
	 *	\param sName a string, defining the name of the identifier
	 */
    CFEIdentifier(String& sName);
	/** copy constructor for this class
	 *	\param src the source object for this new object
	 */
	CFEIdentifier(CFEIdentifier& src);
	/** constructs an identifier object
	 *	\param sName a character string, defining the name of the identifier
	 */
	CFEIdentifier(const char* sName);
    virtual ~CFEIdentifier();

//operations
public:
	virtual String ReplaceName(String sNewName);
	virtual void Suffix(String sSuffix);
	virtual void Prefix(String sPrefix);
	virtual CObject* Clone();
	virtual String GetName();
	bool operator==(CFEIdentifier&);
	bool operator==(String& Name);

// attributes
protected:
	/**	\var String m_sName
	 *	\brief the name of the identifier
	 */
    String m_sName;
};

#endif /* __DICE_FE_FEIDENTIFIER_H__ */

