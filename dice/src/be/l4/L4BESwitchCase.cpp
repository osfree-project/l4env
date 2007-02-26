/**
 *    \file    dice/src/be/l4/L4BESwitchCase.cpp
 *    \brief   contains the implementation of the class CL4BESwitchCase
 *
 *    \date    12/12/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/l4/L4BESwitchCase.h"
#include "be/BETypedDeclarator.h"

#include "Attribute-Type.h"

CL4BESwitchCase::CL4BESwitchCase()
 : CBESwitchCase()
{
}

CL4BESwitchCase::CL4BESwitchCase(CL4BESwitchCase &src)
 : CBESwitchCase(src)
{
}

/** destroys this object */
CL4BESwitchCase::~CL4BESwitchCase()
{
}

/** \brief writes the cleanup code for a switch case
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a [out, ref] for which we allocated memory in the switch case,
 * then this memory has to be valid until the reply, which is after this
 * switch case and the dispatch function.
 *
 * \todo we have to remember which indirect strings are associated with
 * dynamically allocated memory and have to be freed after the IPC.
 */
void CL4BESwitchCase::WriteCleanup(CBEFile* pFile,  CBEContext* pContext)
{
    // cleanup indirect variables
    vector<CBETypedDeclarator*>::iterator iter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(iter)) != 0)
    {
        if (!pParameter->IsDirection(DIRECTION_OUT))
            continue;
        if (pParameter->FindAttribute(ATTR_IN))
            continue;
        // up to here the same as the base class
        // now thest for [ref] which indicates indirect strings
        if (pParameter->FindAttribute(ATTR_REF))
            pParameter->WriteDeferredCleanup(pFile, pContext);
        else
            pParameter->WriteCleanup(pFile, pContext);
    }
}
