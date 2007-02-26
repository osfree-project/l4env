/**
 *	\file	dice/src/be/l4/L4BENameFactory.cpp
 *	\brief	contains the implementation of the class CL4BENameFactory
 *
 *	\date	02/07/2002
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

#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4BENameFactory);

CL4BENameFactory::CL4BENameFactory(bool bVerbose):CBENameFactory(bVerbose)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BENameFactory, CBENameFactory);
}

CL4BENameFactory::CL4BENameFactory(CL4BENameFactory & src):CBENameFactory(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BENameFactory, CBENameFactory);
}

/**	\brief the destructor of this class */
CL4BENameFactory::~CL4BENameFactory()
{

}

/**	\brief general access function to generate strings
 *	\param nStringCode specifying the requested code
 *	\param pContext the context of the name generation
 *	\param pParam additional untyped parameter
 *	\return generated name
 *
 * This function multiplexes the request to the functions of this class' implementation.
 */
String CL4BENameFactory::GetString(int nStringCode, CBEContext * pContext, void *pParam)
{
    switch (nStringCode)
    {
    case STR_RESULT_VAR:
        return GetResultName(pContext);
        break;
    case STR_THREAD_ID_VAR:
        return GetThreadIdVariable(pContext);
        break;
    case STR_INIT_RCVSTRING_FUNC:
        if (pParam)
            return GetInitRcvStringFunction(pContext, *(String*)pParam);
        else
            return GetInitRcvStringFunction(pContext, String());
        break;
    case STR_MSGBUF_SIZE_CONST:
	    if (pParam)
		    return GetMsgBufferSizeDopeConst((CBEMsgBufferType*)pParam);
        break;
    default:
        break;
    }
    return CBENameFactory::GetString(nStringCode, pContext, pParam);
}

/**	\brief generates the name of the result variable
 *	\param pContext the context of the code generation
 *	\return the name of the result variable
 */
String CL4BENameFactory::GetResultName(CBEContext * pContext)
{
    return String("_dice_result");
}

/**	\brief generates the variable of the client side timeout
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 *
 * At client side we always return the timeout member of the environment,
 * since this should be set by the user anyways.
 */
String CL4BENameFactory::GetTimeoutClientVariable(CBEContext * pContext)
{
    String sEnv = GetCorbaEnvironmentVariable(pContext);
    sEnv += "->timeout";
    return sEnv;
}

/**	\brief generates the variable of the component side timeout
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 *
 * We always use the environment's timeout member as well, because
 * we trust that it is set either by the user or per default by the
 * server-loop.
 */
String CL4BENameFactory::GetTimeoutServerVariable(CBEContext * pContext)
{
    String sEnv = GetCorbaEnvironmentVariable(pContext);
    sEnv += "->timeout";
    return sEnv;
}

/**	\brief generates the variable containing the component identifier
 *	\param pContext the context of the name generation
 *	\return  the name of the variable
 */
String CL4BENameFactory::GetComponentIDVariable(CBEContext * pContext)
{
    return GetCorbaObjectVariable(pContext);
}

/**	\brief generates a variable containing a l4thread_t
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CL4BENameFactory::GetThreadIdVariable(CBEContext * pContext)
{
    return String("_dice_thread_id");
}

/** \brief create L4 specific type names
 *  \param nType the type number
 *  \param bUnsigned true if the type is unsigned
 *  \param pContext the context of the name generation
 *  \param nSize the size of the type
 */
String CL4BENameFactory::GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize)
{
    String sReturn;
    if (pContext->IsOptionSet(PROGRAM_USE_L4TYPES))
        sReturn = GetL4TypeName(nType, bUnsigned, pContext, nSize);
    if (!sReturn.IsEmpty())
        return sReturn;
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
    default:
        sReturn = CBENameFactory::GetTypeName(nType, bUnsigned, pContext, nSize);
        break;
    }
    if (m_bVerbose)
        printf("CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
               __FUNCTION__, (const char *) sReturn, nType);
    return sReturn;
}

