/**
 *	\file	dice/src/be/l4/L4BETestsuite.cpp
 *	\brief	contains the implementation of the class CL4BETestsuite
 *
 *	\date	Tue May 28 2002
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

#include "be/l4/L4BETestsuite.h"
#include "be/BEImplementationFile.h"
#include "be/BEContext.h"

IMPLEMENT_DYNAMIC(CL4BETestsuite);

CL4BETestsuite::CL4BETestsuite()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BETestsuite, CBETestsuite);
}

/** destroys the testsuite class */
CL4BETestsuite::~CL4BETestsuite()
{
}

/** \brief creates the testsuite class
 *  \param pFEFile the respective front-end file
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * We only need to add an additional include file...
 */
bool CL4BETestsuite::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    if (!CBETestsuite::CreateBackEnd(pFEFile, pContext))
	{
        VERBOSE("%s failed because base testsuite could not be created\n", __PRETTY_FUNCTION__);
        return false;
	}
    // add include file
    VectorElement *pIter = GetFirstImplementationFile();
    CBEImplementationFile *pFile = GetNextImplementationFile(pIter);
    if (pFile)
        pFile->AddIncludedFileName(String("dice/dice-l4-testsuite.h"), false, false);

    return true;
}
