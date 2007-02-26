/**
 *	\file	dice/src/be/l4/L4BEClass.cpp
 *	\brief	contains the implementation of the class CL4BEClass
 *
 *	\date	01/29/2003
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

#include "be/l4/L4BEClass.h"
#include "be/BEAttribute.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/l4/L4BENameFactory.h"

#include "fe/FEAttribute.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4BEClass);

CL4BEClass::CL4BEClass()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEClass, CBEClass);
}

CL4BEClass::~CL4BEClass()
{
}

/** \brief write L4 specific helper functions
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We have to write the function declaration for user-defined functions.
 * - default_function: is written by server-loop, since it is used there
 * - init-rcvstring: void name(int, l4_umword_t*, l4_umword_t*, CORBA_Environment)
 * - error-function: void name(l4_msgdope_t)
 */
void CL4BEClass::WriteFunctions(CBEHeaderFile * pFile, CBEContext * pContext)
{
    // init-rcvstring
    if (FindAttribute(ATTR_INIT_RCVSTRING))
    {
        CBEAttribute *pAttr = FindAttribute(ATTR_INIT_RCVSTRING);
        String sFuncName = pAttr->GetString();
        if (sFuncName.IsEmpty())
            sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext);
        else
            sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext, (void*)&sFuncName);
        String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
        pFile->PrintIndent("void %s(int, %s*, %s*, CORBA_Environment*);\n\n", (const char*)sFuncName,
                (const char*)sMWord, (const char*)sMWord);
    }
    // error-function
    if (FindAttribute(ATTR_ERROR_FUNCTION))
    {
        CBEAttribute *pAttr = FindAttribute(ATTR_ERROR_FUNCTION);
        String sFuncName = pAttr->GetString();
        pFile->PrintIndent("void %s(l4_msgdope_t);\n", (const char*)sFuncName);
    }
    // call base class
    CBEClass::WriteFunctions(pFile, pContext);
}
