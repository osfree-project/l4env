/**
 *	\file	dice/src/be/BECommunication.cpp
 *	\brief	contains the implementation of the class CBECommunication
 *
 *	\date	08/13/2003
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

#include "BECommunication.h"

IMPLEMENT_DYNAMIC(CBECommunication);

CBECommunication::CBECommunication()
 : CBEObject()
{
    IMPLEMENT_DYNAMIC_BASE(CBECommunication, CBEObject);
}

/** clean up this object */
CBECommunication::~CBECommunication()
{
}
