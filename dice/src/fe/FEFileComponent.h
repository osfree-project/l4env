/**
 *	\file	dice/src/fe/FEFileComponent.h 
 *	\brief	contains the declaration of the class CFEFileComponent
 *
 *	\date	02/22/2001
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
#ifndef __DICE_FE_FEFILECOMPONENT_H__
#define __DICE_FE_FEFILECOMPONENT_H__

#include "fe/FEBase.h"

class CFEFile;

/**	\class CFEFileComponent
 *	\ingroup frontend
 *	\brief describes the file's components (is base class for interface, lib, etc.)
 */
class CFEFileComponent : public CFEBase  
{
DECLARE_DYNAMIC(CFEFileComponent);

// standard constructor/destructor
public:
	CFEFileComponent();
	virtual ~CFEFileComponent();

protected:
	/**	\brief copy consrtructor
	 *	\param src the source to copy from
	 */
	CFEFileComponent(CFEFileComponent &src);

// Operations
public:

// Attributes
protected:
};

#endif // __DICE_FE_FEFILECOMPONENT_H__
