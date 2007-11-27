/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBENameFactory.cpp
 *  \brief   contains the implementation of the class CL4FiascoBENameFactory
 *
 *  \date    01/08/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4FiascoBENameFactory.h"
#include "be/BEClass.h"
#include "be/BETypedDeclarator.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4FiascoBENameFactory::CL4FiascoBENameFactory()
 : CL4BENameFactory()
{ }

/** \brief destructs this instance
 */
CL4FiascoBENameFactory::~CL4FiascoBENameFactory()
{ }

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param nSize the size of the type
 */
string CL4FiascoBENameFactory::GetTypeName(int nType, bool bUnsigned, int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_MSGTAG:
        sReturn = "l4_msgtag_t";
        break;
    default:
        break;
    }
    if (sReturn.empty())
        sReturn = CL4BENameFactory::GetTypeName(nType, bUnsigned, nSize);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CL4FiascoBENameFactory::%s Generated type name \"%s\" for type code %d\n",
	__func__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief returns the variable name for the msgtag return variable of an IPC invocation
 *  \return the name of the variable
 *
 * The generic L4 name factory has no idea what a message tag variable is.
 */
string CL4FiascoBENameFactory::GetMsgTagVariable()
{
    return string("_dice_tag");
}

/** \brief generates the variable of the client side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * Check for the timeout attribute and use the environment's timeout if given.
 * Otherwise construt the default client timeout.
 */
string CL4FiascoBENameFactory::GetTimeoutClientVariable(CBEFunction* pFunction)
{
	if (pFunction->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT))
		return string("L4_IPC_NEVER");

	return CL4BENameFactory::GetTimeoutClientVariable(pFunction);
}

/** \brief generates the variable of the component side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * Check for the timeout attribute and use the environment's timeout if given.
 * Otherwise construt the default server timeout.
 */
string CL4FiascoBENameFactory::GetTimeoutServerVariable(CBEFunction* pFunction)
{
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
	assert(pClass);

	if (pClass->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT))
		return string("L4_IPC_SEND_TIMEOUT_0");

	return CL4BENameFactory::GetTimeoutServerVariable(pFunction);
}

/** \brief general access function to generate strings
 *  \param nStringCode specifying the requested code
 *  \param pParam additional untyped parameter
 *  \return generated name
 *
 * This function multiplexes the request to the functions of this class'
 * implementation.
 */
string CL4FiascoBENameFactory::GetString(int nStringCode, void *pParam)
{
	switch (nStringCode)
	{
	case STR_UTCB_INITIALIZER:
		return GetUtcbInitializer(static_cast<CBEFunction*>(pParam));
		break;
	default:
		break;
	}
	return CL4BENameFactory::GetString(nStringCode, pParam);
}

/** \brief return the string to get the UTCB address
 *  \return string containing the getter
 */
string CL4FiascoBENameFactory::GetUtcbInitializer(CBEFunction *pFunction)
{
	CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
	CBEDeclarator *pEnvDecl = pEnv->m_Declarators.First();
	string sRet = pEnvDecl->GetName();
	if (pEnvDecl->GetStars())
		sRet += "->";
	else
		sRet += ".";
	sRet += "utcb->values";
	return sRet;
}

