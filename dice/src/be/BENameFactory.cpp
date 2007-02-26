/**
 *	\file	dice/src/be/BENameFactory.cpp
 *	\brief	contains the implementation of the class CBENameFactory
 *
 *	\date	01/10/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

IMPLEMENT_DYNAMIC(CBENameFactory);

CBENameFactory::CBENameFactory(bool bVerbose)
{
    m_bVerbose = bVerbose;
    IMPLEMENT_DYNAMIC_BASE(CBENameFactory, CBEObject);
}

CBENameFactory::CBENameFactory(CBENameFactory & src):CBEObject(src)
{
    m_bVerbose = src.m_bVerbose;
    IMPLEMENT_DYNAMIC_BASE(CBENameFactory, CBEObject);
}

/**	\brief the destructor of this class */
CBENameFactory::~CBENameFactory()
{

}

/**	\brief creates the file-name for a specific file type
 *	\param pFEBase a reference for the front-end class, which defines granularity (IDLFILE, MODULE, INTERFACE, OPERATION)
 *	\param pContext the context of the generated name
 *	\return a reference to the created name or 0 if some error occured
 *
 * We have several file types: we have one header file and opcode file per front-end IDL file.
 * There may be several implementation files for a front-end IDL file.
 * E.g. one per IDL file, one per module, one per interface or one per function.
 */
