/**
 *	\file	dice/src/Object.h 
 *	\brief	contains the declaration of the class CObject
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
#ifndef __DICE_OBJECT_H__
#define __DICE_OBJECT_H__

#include "defines.h"

/**	\class CObject
 *	\ingroup backend
 *	\brief base class for all classes
 */
class CObject
{
DECLARE_DYNAMIC(CObject);

// Constructor
  public:
	/** the constructor for this class */
    CObject(CObject * pParent = 0);
    virtual ~ CObject();

  protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
    CObject(CObject & src);

// Operations
  public:
    virtual CObject * Clone();
    virtual void SetParent(CObject * pParent = 0);
    virtual CObject *GetParent();

// Attributes
  protected:
	/**	\var CObject *m_pParent
	 *	\brief a reference to the parent object
	 */
     CObject * m_pParent;
};

#endif				// __DICE_OBJECT_H__
