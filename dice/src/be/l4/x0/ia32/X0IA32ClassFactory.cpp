/**
 *	\file	dice/src/be/l4/x0/ia32/X0IA32ClassFactory.h
 *	\brief	contains the declaration of the class CX0IA32ClassFactory
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

#include "be/l4/x0/ia32/X0IA32ClassFactory.h"

#include "be/l4/x0/ia32/X0IA32IPC.h"

IMPLEMENT_DYNAMIC(CX0IA32ClassFactory);

CX0IA32ClassFactory::CX0IA32ClassFactory(bool bVerbose)
 : CL4X0BEClassFactory(bVerbose)
{
    IMPLEMENT_DYNAMIC_BASE(CX0IA32ClassFactory, CL4X0BEClassFactory);
}

/** destroys the class factory */
CX0IA32ClassFactory::~CX0IA32ClassFactory()
{
}

/** \brief creates a new IPC class
 *  \return a reference to the new IPC class
 */
CBECommunication* CX0IA32ClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CX0IA32ClassFactory: created class CX0IA32IPC\n");
    return new CX0IA32IPC();
}