String CBENameFactory::GetFileName(CFEBase * pFEBase, CBEContext * pContext)
{
	if (m_bVerbose)
		printf("CBENameFactory::GetFileName\n");

    if (!pFEBase)
	{
		if (m_bVerbose)
			printf("CBENameFactory::GetFileName failed because front-end class is 0\n");
		return String();
	}

    String sReturn;
    String sPrefix = pContext->GetFilePrefix();
    int nFileType = pContext->GetFileType();

    // first check non-IDL files
    if (pFEBase->IsKindOf(RUNTIME_CLASS(CFEFile)))
	{
		if (!((CFEFile *) pFEBase)->IsIDLFile())
		{
			// get file-name
			sReturn = sPrefix;
			sReturn += ((CFEFile *) pFEBase)->GetFileName();
			// deliver filename
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s (!IDL file)\n",
						pFEBase->GetClassName(), nFileType, (const char *) sReturn);

			return sReturn;
		}
	}
    // test for header files
    if ((nFileType == FILETYPE_CLIENTHEADER) ||
		(nFileType == FILETYPE_COMPONENTHEADER) ||
		(nFileType == FILETYPE_OPCODE) ||
		(nFileType == FILETYPE_TESTSUITE) ||
		(nFileType == FILETYPE_SKELETON) ||
		(nFileType == FILETYPE_COMPONENTIMPLEMENTATION))
	{
		// should only be files
		if (!pFEBase->IsKindOf(RUNTIME_CLASS(CFEFile)))
		{
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName failed because filetype required CFEFile and it wasn't\n");
			return String();
		}
		// assemble string
		sReturn = sPrefix;
		sReturn += ((CFEFile *) pFEBase)->GetFileNameWithoutExtension();
		switch (nFileType)
		{
		case FILETYPE_CLIENTHEADER:
			sReturn += "-client.h";
			break;
		case FILETYPE_COMPONENTHEADER:
			sReturn += "-server.h";
			break;
		case FILETYPE_COMPONENTIMPLEMENTATION:
			sReturn += "-server.c";
			break;
		case FILETYPE_OPCODE:
			sReturn += "-sys.h";
			break;
		case FILETYPE_TESTSUITE:
			sReturn += "-testsuite.c";
			break;
		case FILETYPE_SKELETON:
			sReturn += "-template.c";
			break;
		default:
			break;
		}
		if (m_bVerbose)
			printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s (header, opcode, testuite)\n",
				pFEBase->GetClassName(), nFileType, (const char *) sReturn);
		return sReturn;
	}


    if (pContext->IsOptionSet(PROGRAM_FILE_IDLFILE) ||
		pContext->IsOptionSet(PROGRAM_FILE_ALL))
	{
		// filename := [&lt;prefix&gt;]&lt;IDL-file-name&gt;-(client|server|testsuite|opcode).c
		// check FE type
		if (!pFEBase->IsKindOf(RUNTIME_CLASS(CFEFile)))
	    {
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_IDLFILE/ALL and not CFEFile\n");
			return String();
		}
		// get FE file
		CFEFile *pFEFile = (CFEFile *) pFEBase;
		// get file-name
		sReturn = sPrefix;
		sReturn += pFEFile->GetFileNameWithoutExtension();
		sReturn += "-client.c";
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_MODULE))
	{
		// filename := [&lt;prefix&gt;]&lt;libname&gt;-(client|server).c
		// check FE type
		// can also be interface (if FILE_MODULE, but top level interface)
		if (pFEBase->IsKindOf(RUNTIME_CLASS(CFEInterface)))
		{
			pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_MODULE);
			String sReturn = GetFileName(pFEBase, pContext);
			pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_INTERFACE);
			return sReturn;
		}
		// else if no lib: return 0
		if (!pFEBase->IsKindOf(RUNTIME_CLASS(CFELibrary)))
		{
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_MODULE and not CFELibrary\n");
			return String();
		}
		// get libname-name
		sReturn = sPrefix;
		sReturn += ((CFELibrary *) pFEBase)->GetName();
		sReturn += "-client.c";
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
	{
		// filename := [&lt;prefix&gt;][&lt;libname&gt;_]&lt;interfacename&gt;-(client|server).c
		// can also be library (if FILE_INTERFACE, but library with types or constants)
		if (pFEBase->IsKindOf(RUNTIME_CLASS(CFELibrary)))
		{
			pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_INTERFACE);
			String sReturn = GetFileName(pFEBase, pContext);
			pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_MODULE);
			return sReturn;
		}
		// check FE type
		if (!pFEBase->IsKindOf(RUNTIME_CLASS(CFEInterface)))
		{
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_INTERFACE and not CFEInterface\n");
			return String();
		}
		// get interface name
		sReturn = sPrefix;
		CFEInterface *pFEInterface = (CFEInterface *) pFEBase;
		if (pFEInterface->GetParentLibrary())
		{
			sReturn += pFEInterface->GetParentLibrary()->GetName();
			sReturn += "_";
		}
		sReturn += pFEInterface->GetName();
		sReturn += "-client.c";
	}
	else if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION))
	{
		// filename := [&lt;prefix&gt;][&lt;libname&gt;_][&lt;interfacename&gt;_]&lt;funcname&gt;-(client|server).c
		// can also be library (if contains types or constants)
		if (pFEBase->IsKindOf(RUNTIME_CLASS(CFELibrary)))
		{
			pContext->ModifyOptions(PROGRAM_FILE_MODULE, PROGRAM_FILE_FUNCTION);
			String sReturn = GetFileName(pFEBase, pContext);
			pContext->ModifyOptions(PROGRAM_FILE_FUNCTION, PROGRAM_FILE_MODULE);
			return sReturn;
		}
		// can also be interface (if contains types or constants)
		if (pFEBase->IsKindOf(RUNTIME_CLASS(CFEInterface)))
		{
			pContext->ModifyOptions(PROGRAM_FILE_INTERFACE, PROGRAM_FILE_FUNCTION);
			String sReturn = GetFileName(pFEBase, pContext);
			pContext->ModifyOptions(PROGRAM_FILE_FUNCTION, PROGRAM_FILE_INTERFACE);
			return sReturn;
		}
		// check FE type
		if (!pFEBase->IsKindOf(RUNTIME_CLASS(CFEOperation)))
		{
			if (m_bVerbose)
				printf("CBENameFactory::GetFileName failed because PROGRAM_FILE_FUNCTION and not CFEOperation\n");
			return String();
		}
		// get class
		sReturn = sPrefix;
		CFEOperation *pFEOperation = (CFEOperation *) pFEBase;
		if (pFEOperation->GetParentLibrary())
		{
			sReturn += pFEOperation->GetParentLibrary()->GetName();
			sReturn += "_";
		}
		if (pFEOperation->GetParentInterface())
		{
			sReturn += pFEOperation->GetParentInterface()->GetName();
			sReturn += "_";
		}
		// get operation's name
		sReturn += pFEOperation->GetName();
		sReturn += "-client.c";
	}
	if (m_bVerbose)
		printf("CBENameFactory::GetFileName(%s, filetype:%d) = %s\n",
			pFEBase->GetClassName(), nFileType, (const char *) sReturn);
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
String CBENameFactory::GetIncludeFileName(CFEBase * pFEBase, CBEContext * pContext)
{
    // first get the file name as usual
    String sName = GetFileName(pFEBase, pContext);
    String sPrefix = pContext->GetFilePrefix();
    // extract the relative path from the original FE file
    CFEFile *pFEFile = pFEBase->GetFile();
    // if no IDL file, return original name
    String sOriginalName = pFEFile->GetFileName();
    if (!pFEFile->IsIDLFile())
        return sPrefix + sOriginalName;
    // get file name (which contains relative path) and extract it
    // it is everything up to the last '/'
    int nPos = sOriginalName.ReverseFind('/');
    String sPath;
    if (nPos > 0)
        sPath = sOriginalName.Left(nPos + 1);
    // concat path and name and return
    return sPath + sName;
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
String CBENameFactory::GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize)
{
    String sReturn;
    if (pContext->IsOptionSet(PROGRAM_USE_CTYPES))
        sReturn = GetCTypeName(nType, bUnsigned, pContext, nSize);
    if (!sReturn.IsEmpty())
        return sReturn;
    switch (nType)
    {
    case TYPE_NONE:
        sReturn = "_UNDEFINED_";
        break;
    case TYPE_INTEGER:
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
                sReturn = "CORBA_unsigned_long";
            else
                sReturn = "CORBA_long";
            break;
        case 8:
            if (bUnsigned)
                sReturn = "CORBA_unsigned_long_long";
            else
                sReturn = "CORBA_long_long";
            break;
        }
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
    case TYPE_OCTET:		// 8-bit type, which will never be converted
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
        sReturn.Empty();
        break;
    default:
        sReturn = "_UNDEFINED_def";
        break;
    }

    if (m_bVerbose)
        printf("%s Generated type name \"%s\" for type code %d\n",
            GetClassName(), (const char *) sReturn, nType);
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
String CBENameFactory::GetCTypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize)
{
    String sReturn;
    switch (nType)
    {
    case TYPE_INTEGER:
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
                sReturn = "unsigned long";
            else
                sReturn = "long";
            break;
        case 8:
            if (bUnsigned)
                sReturn = "unsigned long long";
            else
                sReturn = "long long";
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
        sReturn.Empty();
        break;
    }
    return sReturn;
}