/** \brief generates the name of a message buffer member
 *  \param nFEType the type if the member
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
String CL4BENameFactory::GetMessageBufferMember(int nFEType, CBEContext * pContext)
{
    String sReturn;
    switch(nFEType)
    {
    case TYPE_RCV_FLEXPAGE:
        sReturn = "fpage";
        break;
    case TYPE_MSGDOPE_SIZE:
        sReturn = "size";
        break;
    case TYPE_MSGDOPE_SEND:
        sReturn = "send";
        break;
    default:
        sReturn = CBENameFactory::GetMessageBufferMember(nFEType, pContext);
        break;
    }
    return sReturn;
}

/** \brief create specific L4 type names
 *  \param nType the type to create the name for
 *  \param bUnsigned true if the type is unsigned
 *  \param pContext the context of the creation process
 *  \param nSize the size of the type in bytes
 *  \return the L4 type name
 */
String CL4BENameFactory::GetL4TypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize)
{
    String sReturn;
    switch (nType)
    {
    case TYPE_INTEGER:
	case TYPE_LONG:
        switch (nSize)
        {
        case 1:
            if (bUnsigned)
                sReturn = "l4_uint8_t";
            else
                sReturn = "l4_int8_t";
            break;
        case 2:
            if (bUnsigned)
                sReturn = "l4_uint16_t";
            else
                sReturn = "l4_int16_t";
            break;
        case 4:
            if (bUnsigned)
                sReturn = "l4_uint32_t";
            else
                sReturn = "l4_int32_t";
            break;
        case 8:
            if (bUnsigned)
                sReturn = "l4_uint64_t";
            else
                sReturn = "l4_int64_t";
            break;
        }
        break;
    case TYPE_CHAR:
        if (bUnsigned)
            sReturn = "l4_uint8_t";
        else
            sReturn = "l4_int8_t";
        break;
    case TYPE_BYTE:
        sReturn = "l4_uint8_t";
        break;
    }

    if (m_bVerbose)
        printf("CL4BENameFactory::%s Generated type name \"%s\" for type code %d\n",
               __FUNCTION__, (const char *) sReturn, nType);
    return sReturn;
}

/** \brief create specific C style names
 *  \param nType the type to create the name for
 *  \param bUnsigned true if this type is unsigned
 *  \param pContext the context of the name creation
 *  \param nSize the size of the type
 *  \return the name of the type
 *
 * This implementation simply tests if the wanted C type can also be an L4 type
 */
String CL4BENameFactory::GetCTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize)
{
    String sReturn;
    if (pContext->IsOptionSet(PROGRAM_USE_L4TYPES))
        sReturn = GetL4TypeName(nType, bUnsigned, pContext, nSize);
    if (!sReturn.IsEmpty())
        return sReturn;
    return CBENameFactory::GetCTypeName(nType, bUnsigned, pContext, nSize);
}

/** \brief retrieves the name of an init function for receive strings
 *  \param pContext the context of the name creation
 *  \param sFuncName the function-name given by the user
 *  \return the name of the function
 *
 * The name is either given by the user, or we use the default name.
 *
 * (We might obfusicate the name to differentiate between client and server
 * side, but basically does it not matter, because the two implementation are
 * in different object-files and binaries)
 */
String CL4BENameFactory::GetInitRcvStringFunction(CBEContext *pContext, String sFuncName)
{
    if (sFuncName.IsEmpty())
    {
        if (pContext->GetInitRcvStringFunc().IsEmpty())
            return String("dice_init_rcvstring");
        return pContext->GetInitRcvStringFunc();
    }
    return sFuncName;
}

/** \brief returns the name of the constant declaring the size dope of the message buffer
 *  \param pMsgBuffer the message buffer to write the const for
 *  \return the name of the appropriate constant
 */
String CL4BENameFactory::GetMsgBufferSizeDopeConst(CBEMsgBufferType* pMsgBuffer)
{
    String sName;
    if (pMsgBuffer && pMsgBuffer->GetAlias())
	{
		// use the msg buffer's alias and attach a "_SIZE_DOPE"
		sName = pMsgBuffer->GetAlias()->GetName();
		sName.MakeUpper();
		sName += "_SIZE_INIT";
	}
	return sName;
}
