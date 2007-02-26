/**
 *  \file   dice/src/be/l4/x0/arm/X0ArmClassFactory.cpp
 *  \brief  contains the declaration of the class CX0ArmClassFactory
 *
 *  \date   08/13/2002
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

#include "be/l4/x0/arm/X0ArmClassFactory.h"

#include "be/l4/x0/arm/X0ArmIPC.h"


CX0ArmClassFactory::CX0ArmClassFactory(bool bVerbose)
 : CL4X0BEClassFactory(bVerbose)
{
}

/** destroy class factory */
CX0ArmClassFactory::~CX0ArmClassFactory()
{
}

/** \brief creates a new instance of the IPC class
 *  \return a reference to the new instance
 */
CBECommunication* CX0ArmClassFactory::GetNewCommunication()
{
    if (m_bVerbose)
        printf("CX0ArmClassFactory: created class CX0ArmIPC\n");
    return new CX0ArmIPC();
}

