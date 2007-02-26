/**
 *    \file    dice/src/be/BENameFactory.cpp
 * \brief   contains the implementation of the class CBENameFactory
 *
 *    \date    01/10/2002
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

#include "be/BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEConstDeclarator.h"

#include "TypeSpec-Type.h"
#include <typeinfo>
#include <algorithm>
#include <ctype.h>
using namespace std;

CBENameFactory::CBENameFactory(bool bVerbose)
{
    m_bVerbose = bVerbose;
}

CBENameFactory::CBENameFactory(CBENameFactory & src)
: CBEObject(src)
{
    m_bVerbose = src.m_bVerbose;
}

/** \brief the destructor of this class */
CBENameFactory::~CBENameFactory()
{

}

/** \brief creates the file-name for a specific file type
 *  \param pFEBase a reference for the front-end class, which defines granularity (IDLFILE, MODULE, INTERFACE, OPERATION)
 *  \param pContext the context of the generated name
 *  \return a reference to the created name or 0 if some error occured
 *
 * We have several file types: we have one header file and opcode file per front-end IDL file.
 * There may be several implementation files for a front-end IDL file.
 * E.g. one per IDL file, one per module, one per interface or one per function.
 */
string CBENameFactory::GetFileName(CFEBase * pFEBase, CBEContext * pContext)
{
    if (m_bVerbose)
        printf("CBENameFactory::GetFileName\n");

    if (!pFEBase)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetFileName failed because front-end class is 0\n");
        return string();
    }

    string sReturn;
    string sPrefix = pContext->GetFilePrefix();
    int nFileType = pContext->GetFileType();

    // first check non-IDL files
    if (dynamic_cast<CFEFile*>(pFEBase))
    {
        if (!((CFEFile *) pFEBase)->IsIDLFile())
        {
            // get file-name
            sReturn = sPrefix;
            sReturn += ((CFEFile *) pFEBase)->GetFileName();
            // deliver filename
            if (m_bVerbose)
                printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s (!IDL file)\n",
                        typeid(*pFEBase).name(), nFileType, sReturn.c_str());

            return sReturn;
        }
    }
    // test for header files
    if ((nFileType == FILETYPE_CLIENTHEADER) ||
        (nFileType == FILETYPE_COMPONENTHEADER) ||
        (nFileType == FILETYPE_OPCODE) ||
        (nFileType == FILETYPE_TESTSUITE) ||
        (nFileType == FILETYPE_TEMPLATE) ||
        (nFileType == FILETYPE_COMPONENTIMPLEMENTATION))
    {
        // should only be files
        if (!dynamic_cast<CFEFile*>(pFEBase))
        {
            if (m_bVerbose)
                printf("%s failed because filetype required CFEFile  wasn't\n",
		    __PRETTY_FUNCTION__);
            return string();
        }
        // assemble string
        sReturn = sPrefix;
        sReturn += ((CFEFile *) pFEBase)->GetFileNameWithoutExtension();
        switch (nFileType)
        {
        case FILETYPE_CLIENTHEADER:
            sReturn += "-client";
            break;
        case FILETYPE_COMPONENTHEADER:
            sReturn += "-server";
            break;
        case FILETYPE_COMPONENTIMPLEMENTATION:
            sReturn += "-server";
            break;
        case FILETYPE_OPCODE:
            sReturn += "-sys";
            break;
        case FILETYPE_TESTSUITE:
            sReturn += "-testsuite";
            break;
        case FILETYPE_TEMPLATE:
            sReturn += "-template";
            break;
        default:
            break;
        }
	// add extension
	// FIXME: use extra function to overload
	switch (nFileType)
	{
	case FILETYPE_CLIENTHEADER:
	case FILETYPE_COMPONENTHEADER:
	case FILETYPE_OPCODE:
	    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
		sReturn += ".hh";
	    else
		sReturn += ".h";
	    break;
	case FILETYPE_COMPONENTIMPLEMENTATION:
	case FILETYPE_TESTSUITE:
	case FILETYPE_TEMPLATE:
	    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
		sReturn += ".cc";
	    else
		sReturn += ".c";
	    break;
	default:
	    break;
	}
        if (m_bVerbose)
            printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s (header, opcode, testuite)\n",
                typeid(*pFEBase).name(), nFileType, sReturn.c_str());
        return sReturn;
    }


    if (pContext->IsOptionSet(PROGRAM_FILE_IDLFILE) ||
        pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        // filename := [&lt;prefix&gt;]&lt;IDL-file-name&gt;-(client|server|testsuite|opcode).c
        // check FE type
        if (!dynamic_cast<CFEFile*>(pFEBase))
        {
            if (m_bVerbose)
                printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_IDLFILE/ALL and not CFEFile\n");
            return string();
        }
        // get FE file
        CFEFile *pFEFile = (CFEFile *) pFEBase;
        // get file-name
        sReturn = sPrefix;
        sReturn += pFEFile->GetFileNameWithoutExtension();
        sReturn += "-client";
	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    	    sReturn += ".cc";
	else
	    sReturn += ".c";
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_MODULE))
    {
        // filename := [&lt;prefix&gt;]&lt;libname&gt;-(client|server).c
        // check FE type
        // can also be interface (if FILE_MODULE, but top level interface)
        if (dynamic_cast<CFEInterface*>(pFEBase))
        {
            pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_MODULE);
            string sReturn = GetFileName(pFEBase, pContext);
            pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_INTERFACE);
            return sReturn;
        }
        // else if no lib: return 0
        if (!dynamic_cast<CFELibrary*>(pFEBase))
        {
            if (m_bVerbose)
                printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_MODULE and not CFELibrary\n");
            return string();
        }
        // get libname-name
        sReturn = sPrefix;
        CFELibrary *pFELibrary = (CFELibrary *) pFEBase;
        // always prefix with IDL filename
        sReturn += pFELibrary->GetSpecificParent<CFEFile>(0)->GetFileNameWithoutExtension();
        sReturn += "-";
        sReturn += pFELibrary->GetName();
        sReturn += "-client";
	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
	    sReturn += ".cc";
	else
    	    sReturn += ".c";
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
    {
        // filename := [&lt;prefix&gt;][&lt;libname&gt;_]&lt;interfacename&gt;-(client|server).c
        // can also be library (if FILE_INTERFACE, but library with types or constants)
        if (dynamic_cast<CFELibrary*>(pFEBase))
        {
            pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_INTERFACE);
            string sReturn = GetFileName(pFEBase, pContext);
            pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_MODULE);
            return sReturn;
        }
        // check FE type
        if (!dynamic_cast<CFEInterface*>(pFEBase))
        {
            if (m_bVerbose)
                printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_INTERFACE and not CFEInterface\n");
            return string();
        }
        // get interface name
        sReturn = sPrefix;
        CFEInterface *pFEInterface = (CFEInterface *) pFEBase;
        // always prefix with IDL filename
        sReturn += pFEInterface->GetSpecificParent<CFEFile>(0)->GetFileNameWithoutExtension();
        sReturn += "-";
        string sLibs;
        CFELibrary *pFELibrary = pFEInterface->GetSpecificParent<CFELibrary>();
        while (pFELibrary)
        {
            sLibs = pFELibrary->GetName() + "_" + sLibs;
            pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
        }
        sReturn += sLibs;
        sReturn += pFEInterface->GetName();
        sReturn += "-client";
	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
	    sReturn += ".cc";
	else
    	    sReturn += ".c";
    }
    else if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION))
    {
        // filename := [&lt;prefix&gt;][&lt;libname&gt;_][&lt;interfacename&gt;_]&lt;funcname&gt;-(client|server).c
        // can also be library (if contains types or constants)
        if (dynamic_cast<CFELibrary*>(pFEBase))
        {
            pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_FUNCTION);
            string sReturn = GetFileName(pFEBase, pContext);
            pContext->ModifyOptions(PROGRAM_FILE_FUNCTION, PROGRAM_FILE_MODULE);
            return sReturn;
        }
        // can also be interface (if contains types or constants)
        if (dynamic_cast<CFEInterface*>(pFEBase))
        {
            pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_FUNCTION);
            string sReturn = GetFileName(pFEBase, pContext);
            pContext->ModifyOptions(PROGRAM_FILE_FUNCTION, PROGRAM_FILE_INTERFACE);
            return sReturn;
        }
        // check FE type
        if (!dynamic_cast<CFEOperation*>(pFEBase))
        {
            if (m_bVerbose)
                printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_FUNCTION and not CFEOperation\n");
            return string();
        }
        // get class
        sReturn = sPrefix;
        CFEOperation *pFEOperation = (CFEOperation *) pFEBase;
        // always prefix with IDL filename
        sReturn += pFEOperation->GetSpecificParent<CFEFile>(0)->GetFileNameWithoutExtension();
        sReturn += "-";
        string sLibs;
        CFELibrary *pFELibrary = pFEOperation->GetSpecificParent<CFELibrary>();
        while (pFELibrary)
        {
            sLibs = pFELibrary->GetName() + "_" + sLibs;
            pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
        }
        sReturn += sLibs;
        if (pFEOperation->GetSpecificParent<CFEInterface>())
        {
            sReturn += pFEOperation->GetSpecificParent<CFEInterface>()->GetName();
            sReturn += "_";
        }
        // get operation's name
        sReturn += pFEOperation->GetName();
        sReturn += "-client";
	if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
	    sReturn += ".cc";
	else
    	    sReturn += ".c";
    }
    if (m_bVerbose)
        printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s\n",
            typeid(*pFEBase).name(), nFileType, sReturn.c_str());
    return sReturn;
}

