/**
 *  \file   dice/src/be/cdr/CCDRClassFactory.cpp
 *  \brief  contains the implementation of the class CCDRClassFactory
 *
 *  \date   02/10/2003
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

#include "be/cdr/CCDRClassFactory.h"
#include "be/cdr/CCDRClient.h"
#include "be/cdr/CCDRClass.h"
#include "be/cdr/CCDRComponentFunction.h"
#include "be/cdr/CCDRMarshalFunction.h"
#include "be/cdr/CCDRUnmarshalFunction.h"

CCDRClassFactory::CCDRClassFactory(bool bVerbose)
 : CBEClassFactory(bVerbose)
{
}

/** destroys this object */
CCDRClassFactory::~CCDRClassFactory()
{
}

/** \brief creates a new object of the class CBEClient
 *  \return a reference to the new instance
 */
CBEClient* CCDRClassFactory::GetNewClient()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CCDRClient\n");
    return new CCDRClient();
}

/** \brief creates a new object of the class CBEClass
 *  \return a reference to the new instance
 */
CBEClass* CCDRClassFactory::GetNewClass()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CCDRClass\n");
    return new CCDRClass();
}

/** \brief creates a new object of the class CBEComponentFunction
 *  \return a reference to the new instance
 */
CBEComponentFunction* CCDRClassFactory::GetNewComponentFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CCDRComponentFunction\n");
    return new CCDRComponentFunction();
}

/** \brief creates a new object of the class CBEMarshalFunction
 *  \return a reference to the new instance
 */
CBEMarshalFunction* CCDRClassFactory::GetNewMarshalFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CCDRMarshalFunction\n");
    return new CCDRMarshalFunction();
}

/** \brief creates a new object of the class CBEUnmarshalFunction
 *  \return a reference to the new instance
 */
CBEUnmarshalFunction* CCDRClassFactory::GetNewUnmarshalFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CCDRUnmarshalFunction\n");
    return new CCDRUnmarshalFunction();
}
