/**
 * \file    dice/src/be/l4/L4BENameFactory.cpp
 * \brief   contains the implementation of the class CL4BENameFactory
 *
 * \date    02/07/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "TypeSpec-L4Types.h"
#include "Compiler.h"
#include <iostream>
#include <cctype>
#include <algorithm>

CL4BENameFactory::CL4BENameFactory()
: CBENameFactory()
{
}

/** \brief the destructor of this class */
CL4BENameFactory::~CL4BENameFactory()
{

}

/** \brief general access function to generate strings
 *  \param nStringCode specifying the requested code
 *  \param pParam additional untyped parameter
 *  \return generated name
 *
 * This function multiplexes the request to the functions of this class'
 * implementation.
 */
string
CL4BENameFactory::GetString(int nStringCode,
    void *pParam)
{
    switch (nStringCode)
    {
    case STR_RESULT_VAR:
        return GetResultName();
        break;
    case STR_THREAD_ID_VAR:
        return GetThreadIdVariable();
        break;
    case STR_INIT_RCVSTRING_FUNC:
        if (pParam)
            return GetInitRcvStringFunction(*(string*)pParam);
        else
            return GetInitRcvStringFunction(string());
        break;
    case STR_MSGBUF_SIZE_CONST:
        if (pParam)
            return GetMsgBufferSizeDopeConst((CBETypedDeclarator*)pParam);
        return string();
        break;
    case STR_ZERO_FPAGE:
	return GetZeroFpage();
	break;
    default:
        break;
    }
    return CBENameFactory::GetString(nStringCode, pParam);
}

/** \brief generates the name of the result variable
 *  \return the name of the result variable
 */
string
CL4BENameFactory::GetResultName()
{
    return string("_dice_result");
}

/** \brief generated the name for the zero flexpage of the message buffer
 *  \return name of zero fpage member
 */
string
CL4BENameFactory::GetZeroFpage()
{
    return string("_zero_fpage");
}

/** \brief generates the variable of the client side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * At client side we always return the timeout member of the environment,
 * since this should be set by the user anyways.
 */
string
CL4BENameFactory::GetTimeoutClientVariable(CBEFunction* /* pFunction */)
{
    string sEnv = GetCorbaEnvironmentVariable();
    sEnv += "->timeout";
    return sEnv;
}

/** \brief generates the variable of the component side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 *
 * We always use the environment's timeout member as well, because we trust
 * that it is set either by the user or per default by the server-loop.
 */
string
CL4BENameFactory::GetTimeoutServerVariable(CBEFunction* /* pFunction */)
{
    string sEnv = GetCorbaEnvironmentVariable();
    sEnv += "->timeout";
    return sEnv;
}

/** \brief generates the variable of the client side scheduling parameter
 *  \return the name of the variable
 *
 * At client side we always return the scheduling member of the environment,
 * since this should be set by the user anyways.
 */
string
CL4BENameFactory::GetScheduleClientVariable()
{
    string sEnv = GetCorbaEnvironmentVariable();
    sEnv += "->_p.sched_bits";
    return sEnv;
}

/** \brief generates the variable of the server side scheduling parameter
 *  \return the name of the variable
 *
 * At client side we always return the scheduling member of the environment,
 * since this should be set by the user anyways.
 */
string
CL4BENameFactory::GetScheduleServerVariable()
{
    string sEnv = GetCorbaEnvironmentVariable();
    sEnv += "->_p.sched_bits";
    return sEnv;
}

/** \brief generates the variable to access the partner element of env
 *  \return string containing that var
 */
string CL4BENameFactory::GetPartnerVariable()
{
    string sEnv = GetCorbaEnvironmentVariable();
    sEnv += "->partner";
    return sEnv;
}

/** \brief generates the variable containing the component identifier
 *  \return  the name of the variable
 */
string
CL4BENameFactory::GetComponentIDVariable()
{
    return GetCorbaObjectVariable();
}

/** \brief generates a variable containing a l4thread_t
 *  \return the name of the variable
 */
string
CL4BENameFactory::GetThreadIdVariable()
{
    return string("_dice_thread_id");
}

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param nSize the size of the type
 */
