/**
 *	\file	dice/src/be/BEInterfaceFunction.h
 *	\brief	contains the declaration of the class CBEInterfaceFunction
 *
 *	\date	01/14/2002
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
#ifndef __DICE_BEINTERFACEFUNCTION_H__
#define __DICE_BEINTERFACEFUNCTION_H__

#include "be/BEFunction.h"
#include "be/BEClass.h"

class CFEInterface;
class CBEContext;

/**	\class CBEInterfaceFunction
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end interface (server loop, etc.)
 */
class CBEInterfaceFunction : public CBEFunction  
{
DECLARE_DYNAMIC(CBEInterfaceFunction);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEInterfaceFunction();
	virtual ~CBEInterfaceFunction();

protected:
	/**	\brief copy constructor */
	CBEInterfaceFunction(CBEInterfaceFunction &src);

public:
	virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);

protected:
};

#endif // !__DICE_BEINTERFACEFUNCTION_H__