/** \brief get the file name used in an include statement
 *  \param pFEBase the front-end class used to derive the name from
 *  \param pContext the context of this create process
 *  \return a file name suitable for an include statement
 *
 * A file-name suitable for an include statement contains the original relative
 * path used when the file was included into the idl file. If the original file
 * was the top file, there is no difference to the GetFileName function.
 */
string CBENameFactory::GetIncludeFileName(CFEBase * pFEBase, CBEContext * pContext)
{
    // first get the file name as usual
    // adds prefix to non-IDL files
    string sName = GetFileName(pFEBase, pContext);
    // extract the relative path from the original FE file
    CFEFile *pFEFile = pFEBase->GetSpecificParent<CFEFile>(0);
    // if no IDL file, return original name
    string sOriginalName = pFEFile->GetFileName();
    if (!pFEFile->IsIDLFile())
        return sName;
    // get file name (which contains relative path) and extract it
    // it is everything up to the last '/'
    int nPos = sOriginalName.rfind('/');
    string sPath;
    if (nPos > 0)
        sPath = sOriginalName.substr(0, nPos + 1);
    // concat path and name and return
    return sPath + sName;
}

/** \brief gets the filename used in an include statement if no FE class available
 *  \param sBaseName the name of the original include statement
 *  \param pContext the context of this operation
 *  \return the new name in the include statement
 *
 * We treat the file as non-IDL file. Otherwise there would have been a FE class
 * to use as reference.
 */