/**	\brief creates the name of a function
 *	\param pFEOperation the function to create a name for
 *	\param pContext the context of the name generation
 *	\return the name of the back-end function
 *
 * The name has to be unique, therefore it is build using parent interfaces and libraries.
 * The name looks like this:
 *
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;operation name&gt;
 *
 * The function type determines the ending:
 * - FUNCTION_SEND: "_send"
 * - FUNCTION_RECV: "_recv"
 * - FUNCTION_WAIT: "_wait"
 * - FUNCTION_UNMARSHAL:  "_unmarshal"   (10)
 * - FUNCTION_REPLY_RECV: "_reply_recv"  (11)
 * - FUNCTION_REPLY_WAIT: "_reply_wait"  (11)
 * - FUNCTION_CALL: "_call"
 * - FUNCTION_SWITCH_CASE: "_call"
 * - FUNCTION_SKELETON:   "_component"   (10)
 *
 * The three other function types should not be used with this implementation, because these are interface functions.
 * If they are (accidentally) used here, we redirect the call to the interface function naming implementation.
 * - FUNCTION_WAIT_ANY:   "_wait_any"    (9)
 * - FUNCTION_RECV_ANY:   "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP:   "_server_loop" (12)
 *
 * \todo if nested library use all lib names
 */
String CBENameFactory::GetFunctionName(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!pFEOperation)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetFunctionName failed because the operation is 0\n");
        return String();
    }
    // get file type
    int nFunctionType = pContext->GetFunctionType() & FUNCTION_NOTESTFUNCTION;
    // check for interface functions
    if ((nFunctionType == FUNCTION_WAIT_ANY) ||
        (nFunctionType == FUNCTION_RECV_ANY) ||
        (nFunctionType == FUNCTION_SRV_LOOP) ||
        (nFunctionType == FUNCTION_REPLY_ANY_WAIT_ANY))
        return GetFunctionName(pFEOperation->GetParentInterface(), pContext);

    String sReturn;
    if ((pContext->GetFunctionType() & FUNCTION_TESTFUNCTION) > 0)
        sReturn += "test_";
    if (pFEOperation->GetParentLibrary())
        sReturn += pFEOperation->GetParentLibrary()->GetName() + "_";
    if (pFEOperation->GetParentInterface())
        sReturn += pFEOperation->GetParentInterface()->GetName() + "_";
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
        sReturn += "_call";
        break;
    case FUNCTION_UNMARSHAL:
        sReturn += "_unmarshal";
        break;
    case FUNCTION_SKELETON:
        sReturn += "_component";
        break;
    case FUNCTION_REPLY_RECV:
        sReturn += "_reply_recv";
        break;
    case FUNCTION_REPLY_WAIT:
        sReturn += "_reply_wait";
        break;
    default:
        break;
    }

    if (m_bVerbose)
        printf("CBENameFactory::GetFunctionName(%s, functiontype:%d) = %s\n",
               (const char *) pFEOperation->GetName(), nFunctionType,
               (const char *) sReturn);
    return sReturn;
}

