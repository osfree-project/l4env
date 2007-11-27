/**
 *  \file   dice/src/fe/FEInterfaceComponent.h
 *  \brief  contains the declaration of the class CFEInterfaceComponent
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_FE_FEINTERFACECOMPONENT_H__
#define __DICE_FE_FEINTERFACECOMPONENT_H__

#include "fe/FEFileComponent.h"

/** \class CFEInterfaceComponent
 *  \ingroup frontend
 *  \brief describes the interface's components (is base class)
 */
class CFEInterfaceComponent : public CFEFileComponent
{
protected:
	/** copy constructor
	 *  \param src the source to copy from
	 */
	CFEInterfaceComponent(CFEInterfaceComponent* src)
		: CFEFileComponent(src)
	{ }

public:
	/** constructor
	 *  \param pParent the parent of the interface component
	 */
	CFEInterfaceComponent(CObject *pParent)
		: CFEFileComponent(pParent)
	{ }
};

#endif

