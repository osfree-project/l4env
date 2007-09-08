/**
 *  \file    dice/src/be/l4/L4BESwitchCase.cpp
 *  \brief   contains the implementation of the class CL4BESwitchCase
 *
 *  \date    12/12/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "be/l4/L4BESwitchCase.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFile.h"

#include "Attribute-Type.h"

CL4BESwitchCase::CL4BESwitchCase()
 : CBESwitchCase()
{ }

/** destroys this object */
CL4BESwitchCase::~CL4BESwitchCase()
{ }

/** \brief initialize local variables
 *  \param pFile the file to write to
 *  \param nDirection the direction of the parameters to initailize
 *
 * If we have a [out, ref] for which we allocated memory in the switch case,
 * then this memory has to be valid until the reply, which is after this
 * switch case and the dispatch function.
 *
 * \todo we have to remember which indirect strings are associated with
 * dynamically allocated memory and have to be freed after the IPC.
 */
void CL4BESwitchCase::WriteVariableInitialization(CBEFile& pFile, DIRECTION_TYPE nDirection)
{
	// first call the base class
	CBESwitchCase::WriteVariableInitialization(pFile, nDirection);
	// now check for [out, ref] and call appropriate "deferred" cleanup method
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		if (!DoWriteVariable(*iter))
			continue;
		if (!(*iter)->IsDirection(nDirection))
			continue;
		if ((*iter)->m_Attributes.Find(ATTR_IN))
			continue;
		if (!(*iter)->m_Attributes.Find(ATTR_REF))
			continue;
		(*iter)->WriteCleanup(pFile, true);
	}
}

/** \brief writes the clean up code
 *  \param pFile the file to write to
 *
 * If we have an [out, ref] skip the cleanup here, because it is already
 * registered for "deferred" cleanup.
 */
void CL4BESwitchCase::WriteCleanup(CBEFile& pFile)
{
	// cleanup indirect variables
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
		iter != m_Parameters.end();
		iter++)
	{
		if (!DoWriteVariable(*iter))
			continue;
		if (!(*iter)->IsDirection(DIRECTION_OUT))
			continue;
		if ((*iter)->m_Attributes.Find(ATTR_IN))
			continue;
		if ((*iter)->m_Attributes.Find(ATTR_REF))
			continue;
		(*iter)->WriteCleanup(pFile, false);
	}
}