/**	\brief creates the name of a function
 *	\param pFEInterface the interface to create a name for
 *	\param pContext the context of the name generat
 *	\return the name of the interface's function
 *
 * This implementation creates function names for interface functions. The name looks like this:
 *
 * [&lt;library name&gt;_]&lt;interface name&gt;
 *
 * The function type determines which ending is added to the above string
 * - FUNCTION_WAIT_ANY: "_wait_any"    (9)
 * - FUNCTION_RECV_ANY: "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP: "_server_loop" (12)
 * - FUNCTION_REPLY_ANY_WAIT_ANY: "_reply_and_wait"  (15)
 *
 *	\todo if nested libraries regard them as well
 */
String CBENameFactory::GetFunctionName(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
    return String();

    // get function type
    int nFunctionType = pContext->GetFunctionType() & FUNCTION_NOTESTFUNCTION;

    String sReturn;
    if ((pContext->GetFunctionType() & FUNCTION_TESTFUNCTION) > 0)
        sReturn += "test_";
    if (pFEInterface->GetParentLibrary())
        sReturn += pFEInterface->GetParentLibrary()->GetName() + "_";
    sReturn += pFEInterface->GetName();
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
    case FUNCTION_REPLY_ANY_WAIT_ANY:
        sReturn += "_reply_and_wait";
        break;
    default:
        break;
    }

    if (m_bVerbose)
        printf("CBENameFactory::GetFunctionName(%s, functiontype:%d) = %s\n",
               (const char *) pFEInterface->GetName(), nFunctionType,
               (const char *) sReturn);
    return sReturn;
}

