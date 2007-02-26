/**
 *	\file	dice/src/be/l4/x0/arm/X0ArmClassFactory.h
 *	\brief	contains the declaration of the class CX0ArmClassFactory
 *
 *	\date	08/13/2002
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
#ifndef X0ARMCLASSFACTORY_H
#define X0ARMCLASSFACTORY_H

#include <be/l4/x0/L4X0BEClassFactory.h>

/** \class CX0ArmClassFactory
 *  \ingroup backend
 *  \brief generates L4 X0 specific classes for the Arm architecture
 */
class CX0ArmClassFactory : public CL4X0BEClassFactory
{
DECLARE_DYNAMIC(CX0ArmClassFactory);

public:
    /** creates a new class factory
	 *  \param bVerbose true if the factory should generate verbose output
	 */
    CX0ArmClassFactory(bool bVerbose = false);

    ~CX0ArmClassFactory();

public:
    virtual CBECommunication* GetNewCommunication();
};

#endif