string CBENameFactory::GetIncludeFileName(string sBaseName, CBEContext* pContext)
{
    // get file-name
    string sReturn = pContext->GetFilePrefix() + sBaseName;
    // deliver filename
    if (m_bVerbose)
        printf("CBENameFactory::GetFileName(filetype:%d) = %s (!IDL file)\n",
                pContext->GetFileType(), sReturn.c_str());

    return sReturn;
}

/** \brief creates the name of a type
 *  \param nType the type of the type (char, int, bool, ...)
 *  \param bUnsigned true if the wished type name should be for an unsigned type
 *  \param pContext the context of the code generation
 *  \param nSize the size of the type (long int, ...)
 *  \return the string describing the type
 *
 * This function returns the C representations of the given types.
 */
string CBENameFactory::GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize)
{
    string sReturn;
    if (pContext->IsOptionSet(PROGRAM_USE_CTYPES))
        sReturn = GetCTypeName(nType, bUnsigned, pContext, nSize);
    if (!sReturn.empty())
        return sReturn;
    switch (nType)
    {
    case TYPE_NONE:
        sReturn = "_UNDEFINED_";
        break;
    case TYPE_INTEGER:
    case TYPE_LONG:
        switch (nSize)
        {
        case 1:
            sReturn = "CORBA_small";
            break;
        case 2:
            if (bUnsigned)
                sReturn = "CORBA_unsigned_short";
            else
                sReturn = "CORBA_short";
            break;
        case 4:
            if (bUnsigned)
                sReturn = "CORBA_unsigned_";
            else
                sReturn = "CORBA_";
            if (nType == TYPE_LONG)
                sReturn += "long";
            else
                sReturn += "int";
            break;
        case 8:
            if (bUnsigned)
                sReturn = "CORBA_unsigned_long_long";
            else
                sReturn = "CORBA_long_long";
            break;
        }
        break;
    case TYPE_MWORD:
        sReturn = "CORBA_unsigned_long";
        break;
    case TYPE_VOID:
        sReturn = "CORBA_void";
        break;
    case TYPE_FLOAT:
        sReturn = "CORBA_float";
        break;
    case TYPE_DOUBLE:
        sReturn = "CORBA_double";
        break;
    case TYPE_LONG_DOUBLE:
        sReturn = "CORBA_long_double";
        break;
    case TYPE_CHAR:
        if (bUnsigned)
            sReturn = "CORBA_unsigned_char";
        else
            sReturn = "CORBA_char";
        break;
    case TYPE_STRING:
        sReturn = "CORBA_char";
        break;
    case TYPE_WCHAR:
    case TYPE_WSTRING:
        sReturn = "CORBA_wchar";
        break;
    case TYPE_BOOLEAN:
        sReturn = "CORBA_boolean";
        break;
    case TYPE_BYTE:
        sReturn = "CORBA_byte";
        break;
    case TYPE_VOID_ASTERISK:
        sReturn = "CORBA_void_ptr";
        break;
    case TYPE_CHAR_ASTERISK:
        sReturn = "CORBA_char_ptr";
        break;
    case TYPE_ARRAY:
    case TYPE_STRUCT:
    case TYPE_TAGGED_STRUCT:
        sReturn = "struct";
        break;
    case TYPE_UNION:
    case TYPE_TAGGED_UNION:
        sReturn = "union";
        break;
    case TYPE_ENUM:
    case TYPE_TAGGED_ENUM:
        sReturn = "enum";
        break;
    case TYPE_PIPE:
        sReturn = "_UNDEFINED_1";
        break;
    case TYPE_HANDLE_T:
        sReturn = "handle_t";
        break;
    case TYPE_OCTET:        // 8-bit type, which will never be converted
        sReturn = "CORBA_char";
        break;
    case TYPE_ANY:
    case TYPE_OBJECT:
    case TYPE_ISO_LATIN_1:
    case TYPE_ISO_MULTILINGUAL:
    case TYPE_ISO_UCS:
    case TYPE_ERROR_STATUS_T:
        sReturn = "_UNDEFINED_2";
        break;
    case TYPE_USER_DEFINED:
        sReturn.erase(sReturn.begin(), sReturn.end());
        break;
    default:
        sReturn = "_UNDEFINED_def";
        break;
    }

    if (m_bVerbose)
        printf("CBENameFactory::%s Generated type name \"%s\" for type code %d\n",
            __FUNCTION__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief creates the C-style name of a type
 *  \param nType the type of the type (char, int, bool, ...)
 *  \param bUnsigned true if the wished type name should be for an unsigned type
 *  \param pContext the context of the code generation
 *  \param nSize the size of the type (long int, ...)
 *  \return the string describing the type
 *
 * This function skips types, which it won't provide names for. This way the GetTypeName
 * function will set the name for it.
 */
string CBENameFactory::GetCTypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize)
{
    string sReturn;
    switch (nType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
        switch (nSize)
        {
        case 1:
            sReturn = "unsigned char";
            break;
        case 2:
            if (bUnsigned)
                sReturn = "unsigned short";
            else
                sReturn = "short";
            break;
        case 4:
            if (bUnsigned)
                sReturn = "unsigned ";
            if (nType == TYPE_LONG)
                sReturn += "long";
            else
                sReturn += "int";
            break;
        case 8:
#if SIZEOF_LONG_LONG > 0
            if (bUnsigned)
                sReturn = "unsigned long long";
            else
                sReturn = "long long";
#else
            if (bUnsigned)
                sReturn = "unsigned long";
            else
                sReturn = "long";
#endif
            break;
        }
        break;
    case TYPE_VOID:
        sReturn = "void";
        break;
    case TYPE_FLOAT:
        sReturn = "float";
        break;
    case TYPE_DOUBLE:
        sReturn = "double";
        break;
    case TYPE_LONG_DOUBLE:
        sReturn = "long double";
        break;
    case TYPE_CHAR:
        if (bUnsigned)
            sReturn = "unsigned char";
        else
            sReturn = "char";
        break;
    case TYPE_STRING:
        sReturn = "char";
        break;
    case TYPE_BYTE:
        sReturn = "unsigned char";
        break;
    case TYPE_VOID_ASTERISK:
        sReturn = "void*";
        break;
    case TYPE_CHAR_ASTERISK:
        sReturn = "char*";
        break;
    case TYPE_STRUCT:
    case TYPE_TAGGED_STRUCT:
        sReturn = "struct";
        break;
    case TYPE_UNION:
    case TYPE_TAGGED_UNION:
        sReturn = "union";
        break;
    case TYPE_ENUM:
    case TYPE_TAGGED_ENUM:
        sReturn = "enum";
        break;
    default:
        sReturn.erase(sReturn.begin(), sReturn.end());
        break;
    }
    if (m_bVerbose)
        printf("CBENameFactory::%s Generated type name \"%s\" for type code %d\n",
               __FUNCTION__, sReturn.c_str(), nType);
    return sReturn;
}

/** \brief creates the name of a function
 *  \param pFEOperation the function to create a name for
 *  \param pContext the context of the name generation
 *  \return the name of the back-end function
 *
 * The name has to be unique, therefore it is build using parent interfaces
 * and libraries.  The name looks like this:
 *
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;operation name&gt;
 *
 * The function type determines the ending:
 * - FUNCTION_SEND: "_send"
 * - FUNCTION_RECV: "_recv"
 * - FUNCTION_WAIT: "_wait"
 * - FUNCTION_UNMARSHAL:  "_unmarshal"   (10)
 * - FUNCTION_MARSHAL:  "_marshal"       (8)
 * - FUNCTION_REPLY_RECV: "_reply_recv"  (11)
 * - FUNCTION_REPLY_WAIT: "_reply_wait"  (11)
 * - FUNCTION_CALL: "_call"
 * - FUNCTION_SWITCH_CASE: "_call"
 * - FUNCTION_TEMPLATE:   "_component"   (10)
 *
 * The three other function types should not be used with this implementation,
 * because these are interface functions.  If they are (accidentally) used
 * here, we redirect the call to the interface function naming implementation.
 * - FUNCTION_WAIT_ANY:   "_wait_any"    (9)
 * - FUNCTION_RECV_ANY:   "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP:   "_server_loop" (12)
 * - FUNCTION_DISPATCH:   "_dispatch"    (9)
 *
 * \todo if nested library use all lib names
 */
string 
CBENameFactory::GetFunctionName(CFEOperation * pFEOperation, 
    CBEContext * pContext)
{
    if (!pFEOperation)
    {
        if (m_bVerbose)
            printf("%s failed because the operation is 0\n",
		__PRETTY_FUNCTION__);
        return string();
    }
    // get interface and library for operation
    CFEInterface *pFEInterface = 
	pFEOperation->GetSpecificParent<CFEInterface>();
    CFELibrary *pFELibrary = pFEOperation->GetSpecificParent<CFELibrary>();
    // get file type
    int nFunctionType = pContext->GetFunctionType() & FUNCTION_NOTESTFUNCTION;
    // check for interface functions
    if ((nFunctionType == FUNCTION_WAIT_ANY) ||
        (nFunctionType == FUNCTION_RECV_ANY) ||
        (nFunctionType == FUNCTION_SRV_LOOP) ||
        (nFunctionType == FUNCTION_DISPATCH) ||
        (nFunctionType == FUNCTION_REPLY_ANY_WAIT_ANY))
        return GetFunctionName(pFEInterface, pContext);

    string sReturn;
    if ((pContext->GetFunctionType() & FUNCTION_TESTFUNCTION) > 0)
        sReturn += "test_";
    if (pContext->IsBackEndSet(PROGRAM_BE_C))
    {
	if (pFELibrary)
	    sReturn += pFELibrary->GetName() + "_";
	if (pFEInterface)
	    sReturn += pFEInterface->GetName() + "_";
    }
    sReturn += pFEOperation->GetName();
    switch (nFunctionType)
    {
    case FUNCTION_SEND:
        sReturn += "_send";
        break;
    case FUNCTION_RECV:
        sReturn += "_recv";
        break;
    case FUNCTION_WAIT:
        sReturn += "_wait";
        break;
    case FUNCTION_CALL:
    case FUNCTION_SWITCH_CASE:
	if (pContext->IsBackEndSet(PROGRAM_BE_C))
	    sReturn += "_call";
        break;
    case FUNCTION_UNMARSHAL:
        sReturn += "_unmarshal";
        break;
    case FUNCTION_MARSHAL:
        sReturn += "_marshal";
        break;
    case FUNCTION_TEMPLATE:
	if (pContext->IsBackEndSet(PROGRAM_BE_C))
	    sReturn += "_component";
        break;
    case FUNCTION_REPLY_RECV:
        sReturn += "_reply_recv";
        break;
    case FUNCTION_REPLY_WAIT:
        sReturn += "_reply_wait";
        break;
    case FUNCTION_REPLY:
        sReturn += "_reply";
        break;
    default:
        break;
    }

    if (m_bVerbose)
        printf("CBENameFactory::GetFunctionName(%s, functiontype:%d) = %s\n",
               pFEOperation->GetName().c_str(), nFunctionType,
               sReturn.c_str());
    return sReturn;
}

/** \brief creates the name of a function
 *  \param pFEInterface the interface to create a name for
 *  \param pContext the context of the name generat
 *  \return the name of the interface's function
 *
 * This implementation creates function names for interface functions. The
 * name looks like this:
 *
 * [&lt;library name&gt;_]&lt;interface name&gt;
 *
 * The function type determines which ending is added to the above string
 * - FUNCTION_WAIT_ANY: "_wait_any"    (9)
 * - FUNCTION_RECV_ANY: "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP: "_server_loop" (12)
 * - FUNCTION_DISPATCH: "_dispatch"    (9)
 * - FUNCTION_REPLY_ANY_WAIT_ANY: "_reply_and_wait"  (15)
 *
 *    \todo if nested libraries regard them as well
 */
string 
CBENameFactory::GetFunctionName(CFEInterface * pFEInterface, 
    CBEContext * pContext)
{
    if (!pFEInterface)
       	return string();
    
    // get function type
    int nFunctionType = pContext->GetFunctionType() & FUNCTION_NOTESTFUNCTION;
    
    string sReturn;
    if ((pContext->GetFunctionType() & FUNCTION_TESTFUNCTION) > 0)
    	sReturn += "test_";
    if (pContext->IsBackEndSet(PROGRAM_BE_C))
    {
	if (pFEInterface->GetSpecificParent<CFELibrary>())
	    sReturn += pFEInterface->GetSpecificParent<CFELibrary>()
		->GetName() +  "_";
	sReturn += pFEInterface->GetName();
    }
    else if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	sReturn += "_dice";
    }
    switch (nFunctionType)
    {
    case FUNCTION_WAIT_ANY:
     	sReturn += "_wait_any";
     	break;
    case FUNCTION_RECV_ANY:
        sReturn += "_recv_any";
        break;
    case FUNCTION_SRV_LOOP:
        sReturn += "_server_loop";
        break;
    case FUNCTION_DISPATCH:
        sReturn += "_dispatch";
        break;
    case FUNCTION_REPLY_ANY_WAIT_ANY:
        sReturn += "_reply_and_wait";
        break;
    default:
        break;
    }

    if (m_bVerbose)
        printf("CBENameFactory::GetFunctionName(%s, functiontype:%d) = %s\n",
               pFEInterface->GetName().c_str(), nFunctionType,
               sReturn.c_str());
    return sReturn;
}