/**	\brief creates a unique define label for a header file name
 *	\param sFilename the filename to create the define for
 *	\param pContext the context of the name generation
 *	\return a unique define label
 *
 * To build a unique define label from a file name there is not much to do - to file name is unique itself.
 * To make it look fancy we simply prefix and suffix the file name with two underscores and replace all
 * "nonconforming characters" with underscores. Because define labale commonly are uppercase, we do that as well
 */
String CBENameFactory::GetHeaderDefine(String sFilename, CBEContext * pContext)
{
    if (sFilename.IsEmpty())
	return String();

    // add underscores
    String sReturn;
    sReturn = "__" + sFilename + "__";
    // make uppercase
    sReturn.MakeUpper();
    // replace "nonconforming characters"
    sReturn.ReplaceExcluding("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", '_');

    return sReturn;
}

/**	\brief generates a define symbol to brace the definition of a type
 *	\param sTypedefName the name of the typedef to brace
 *	\param pContext the context of the code generation
 *	\return the define symbol
 *
 * Add to the type's name two underscores on either side and the "typedef_" string. Then remove
 * "nonconforming characters" and make the string uppercase.
 */
String CBENameFactory::GetTypeDefine(String sTypedefName, CBEContext * pContext)
{
    if (sTypedefName.IsEmpty())
	return String();

    // add underscores
    String sReturn;
    sReturn = "__typedef_" + sTypedefName + "__";
    // make uppercase
    sReturn.MakeUpper();
    // replace "nonconforming characters"
    sReturn.ReplaceExcluding("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", '_');

    return sReturn;
}

/**	\brief generates a variable name for the return variable of a function
 *	\param pContext the context of the variable
 *	\return a variable name for the return variable
 */
String CBENameFactory::GetReturnVariable(CBEContext * pContext)
{
    return String("_dice_return");
}

/**	\brief generates a variable name for the opcode variable
 *	\param pContext the context of the variable
 *	\return a variable name for the opcode variable
 */
String CBENameFactory::GetOpcodeVariable(CBEContext * pContext)
{
    return String("_dice_opcode");
}

/**	\brief generates the variable name for the return variable of the server loop
 *	\param pContext the context of the variable
 *	\return a variable name for the return variable
 */
String CBENameFactory::GetSrvReturnVariable(CBEContext * pContext)
{
    return String("_dice_srv_return");
}

/**	\brief generate the constant name of the function's opcode
 *	\param pFunction the operation to generate the opcode for
 *	\param pContext the context of the name generation
 *	\return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;function name&gt;_opcode
 */
String CBENameFactory::GetOpcodeConst(CBEFunction * pFunction, CBEContext * pContext)
{
    if (!pFunction)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because function is 0\n");
        return String();
    }

    String sReturn;
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
    sReturn.MakeUpper();

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(BE:%s) = %s\n",
               (const char *) pFunction->GetName(), (const char *) sReturn);
    return sReturn;
}

/**	\brief generate the constant name of the function's opcode
 *	\param pFEOperation the operation to generate the opcode for
 *	\param pContext the context of the name generation
 *	\return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_&lt;function name&gt;_opcode
 */
String CBENameFactory::GetOpcodeConst(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!pFEOperation)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because FE function is 0\n");
        return String();
    }

    String sReturn;
    CFEInterface *pFEInterface = pFEOperation->GetParentInterface();
    CFELibrary *pFELibrary = (pFEInterface) ? pFEInterface->GetParentLibrary() : 0;
    if (pFELibrary)
        sReturn += (pFELibrary->GetName()) + "_";
    if (pFEInterface)
        sReturn += (pFEInterface->GetName()) + "_";
    sReturn += pFEOperation->GetName() + "_opcode";
    // make upper case
    sReturn.MakeUpper();

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(FE:%s) = %s\n",
               (const char *) pFEOperation->GetName(), (const char *) sReturn);
    return sReturn;
}

/**	\brief generate the constant name of the interface's base opcode
 *	\param pClass the interface to generate the opcode for
 *	\param pContext the context of the name generation
 *	\return a string containing the base-opcode constant name
 *
 * A opcode constant is usually named:
 * [&lt;library name&gt;_]&lt;interface name&gt;_base_opcode
 */