string
CL4BENameFactory::GetTypeName(int nType, 
    bool bUnsigned,
    int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_FLEXPAGE:
        sReturn = "l4_snd_fpage_t";
        break;
    case TYPE_RCV_FLEXPAGE:
        sReturn = "l4_fpage_t";
        break;
    case TYPE_MWORD:
        if (bUnsigned)
            sReturn = "l4_umword_t";
        else
            sReturn = "l4_mword_t";
        break;
    case TYPE_MSGDOPE_SEND:
    case TYPE_MSGDOPE_SIZE:
        sReturn = "l4_msgdope_t";
        break;
    case TYPE_REFSTRING:
        sReturn = "l4_strdope_t";
        break;
    default:
        sReturn = 
	    CBENameFactory::GetTypeName(nType, bUnsigned, nSize);
        break;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
	__func__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief generates the name of a message buffer member
 *  \param nFEType the type if the member
 *  \return the name of the variable
 */
string
CL4BENameFactory::GetMessageBufferMember(int nFEType)
{
    string sReturn;
    switch(nFEType)
    {
    case TYPE_RCV_FLEXPAGE:
        sReturn = "_dice_rcv_fpage";
        break;
    case TYPE_MSGDOPE_SIZE:
        sReturn = "_dice_size_dope";
        break;
    case TYPE_MSGDOPE_SEND:
        sReturn = "_dice_send_dope";
        break;
    default:
        sReturn = CBENameFactory::GetMessageBufferMember(nFEType);
        break;
    }
    return sReturn;
}

/** \brief retrieves the name of an init function for receive strings
 *  \param sFuncName the function-name given by the user
 *  \return the name of the function
 *
 * The name is either given by the user, or we use the default name.
 *
 * (We might obfusicate the name to differentiate between client and server
 * side, but basically does it not matter, because the two implementation are
 * in different object-files and binaries)
 */
string
CL4BENameFactory::GetInitRcvStringFunction(string sFuncName)
{
    if (sFuncName.empty())
    {
        if (CCompiler::GetInitRcvStringFunc().empty())
            return string("dice_init_rcvstring");
        return CCompiler::GetInitRcvStringFunc();
    }
    return sFuncName;
}

/** \brief returns the name of the constant declaring the size dope of the \
 *         message buffer
 *  \param pMsgBuffer the message buffer to write the const for
 *  \return the name of the appropriate constant
 */
string 
CL4BENameFactory::GetMsgBufferSizeDopeConst(CBETypedDeclarator* pMsgBuffer)
{
    string sName;
    if (pMsgBuffer && pMsgBuffer->m_Declarators.First())
    {
//         use the msg buffer's alias and attach a "_SIZE_INIT"
        sName = pMsgBuffer->m_Declarators.First()->GetName();
        transform(sName.begin(), sName.end(), sName.begin(), _toupper);
        sName += "_SIZE_INIT";
    }
    return sName;
}

/** \brief get the name for the padding member
 *  \param nPadType the type of the padding member
 *  \param nPadToType the type of the member to insert the padding before
 *  \return the name of this member
 */
string
CL4BENameFactory::GetPaddingMember(int nPadType, 
    int nPadToType)
{
    string sReturn = string("_dice_pad_");
    // strip leading l4_ and trainling _t's
    string sType = StripL4Fixes(GetTypeName(nPadToType, false));
    sReturn += sType;
    sReturn += "_with_";
    sType = StripL4Fixes(GetTypeName(nPadType, false));
    sReturn += sType;
    // replace spaces in type names
    replace_if(sReturn.begin(), sReturn.end(), ::isspace, '_');
    return sReturn;
}

/** \brief strip leading l4_ prefix and trailing _t suffix
 *  \param sName the string to strip
 *  \return the stripped string or the input if nothing stripped
 */
string
CL4BENameFactory::StripL4Fixes(string sName)
{
    if (sName.substr(0, 3) == "l4_")
	sName = sName.substr(3);
    if (sName.substr(sName.length()-2) == "_t")
	sName = sName.substr(0, sName.length() - 2);
    return sName;
}