/** \brief creates the name of "other" functions
 *  \param nFunctionType the type of the function
 *  \param pContext the omnipresent context
 *  \return the name of the function
 */
string 
CBENameFactory::GetFunctionName(int nFunctionType, CBEContext *pContext)
{
    return string();
}

/** \brief creates a unique define label for a header file name
 *  \param sFilename the filename to create the define for
 *  \param pContext the context of the name generation
 *  \return a unique define label
 *
 * To build a unique define label from a file name there is not much to do -
 * to file name is unique itself.  To make it look fancy we simply prefix and
 * suffix the file name with two underscores and replace all "nonconforming
 * characters" with underscores. Because define labale commonly are uppercase,
 * we do that as well
 */
string CBENameFactory::GetHeaderDefine(string sFilename, CBEContext * pContext)
{
    if (sFilename.empty())
    return string();

    // add underscores
    string sReturn;
    sReturn = "__" + sFilename + "__";
    // make uppercase
    transform(sReturn.begin(), sReturn.end(), sReturn.begin(), toupper);
    // replace "nonconforming characters"
    string::size_type pos;
    while ((pos = sReturn.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) != string::npos)
        sReturn[pos] = '_';
//     sReturn.ReplaceExcluding("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", '_');

    return sReturn;
}

/** \brief generates a define symbol to brace the definition of a type
 *  \param sTypedefName the name of the typedef to brace
 *  \param pContext the context of the code generation
 *  \return the define symbol
 *
 * Add to the type's name two underscores on either side and the "typedef_"
 * string. Then remove "nonconforming characters" and make the string
 * uppercase.
 */