String CBENameFactory::GetOpcodeConst(CBEClass * pClass, CBEContext * pContext)
{
    if (!pClass)
    {
        if (m_bVerbose)
            printf("CBENameFactory::GetOpcodeConst failed because class is 0\n");
        return String();
    }

    String sReturn;
    CBENameSpace *pNameSpace = pClass->GetNameSpace();
    if (pNameSpace)
        sReturn += (pNameSpace->GetName()) + "_";
    sReturn += pClass->GetName() + "_base_opcode";
    // make upper case
    sReturn.MakeUpper();

    if (m_bVerbose)
        printf("CBENameFactory::GetOpcodeConst(C:%s) = %s\n",
               (const char *) pClass->GetName(), (const char *) sReturn);
    return sReturn;
}

/**	\brief generates the inline prefix
 *	\param pContext the context of the write operation
 *	\return a string containing the prefix (usually "inline")
 */
String CBENameFactory::GetInlinePrefix(CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_INLINE_EXTERN))
        return String("extern inline");
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_INLINE_STATIC))
        return String("static inline");
    else
        return String("inline");
}

/**	\brief general function for accessing strings of derived name factories
 *	\param nStringCode the identifier of the requested string
 *	\param pContext the context of the string generation
 *	\param pParam additional unknown parameter
 *	\return the requested string
 *
 * If using an instance of this class, the explicit functions cover all requested strings.
 * This functions should only be accessed in derived name factories.
 */
String CBENameFactory::GetString(int nStringCode, CBEContext * pContext, void *pParam)
{
    return String();
}

/**	\brief generates the variable name for the CORBA_object parameter
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetCorbaObjectVariable(CBEContext * pContext)
{
    return String("_dice_corba_obj");
}

/**	\brief generates the variable name for the CORBA_environment parameter
 *	\param pContext the context of the code generation
 *	\return the name of the variable
 */
String CBENameFactory::GetCorbaEnvironmentVariable(CBEContext * pContext)
{
    return String("_dice_corba_env");
}

/**	\brief generates the variable name for a message buffer
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetMessageBufferVariable(CBEContext * pContext)
{
    return String("_dice_msg_buffer");
}

/**	\brief generates the name of the message buffer's type
 *	\param pFEInterface the interface this name is for
 *	\param pContext the context of the name generation
 *	\return the name of the type
 */
String CBENameFactory::GetMessageBufferTypeName(CFEInterface * pFEInterface, CBEContext * pContext)
{
    return GetMessageBufferTypeName(pFEInterface->GetName(), pContext);
}

/**	\brief generates the name of the message buffer's type
 *	\param sInterfaceName the name of the interface this message buffer is for
 *	\param pContext the context of the code generation
 *	\return the name of the type;
 */
String CBENameFactory::GetMessageBufferTypeName(String sInterfaceName, CBEContext * pContext)
{
    String sBase = GetMessageBufferTypeName(pContext);
    if (sInterfaceName.IsEmpty())
        return String("dice_") + sBase;

    return sInterfaceName + String("_") + sBase;
}

/** \brief generates the name of the message buffer's type
 *  \param pContext the context of the name generation
 *  \return the base name of the message buffer type
 *
 * This function is used internally to get the "base" of the type name _and_ its
 * used if the the name is later altered by teh calling function (e.g. add scopes, etc.)
 */
String CBENameFactory::GetMessageBufferTypeName(CBEContext *pContext)
{
    return "msg_buffer_t";
}

/**	\brief generates the variable of the client side timeout
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetTimeoutClientVariable(CBEContext * pContext)
{
    return String("timeout");
}

/**	\brief generates the variable of the component side timeout
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetTimeoutServerVariable(CBEContext * pContext)
{
    return String("timeout");
}

/**	\brief generates the variable containing the component identifier
 *	\param pContext the context of the name generation
 *	\return  the name of the variable
 */
