/**
 *	\file	dice/src/fe/FEConstructedType.h 
 *	\brief	contains the declaration of the class CFEConstructedType
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
#ifndef __DICE_FE_FECONSTRUCTEDTYPE_H__
#define __DICE_FE_FECONSTRUCTEDTYPE_H__

#include "fe/FETypeSpec.h"

/* CFEConstructedType : for class-type checking only (ppure virtual class) */
/** \class CFEConstructedType
 *	\ingroup frontend
 *	\brief base class of constructed types
 *
 * This class is only used for type checking. If the type checking (whether a
 * type is a constructed type) can be implemented another way, this class can
 * be removed.
 */
class CFEConstructedType : public CFETypeSpec
{
DECLARE_DYNAMIC(CFEConstructedType);

// standard constructor/destructor
public:
	/** CFEConstructedType constructor 
	 *	\param nType the type f the constructed type
	 */
	CFEConstructedType(TYPESPEC_TYPE nType);
	virtual ~CFEConstructedType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEConstructedType(CFEConstructedType &src);
};

#endif /* __DICE_FE_FECONSTRUCTEDTYPE_H__ */

