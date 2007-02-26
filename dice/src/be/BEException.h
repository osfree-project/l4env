/**
 *	\file	dice/src/be/BEException.h
 *	\brief	contains the declaration of the class CBEException
 *
 *	\date	01/15/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#ifndef __DICE_BEEXCEPTION_H__
#define __DICE_BEEXCEPTION_H__

#include "be/BEObject.h"

class CFEIdentifier;
class CBEContext;

/**	\class CBEException
 *	\ingroup backend
 *	\brief the back-end attribute
 */
class CBEException : public CBEObject  
{
DECLARE_DYNAMIC(CBEException);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEException();
	virtual ~CBEException();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEException(CBEException &src);

public:
	virtual bool CreateBackEnd(CFEIdentifier *pFEException, CBEContext *pContext);

protected:
	/**	\var String m_sName
	 *	\brief the name of the exception
	 */
	String m_sName;
};

#endif // !__DICE_BEEXCEPTION_H__