string CBENameFactory::GetTypeDefine(string sTypedefName, CBEContext * pContext)
{
    if (sTypedefName.empty())
        return string();

    // add underscores
    string sReturn;
    sReturn = "__typedef_" + sTypedefName + "__";
    // make uppercase
    transform(sReturn.begin(), sReturn.end(), sReturn.begin(), toupper);
    // replace "nonconforming characters"
    string::size_type pos;
    while ((pos = sReturn.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) != string::npos)
        sReturn[pos] = '_';

    return sReturn;
}

/** \brief generates a variable name for the return variable of a function
 *  \param pContext the context of the variable
 *  \return a variable name for the return variable
 */
string CBENameFactory::GetReturnVariable(CBEContext * pContext)
{
    return string("_dice_return");
}

/** \brief generates a variable name for the opcode variable
 *  \param pContext the context of the variable
 *  \return a variable name for the opcode variable
 */
string CBENameFactory::GetOpcodeVariable(CBEContext * pContext)
{
    return string("_dice_opcode");
}

/** \brief generates a variable name for the server loop reply code variable
 *  \param pContext the context of the variable
 *  \return a variable name for the reply code variable
 */
string CBENameFactory::GetReplyCodeVariable(CBEContext * pContext)
{
    return string("_dice_reply");
}