String CBENameFactory::GetComponentIDVariable(CBEContext * pContext)
{
    return String("componentID");
}

/**	\brief generate a global variable name for the testsuite
 *	\param pParameter the parameter to make a global name for
 *	\param pContext the context of the name generation
 *	\return the name of the global variable
 */
String CBENameFactory::GetGlobalTestVariable(CBEDeclarator * pParameter,
					     CBEContext * pContext)
{
    if (!pParameter)
	return String();
    CBEFunction *pFunc = pParameter->GetFunction();
    ASSERT(pFunc);
    return pFunc->GetName() + "_" + pParameter->GetName();
}

/**	\brief generate a global variable name for the return value of a function
 *	\param pFunction the function to generate the return var for
 *	\param pContext the context of the name generation
 *	\return the name of the global return variable
 */
String CBENameFactory::GetGlobalReturnVariable(CBEFunction * pFunction,
					       CBEContext * pContext)
{
    if (!pFunction)
	return String();
    String sRetVar = GetReturnVariable(pContext);
    return pFunction->GetName() + "_" + sRetVar;
}

/**	\brief generates the message buffers member for a specific type
 *	\param pType the type fot which the member is searched
 *	\param pContext the context of the name generation
 *	\return the name of the message buffer's member responsible for the type
 */
String CBENameFactory::GetMessageBufferMember(CBEType * pType, CBEContext * pContext)
{
    return GetMessageBufferMember(pType->GetFEType(), pContext);
}

/**	\brief generates the message buffers member for a specific type
 *	\param nFEType  the type fot which the member is searched
 *	\param pContext the context of the name generation
 *	\return the name of the message buffer's member responsible for the type
 */
String CBENameFactory::GetMessageBufferMember(int nFEType, CBEContext * pContext)
{
    String sReturn;
    switch (nFEType)
    {
    case TYPE_INTEGER:
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
String CBENameFactory::GetSwitchVariable()
{
    return String("_d");
}

/**	\brief generates the variable name of the offset variable
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetOffsetVariable(CBEContext * pContext)
{
    return String("_dice_offset");
}

/**	\brief generates the variable name of a temporary offset variable
 *	\param pContext the context of the name generation
 *	\return the name of the variable
 */
String CBENameFactory::GetTempOffsetVariable(CBEContext * pContext)
{
    return String("_dice_tmp_offset");
}

/** \brief generates the constant name for the function bitmask
 *  \return the name of the constant
 */
String CBENameFactory::GetFunctionBitMaskConstant()
{
    return String("DICE_FID_MASK");
}

/** \brief generates a constant name containing the shift bits for the interface ID
 *  \return the name of the constant
 */
String CBENameFactory::GetInterfaceNumberShiftConstant()
{
    return String("DICE_IID_BITS");
}

/** \brief generates a generic variable name for the server loop
 *  \return a variable name
 */
String CBENameFactory::GetServerParameterName()
{
    return String("dice_server_param");
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
String CBENameFactory::GetTypeName(CFEBase *pFERefType, String sName, CBEContext *pContext)
{
    String sReturn;
    // check for parent interface
    CFEInterface *pInterface = pFERefType->GetParentInterface();
    if (pInterface)
    {
        sReturn = pInterface->GetName() + "_";
    }
    // check for parent libraries
    CFELibrary *pLib = pFERefType->GetParentLibrary();
    while (pLib)
    {
        sReturn = pLib->GetName() + "_" + sReturn;
        pLib = pLib->GetParentLibrary();
    }
    // add original name
    sReturn += sName;
    return sReturn;
}

/** \brief creates the name of a dummy variable
 *  \param pContext the context of the name creation
 *  \return the name of the dummy variable
 *
 *  \todo count the dummy variables in the context
 */
String CBENameFactory::GetDummyVariable(CBEContext* pContext)
{
    return String("dummy");
}