/** \brief generates the variable name for the return variable of the server loop
 *  \param pContext the context of the variable
 *  \return a variable name for the return variable
 */
string CBENameFactory::GetSrvReturnVariable(CBEContext * pContext)
{
    return string("_dice_srv_return");
}

/** \brief generate the constant name of the function's opcode
 *  \param pFunction the operation to generate the opcode for
 *  \param pContext the context of the name generation
 *  \return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;function name&gt;_opcode
 */
string CBENameFactory::GetOpcodeConst(CBEFunction * pFunction, CBEContext * pContext)
{
    if (!pFunction)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because function is 0\n");
        return string();
    }

    string sReturn;
    CBEClass *pClass = pFunction->GetClass();
    // if pClass != 0 search for FE function
    if (pClass)
    {
        CFunctionGroup *pGroup = pClass->FindFunctionGroup(pFunction);
        if (pGroup)
        {
            return GetOpcodeConst(pGroup->GetOperation(), pContext);
        }
    }

    CBENameSpace *pNameSpace = (pClass) ? pClass->GetNameSpace() : 0;
    if (pNameSpace)
        sReturn += (pNameSpace->GetName()) + "_";
    if (pClass)
        sReturn += (pClass->GetName()) + "_";
    sReturn += pFunction->GetName() + "_opcode";
    // make upper case
    transform(sReturn.begin(), sReturn.end(), sReturn.begin(), toupper);

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(BE:%s) = %s\n",
               pFunction->GetName().c_str(), sReturn.c_str());
    return sReturn;
}

/** \brief generate the constant name of the function's opcode
 *  \param pFEOperation the operation to generate the opcode for
 *  \param pContext the context of the name generation
 *  \return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;function name&gt;_opcode
 */
string CBENameFactory::GetOpcodeConst(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!pFEOperation)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because FE function is 0\n");
        return string();
    }

    string sReturn;
    CFEInterface *pFEInterface = pFEOperation->GetSpecificParent<CFEInterface>();
    CFELibrary *pFELibrary = (pFEInterface) ? pFEInterface->GetSpecificParent<CFELibrary>() : 0;
    if (pFELibrary)
        sReturn += (pFELibrary->GetName()) + "_";
    if (pFEInterface)
        sReturn += (pFEInterface->GetName()) + "_";
    sReturn += pFEOperation->GetName() + "_opcode";
    // make upper case
    transform(sReturn.begin(), sReturn.end(), sReturn.begin(), toupper);

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(FE:%s) = %s\n",
               pFEOperation->GetName().c_str(), sReturn.c_str());
    return sReturn;
}

/** \brief generate the constant name of the interface's base opcode
 *  \param pClass the interface to generate the opcode for
 *  \param pContext the context of the name generation
 *  \return a string containing the base-opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_base_opcode
 */
string CBENameFactory::GetOpcodeConst(CBEClass * pClass, CBEContext * pContext)
{
    if (!pClass)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because class is 0\n");
        return string();
    }

    string sReturn;
    CBENameSpace *pNameSpace = pClass->GetNameSpace();
    if (pNameSpace)
        sReturn += (pNameSpace->GetName()) + "_";
    sReturn += pClass->GetName() + "_base_opcode";
    // make upper case
    transform(sReturn.begin(), sReturn.end(), sReturn.begin(), toupper);

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(C:%s) = %s\n",
               pClass->GetName().c_str(), sReturn.c_str());
    return sReturn;
}

/** \brief generates the inline prefix
 *  \param pContext the context of the write operation
 *  \return a string containing the prefix (usually "inline")
 */
string CBENameFactory::GetInlinePrefix(CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_INLINE_EXTERN))
        return string("extern inline");
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_INLINE_STATIC))
        return string("static inline");
    else
        return string("inline");
}

/** \brief general function for accessing strings of derived name factories
 *  \param nStringCode the identifier of the requested string
 *  \param pContext the context of the string generation
 *  \param pParam additional unknown parameter
 *  \return the requested string
 *
 * If using an instance of this class, the explicit functions cover all requested strings.
 * This functions should only be accessed in derived name factories.
 */
string CBENameFactory::GetString(int nStringCode, CBEContext * pContext, void *pParam)
{
    return string();
}

/** \brief generates the variable name for the CORBA_object parameter
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetCorbaObjectVariable(CBEContext * pContext)
{
    return string("_dice_corba_obj");
}

/** \brief generates the variable name for the CORBA_environment parameter
 *  \param pContext the context of the code generation
 *  \return the name of the variable
 */
string CBENameFactory::GetCorbaEnvironmentVariable(CBEContext * pContext)
{
    return string("_dice_corba_env");
}

/** \brief generates the variable name for a message buffer
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetMessageBufferVariable(CBEContext * pContext)
{
    return string("_dice_msg_buffer");
}

/** \brief generates the name of the message buffer's type
 *  \param pFEInterface the interface this name is for
 *  \param pContext the context of the name generation
 *  \return the name of the type
 */
string CBENameFactory::GetMessageBufferTypeName(CFEInterface * pFEInterface, CBEContext * pContext)
{
    return GetMessageBufferTypeName(pFEInterface->GetName(), pContext);
}

/** \brief generates the name of the message buffer's type
 *  \param sInterfaceName the name of the interface this message buffer is for
 *  \param pContext the context of the code generation
 *  \return the name of the type;
 */
string CBENameFactory::GetMessageBufferTypeName(string sInterfaceName, CBEContext * pContext)
{
    string sBase = GetMessageBufferTypeName(pContext);
    if (sInterfaceName.empty())
        return string("dice_") + sBase;

    return sInterfaceName + string("_") + sBase;
}

/** \brief generates the name of the message buffer's type
 *  \param pContext the context of the name generation
 *  \return the base name of the message buffer type
 *
 * This function is used internally to get the "base" of the type name _and_ its
 * used if the the name is later altered by teh calling function (e.g. add scopes, etc.)
 */
string CBENameFactory::GetMessageBufferTypeName(CBEContext *pContext)
{
    return "msg_buffer_t";
}

/** \brief generates the variable of the client side timeout
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetTimeoutClientVariable(CBEContext * pContext)
{
    return string("timeout");
}

/** \brief generates the variable of the component side timeout
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetTimeoutServerVariable(CBEContext * pContext)
{
    return string("timeout");
}

/** \brief generates the variable of the client side scheduling options
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetScheduleClientVariable(CBEContext * pContext)
{
    return string();
}

/** \brief generates the variable containing the component identifier
 *  \param pContext the context of the name generation
 *  \return  the name of the variable
 */
string CBENameFactory::GetComponentIDVariable(CBEContext * pContext)
{
    return string("componentID");
}

/** \brief generate a global variable name for the testsuite
 *  \param pParameter the parameter to make a global name for
 *  \param pContext the context of the name generation
 *  \return the name of the global variable
 */
string CBENameFactory::GetGlobalTestVariable(CBEDeclarator * pParameter,
                         CBEContext * pContext)
{
    if (!pParameter)
        return string();
    // can be the parameter of a function
    CBEFunction *pFunc = pParameter->GetSpecificParent<CBEFunction>();
    if (pFunc)
        return pFunc->GetName() + "_" + pParameter->GetName();
    return string();
}

/** \brief generate a global variable name for the return value of a function
 *  \param pFunction the function to generate the return var for
 *  \param pContext the context of the name generation
 *  \return the name of the global return variable
 */
string CBENameFactory::GetGlobalReturnVariable(CBEFunction * pFunction,
                           CBEContext * pContext)
{
    if (!pFunction)
        return string();
    string sRetVar = GetReturnVariable(pContext);
    return pFunction->GetName() + "_" + sRetVar;
}

/** \brief generates the message buffers member for a specific type
 *  \param pType the type fot which the member is searched
 *  \param pContext the context of the name generation
 *  \return the name of the message buffer's member responsible for the type
 */
string CBENameFactory::GetMessageBufferMember(CBEType * pType, CBEContext * pContext)
{
    return GetMessageBufferMember(pType->GetFEType(), pContext);
}

/** \brief generates the message buffers member for a specific type
 *  \param nFEType  the type fot which the member is searched
 *  \param pContext the context of the name generation
 *  \return the name of the message buffer's member responsible for the type
 */
string CBENameFactory::GetMessageBufferMember(int nFEType, CBEContext * pContext)
{
    string sReturn;
    switch (nFEType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
    case TYPE_MWORD:
    case TYPE_VOID:
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
    case TYPE_LONG_DOUBLE:
    case TYPE_CHAR:
    case TYPE_BOOLEAN:
    case TYPE_BYTE:
    case TYPE_VOID_ASTERISK:
    case TYPE_CHAR_ASTERISK:
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_ENUM:
    case TYPE_PIPE:
    case TYPE_TAGGED_STRUCT:
    case TYPE_TAGGED_UNION:
    case TYPE_HANDLE_T:
    case TYPE_ISO_LATIN_1:
    case TYPE_ISO_MULTILINGUAL:
    case TYPE_ISO_UCS:
    case TYPE_ERROR_STATUS_T:
    case TYPE_FLEXPAGE:
    case TYPE_RCV_FLEXPAGE:
    case TYPE_USER_DEFINED:
    case TYPE_WCHAR:
    case TYPE_OCTET:
    case TYPE_ANY:
    case TYPE_OBJECT:
    case TYPE_TAGGED_ENUM:
    case TYPE_STRING:
    case TYPE_WSTRING:
    case TYPE_ARRAY:
        sReturn = "_bytes";
        break;
    case TYPE_REFSTRING:
        sReturn = "_strings";
        break;
    default:
        sReturn = "_stuff";
        break;
    }
    return sReturn;
}

/**  \brief generate variable name for switch variable
 *   \return name of switch variable
 *
 * Generates a variable for a swicth variable of a union.
 * This variable is also member of a struct, which surrounds the union,
 * and thus does not have to be unique.
 *
 * Conforming to the CORBA C language mapping this is "_d"
 */
string CBENameFactory::GetSwitchVariable()
{
    return string("_d");
}

/** \brief generates the variable name of the offset variable
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetOffsetVariable(CBEContext * pContext)
{
    return string("_dice_offset");
}

/** \brief generates the variable name of a temporary offset variable
 *  \param pContext the context of the name generation
 *  \return the name of the variable
 */
string CBENameFactory::GetTempOffsetVariable(CBEContext * pContext)
{
    return string("_dice_tmp_offset");
}

/** \brief generates the constant name for the function bitmask
 *  \return the name of the constant
 */
string CBENameFactory::GetFunctionBitMaskConstant()
{
    return string("DICE_FID_MASK");
}

/** \brief generates a constant name containing the shift bits for the interface ID
 *  \return the name of the constant
 */
string CBENameFactory::GetInterfaceNumberShiftConstant()
{
    return string("DICE_IID_BITS");
}

/** \brief generates a generic variable name for the server loop
 *  \return a variable name
 */
string CBENameFactory::GetServerParameterName()
{
    return string("dice_server_param");
}

/** \brief get a scoped name for a tagged declarator or typedef
 *  \param pFERefType the class to use as reference in the hierarchy
 *  \param sName the name to scope
 *  \param pContext the context of this name creation
 *  \return the scoped name
 *
 * This function is used to generate a suitable name for a tagged decl, typedef or
 * type declaration with a flat namespace.
 */
string CBENameFactory::GetTypeName(CFEBase *pFERefType, string sName, CBEContext *pContext)
{
    string sReturn;
    // check for parent interface
    CFEInterface *pInterface = pFERefType->GetSpecificParent<CFEInterface>();
    if (pInterface)
    {
        sReturn = pInterface->GetName() + "_";
    }
    // check for parent libraries
    CFELibrary *pLib = pFERefType->GetSpecificParent<CFELibrary>();
    while (pLib)
    {
        sReturn = pLib->GetName() + "_" + sReturn;
        pLib = pLib->GetSpecificParent<CFELibrary>();
    }
    // add original name
    sReturn += sName;
    return sReturn;
}

/** \brief get a scoped name for the constant
 *  \param pFEConstant the constant which's name should be scoped
 *  \param pContext the context of this scoping
 *  \return the scoped named
 */
string CBENameFactory::GetConstantName(CFEConstDeclarator* pFEConstant, CBEContext* pContext)
{
    string sReturn;
    // check for parent interface
    CFEInterface *pInterface = pFEConstant->GetSpecificParent<CFEInterface>();
    if (pInterface)
    {
        sReturn = pInterface->GetName() + "_";
    }
    // check for parent libraries
    CFELibrary *pLib = pFEConstant->GetSpecificParent<CFELibrary>();
    while (pLib)
    {
        sReturn = pLib->GetName() + "_" + sReturn;
        pLib = pLib->GetSpecificParent<CFELibrary>();
    }
    // add original name
    sReturn += pFEConstant->GetName();
    return sReturn;
}

/** \brief creates the name of a dummy variable
 *  \param pContext the context of the name creation
 *  \return the name of the dummy variable
 *
 *  \todo count the dummy variables in the context
 */
string CBENameFactory::GetDummyVariable(CBEContext* pContext)
{
    return string("dummy");
}

/** \brief returns the variable name of a exception word variable
 *  \return the string _exception
 */
string CBENameFactory::GetExceptionWordVariable(CBEContext *pContext)
{
    return string("_exception");
}
