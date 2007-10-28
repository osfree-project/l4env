/**
 * \file    dice/src/be/BENameFactory.cpp
 * \brief   contains the implementation of the class CBENameFactory
 *
 * \date    01/10/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEFunction.h"
#include "be/BEType.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"
#include "be/BETypedDeclarator.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEConstDeclarator.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include "FactoryFactory.h"

#include <typeinfo>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <cassert>

CBENameFactory* CBENameFactory::m_pInstance = 0;

CBENameFactory::CBENameFactory()
{ }

/** \brief the destructor of this class */
CBENameFactory::~CBENameFactory()
{ }

/** \brief retrieve the active name factory
 *  \return a reference to the name factory
 *
 * The intention was to move the factory creation code from the
 * factory-factory here, but this didn't work: this class is in the basic
 * back-end lib (libbe.a) and uses methods from other back-end libs. When
 * linking dice the libs are linked with the most derived one first so that
 * basic libs can provide the missing methods. Placing the factory creation
 * code here would require the back-end libs to be placed in a group that is
 * linked until all dependencies are met. I couldn't get automake to obey my
 * will, thus I had to find another solution: I placed the factory creation
 * code at the top level into the factory-factory. This class gets added to
 * the binary last and because its an object file when linking the binary it
 * somehow works.
 */
CBENameFactory* CBENameFactory::Instance()
{
	if (!m_pInstance)
	{
		CFactoryFactory ff;
		m_pInstance = ff.GetNewNameFactory();
	}
	return m_pInstance;
}

/** \brief creates the file-name for a specific file type
 *  \param pFEBase a reference for the front-end class, which defines \
 *         granularity (IDLFILE, MODULE, INTERFACE, OPERATION)
 *  \param nFileType the type of the file
 *  \return a reference to the created name or 0 if some error occured
 *
 * We have several file types: we have one header file and opcode file per
 * front-end IDL file.  There may be several implementation files for a
 * front-end IDL file.  E.g. one per IDL file, one per module, one per
 * interface or one per function.
 */
string
CBENameFactory::GetFileName(CFEBase * pFEBase,
	FILE_TYPE nFileType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBENameFactory::%s called\n",
		__func__);

	if (!pFEBase)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s failed because front-end class is 0\n", __func__);
		return string();
	}

	string sReturn;
	string sPrefix;
	CCompiler::GetBackEndOption(string("file-prefix"), sPrefix);

	// first check non-IDL files
	CFEFile *pFEFile = dynamic_cast<CFEFile *>(pFEBase);
	if (pFEFile)
	{
		if (!pFEFile->IsIDLFile())
		{
			// get file-name
			sReturn = sPrefix;
			sReturn += pFEFile->GetFileName();
			// deliver filename
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s(%s, filetype: %d) = %s (!IDL file)\n",
				__func__, typeid(*pFEBase).name(), nFileType,
				sReturn.c_str());

			return sReturn;
		}
	}
	// test for header files
	if ((nFileType == FILETYPE_CLIENTHEADER) ||
		(nFileType == FILETYPE_COMPONENTHEADER) ||
		(nFileType == FILETYPE_OPCODE) ||
		(nFileType == FILETYPE_TEMPLATE) ||
		(nFileType == FILETYPE_COMPONENTIMPLEMENTATION))
	{
		// should only be files
		if (!pFEFile)
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s failed because filetype required CFEFile and it wasn't\n",
				__func__);
			return string();
		}
		// assemble string
		sReturn = sPrefix;
		sReturn += pFEFile->GetFileNameWithoutExtension();
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBENameFactory::%s filename is %s\n", __func__,
			pFEFile->GetFileNameWithoutExtension().c_str());
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
			if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
				sReturn += ".hh";
			else
				sReturn += ".h";
			break;
		case FILETYPE_COMPONENTIMPLEMENTATION:
		case FILETYPE_TEMPLATE:
			if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
				sReturn += ".cc";
			else
				sReturn += ".c";
			break;
		default:
			break;
		}
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s(%s, filetype: %d) = %s (header, opcode)\n",
			__func__, typeid(*pFEBase).name(), nFileType, sReturn.c_str());
		return sReturn;
	}

	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_IDLFILE) ||
		CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
	{
		// filename := [\<prefix\>]\<IDL-file-name\>-(client|server|opcode).c
		// check FE type
		if (!pFEFile)
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s failed because PROGRAM_FILE_IDLFILE/ALL and not CFEFile\n",
				__func__);
			return string();
		}
		// get file-name
		sReturn = sPrefix;
		sReturn += pFEFile->GetFileNameWithoutExtension();
		sReturn += "-client";
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sReturn += ".cc";
		else
			sReturn += ".c";
	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_MODULE))
	{
		// filename := [\<prefix\>]\<libname\>-(client|server).c
		// check FE type
		// can also be interface (if FILE_MODULE, but top level interface)
		if (dynamic_cast<CFEInterface*>(pFEBase))
		{
			return GetFileName(pFEBase, nFileType);
		}
		// else if no lib: return 0
		if (!dynamic_cast<CFELibrary*>(pFEBase))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s failed because PROGRAM_FILE_MODULE and not CFELibrary\n",
				__func__);
			return string();
		}
		// get libname-name
		sReturn = sPrefix;
		CFELibrary *pFELibrary = static_cast<CFELibrary *>(pFEBase);
		// always prefix with IDL filename
		sReturn += pFELibrary->GetSpecificParent<CFEFile>(0)->
			GetFileNameWithoutExtension();
		sReturn += "-";
		sReturn += pFELibrary->GetName();
		sReturn += "-client";
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sReturn += ".cc";
		else
			sReturn += ".c";
	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
	{
		// filename :=
		// [\<prefix\>][\<libname\>_]\<interfacename\>-(client|server).c
		// can also be library (if FILE_INTERFACE, but library with types or
		// constants)
		if (dynamic_cast<CFELibrary*>(pFEBase))
		{
			return GetFileName(pFEBase, nFileType);
		}
		// check FE type
		if (!dynamic_cast<CFEInterface*>(pFEBase))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s failed because PROGRAM_FILE_INTERFACE and not CFEInterface\n",
				__func__);
			return string();
		}
		// get interface name
		sReturn = sPrefix;
		CFEInterface *pFEInterface = (CFEInterface *) pFEBase;
		// always prefix with IDL filename
		sReturn += pFEInterface->GetSpecificParent<CFEFile>(0)->
			GetFileNameWithoutExtension();
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
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sReturn += ".cc";
		else
			sReturn += ".c";

	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION))
	{
		// filename := [\<prefix\>][\<libname\>_][\<interfacename\>_]\<funcname\>-(client|server).c
		// can also be library (if contains types or constants)
		if (dynamic_cast<CFELibrary*>(pFEBase))
		{
			return GetFileName(pFEBase, nFileType);
		}
		// can also be interface (if contains types or constants)
		if (dynamic_cast<CFEInterface*>(pFEBase))
		{
			return GetFileName(pFEBase, nFileType);
		}
		// check FE type
		if (!dynamic_cast<CFEOperation*>(pFEBase))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBENameFactory::%s failed because PROGRAM_FILE_FUNCTION and not CFEOperation\n",
				__func__);
			return string();
		}
		// get class
		sReturn = sPrefix;
		CFEOperation *pFEOperation = (CFEOperation *) pFEBase;
		// always prefix with IDL filename
		sReturn += pFEOperation->GetSpecificParent<CFEFile>(0)->
			GetFileNameWithoutExtension();
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
			sReturn += pFEOperation->GetSpecificParent<CFEInterface>()->
				GetName();
			sReturn += "_";
		}
		// get operation's name
		sReturn += pFEOperation->GetName();
		sReturn += "-client";
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			sReturn += ".cc";
		else
			sReturn += ".c";
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s(%s, filetype: %d) = %s\n", __func__,
		typeid(*pFEBase).name(), nFileType, sReturn.c_str());
	return sReturn;
}

/** \brief get the file name used in an include statement
 *  \param pFEBase the front-end class used to derive the name from
 *  \param nFileType the type of the file
 *  \return a file name suitable for an include statement
 *
 * A file-name suitable for an include statement contains the original
 * relative path used when the file was included into the idl file. If the
 * original file was the top file, there is no difference to the GetFileName
 * function.
 */
string
CBENameFactory::GetIncludeFileName(CFEBase * pFEBase,
	FILE_TYPE nFileType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBENameFactory::%s called\n", __func__);
	// first get the file name as usual
	// adds prefix to non-IDL files
	string sName = GetFileName(pFEBase, nFileType);
	// extract the relative path from the original FE file
	CFEFile *pFEFile = pFEBase->GetSpecificParent<CFEFile>(0);
	// if no IDL file, return original name
	string sOriginalName = pFEFile->GetFileName();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBENameFactory::%s sOriginalName=%s\n", __func__,
		sOriginalName.c_str());
	if (!pFEFile->IsIDLFile())
		return sName;
	// get file name (which contains relative path) and extract it
	// it is everything up to the last '/'
	int nPos = sOriginalName.rfind('/');
	string sPath;
	if (nPos > 0)
		sPath = sOriginalName.substr(0, nPos + 1);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBENameFactory::%s sPath=%s -> sPath + sName = %s\n", __func__,
		sPath.c_str(), (sPath + sName).c_str());
	// concat path and name and return
	return sPath + sName;
}

/** \brief gets the filename used in an include statement if no FE class \
 *         available
 *  \param sBaseName the name of the original include statement
 *  \return the new name in the include statement
 *
 * We treat the file as non-IDL file. Otherwise there would have been a FE
 * class to use as reference.
 */
string
CBENameFactory::GetIncludeFileName(string sBaseName)
{
	// get file-name
	string sReturn;
	CCompiler::GetBackEndOption(string("file-prefix"), sReturn);
	sReturn += sBaseName;
	// deliver filename
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBENameFactory::%s = %s (!IDL file)\n",
		__func__, sReturn.c_str());
	return sReturn;
}

/** \brief creates the name of a type
 *  \param nType the type of the type (char, int, bool, ...)
 *  \param bUnsigned true if the wished type name should be for an unsigned type
 *  \param nSize the size of the type (long int, ...)
 *  \return the string describing the type
 *
 * This function returns the C representations of the given types.
 */
string
CBENameFactory::GetCORBATypeName(int nType,
	bool bUnsigned,
	int nSize)
{
	string sReturn;
	switch (nType)
	{
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
	case TYPE_OCTET:        // 8-bit type, which will never be converted
		sReturn = "CORBA_char";
		break;
	default:
		{
			std::ostringstream os;
			os << nType;
			sReturn = "_UNDEFINED_def_" + os.str();
		}
		break;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s Generated type \"%s\" for code %d\n",
		__func__, sReturn.c_str(), nType);
	return sReturn;
}

/** \brief creates the C-style name of a type
 *  \param nType the type of the type (char, int, bool, ...)
 *  \param bUnsigned true if the wished type name should be for an unsigned type
 *  \param nSize the size of the type (long int, ...)
 *  \return the string describing the type
 *
 * This function skips types, which it won't provide names for. This way the
 * GetTypeName function will set the name for it.
 */
string
CBENameFactory::GetTypeName(int nType,
	bool bUnsigned,
	int nSize)
{
	string sReturn;
	if (CCompiler::IsOptionSet(PROGRAM_USE_CORBA_TYPES))
		sReturn = GetCORBATypeName(nType, bUnsigned, nSize);
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
			sReturn = "unsigned char";
			break;
		case 2:
			if (bUnsigned)
				sReturn = "unsigned short";
			else
				sReturn = "short";
			break;
		case 4:
		case 0:
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
	case TYPE_MWORD:
		sReturn = "unsigned long";
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
	case TYPE_WCHAR:
		if (bUnsigned)
			sReturn = "unsigned short";
		else
			sReturn = "short";
		break;
	case TYPE_WSTRING:
		sReturn = "short";
		break;
	case TYPE_BOOLEAN:
		sReturn = "unsigned char";
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
		sReturn = "struct";
		break;
	case TYPE_IDL_UNION:
	case TYPE_UNION:
		sReturn = "union";
		break;
	case TYPE_ENUM:
		sReturn = "enum";
		break;
	case TYPE_PIPE:
		sReturn = "_UNDEFINED_1";
		break;
	case TYPE_HANDLE_T:
		sReturn = "handle_t";
		break;
	case TYPE_OCTET:        // 8-bit type, which will never be converted
		sReturn = "char";
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
		sReturn.clear();
		break;
	case TYPE_EXCEPTION:
		sReturn = "dice_CORBA_exception_type";
		break;
	default:
		{
			std::ostringstream os;
			os << nType;
			sReturn = "_UNDEFINED_def_" + os.str();
		}
		break;
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s: generated type name \"%s\" for type code %d\n",
		__func__, sReturn.c_str(), nType);
	return sReturn;
}

/** \brief creates the name of a function
 *  \param pFEOperation the function to create a name for
 *  \param nFunctionType the type of the function
 *  \return the name of the back-end function
 *
 * The name has to be unique, therefore it is build using parent interfaces
 * and libraries.  The name looks like this:
 *
 * [\<library name\>_]\<interface name\>_\<operation name\>
 *
 * The function type determines the ending:
 * - FUNCTION_SEND: "_send"
 * - FUNCTION_RECV: "_recv"
 * - FUNCTION_WAIT: "_wait"
 * - FUNCTION_UNMARSHAL:  "_unmarshal"   (10)
 * - FUNCTION_MARSHAL:  "_marshal"       (8)
 * - FUNCTION_MARSHAL_EXCEPTION:  "_marshal_exc"       (12)
 * - FUNCTION_REPLY_RECV: "_reply_recv"  (11)
 * - FUNCTION_REPLY_WAIT: "_reply_wait"  (11)
 * - FUNCTION_CALL: "_call"
 * - FUNCTION_SWITCH_CASE: "_call"
 * - FUNCTION_TEMPLATE:   "_component"   (10)
 * - FUNCTION_REPLY: "_reply"
 *
 * The three other function types should not be used with this implementation,
 * because these are interface functions.  If they are (accidentally) used
 * here, we redirect the call to the interface function naming implementation.
 * - FUNCTION_WAIT_ANY:   "_wait_any"    (9)
 * - FUNCTION_RECV_ANY:   "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP:   "_server_loop" (12)
 * - FUNCTION_DISPATCH:   "_dispatch"    (9)
 */
string
CBENameFactory::GetFunctionName(CFEOperation * pFEOperation,
	FUNCTION_TYPE nFunctionType, bool bComponentSide)
{
	if (!pFEOperation)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s failed because the operation is 0\n",
			__func__);
		return string();
	}
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CFELibrary *pFELibrary = pFEOperation->GetSpecificParent<CFELibrary>();
	// check for interface functions
	if ((nFunctionType == FUNCTION_WAIT_ANY) ||
		(nFunctionType == FUNCTION_RECV_ANY) ||
		(nFunctionType == FUNCTION_SRV_LOOP) ||
		(nFunctionType == FUNCTION_DISPATCH) ||
		(nFunctionType == FUNCTION_REPLY_WAIT))
		return GetFunctionName(pFEInterface, nFunctionType, bComponentSide);

	string sReturn;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		while (pFELibrary)
		{
			sReturn = pFELibrary->GetName() + "_" + sReturn;
			pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
		}
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
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
			sReturn += "_call";
		break;
	case FUNCTION_UNMARSHAL:
		sReturn += "_unmarshal";
		break;
	case FUNCTION_MARSHAL:
		sReturn += "_marshal";
		break;
	case FUNCTION_MARSHAL_EXCEPTION:
		sReturn += "_marshal_exc";
		break;
	case FUNCTION_TEMPLATE:
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
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

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s(%s, functiontype:%d) = %s\n",
		__func__, pFEOperation->GetName().c_str(), nFunctionType,
		sReturn.c_str());
	return sReturn;
}

/** \brief creates the name of a function
 *  \param pFEInterface the interface to create a name for
 *  \param nFunctionType the type of the function
 *  \return the name of the interface's function
 *
 * This implementation creates function names for interface functions. The
 * name looks like this:
 *
 * [\<library name\>_]\<interface name\>
 *
 * The function type determines which ending is added to the above string
 * - FUNCTION_WAIT_ANY: "_wait_any"    (9)
 * - FUNCTION_RECV_ANY: "_recv_any"    (9)
 * - FUNCTION_SRV_LOOP: "_server_loop" (12)
 * - FUNCTION_DISPATCH: "_dispatch"    (9)
 * - FUNCTION_REPLY_WAIT: "_reply_and_wait"  (15)
 */
string
CBENameFactory::GetFunctionName(CFEInterface * pFEInterface,
	FUNCTION_TYPE nFunctionType, bool bComponentSide)
{
	if (!pFEInterface)
		return string();

	string sReturn;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		CFELibrary *pFELibrary = pFEInterface->GetSpecificParent<CFELibrary>();
		while (pFELibrary)
		{
			sReturn = pFELibrary->GetName() + "_" + sReturn;
			pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
		}
	}
	else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		sReturn += "_dice_";
	sReturn += pFEInterface->GetName();
	switch (nFunctionType)
	{
	case FUNCTION_WAIT_ANY:
		if (bComponentSide)
			sReturn += "_srv";
		else
			sReturn += "_clt";
		sReturn += "_wait_any";
		break;
	case FUNCTION_RECV_ANY:
		if (bComponentSide)
			sReturn += "_srv";
		else
			sReturn += "_clt";
		sReturn += "_recv_any";
		break;
	case FUNCTION_SRV_LOOP:
		sReturn += "_server_loop";
		break;
	case FUNCTION_DISPATCH:
		sReturn += "_dispatch";
		break;
	case FUNCTION_REPLY_WAIT:
		sReturn += "_reply_and_wait";
		break;
	default:
		break;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s(%s, functiontype:%d) = %s\n",
		__func__, pFEInterface->GetName().c_str(), nFunctionType,
		sReturn.c_str());
	return sReturn;
}

/** \brief creates a unique define label for a header file name
 *  \param sFilename the filename to create the define for
 *  \return a unique define label
 *
 * To build a unique define label from a file name there is not much to do -
 * to file name is unique itself.  To make it look fancy we simply prefix and
 * suffix the file name with two underscores and replace all "nonconforming
 * characters" with underscores. Because define labale commonly are uppercase,
 * we do that as well
 */
string
CBENameFactory::GetHeaderDefine(string sFilename)
{
	if (sFilename.empty())
		return string();

	// add underscores
	string sReturn;
	sReturn = "__" + sFilename + "__";
	// make uppercase
	transform(sReturn.begin(), sReturn.end(), sReturn.begin(), _toupper);
	// replace "nonconforming characters"
	string::size_type pos;
	while ((pos = sReturn.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) != string::npos)
		sReturn[pos] = '_';

	return sReturn;
}

/** \brief generates a define symbol to brace the definition of a type
 *  \param sTypedefName the name of the typedef to brace
 *  \return the define symbol
 *
 * Add to the type's name two underscores on either side and the "typedef_"
 * string. Then remove "nonconforming characters" and make the string
 * uppercase.
 */
string CBENameFactory::GetTypeDefine(string sTypedefName)
{
	if (sTypedefName.empty())
		return string();

	// add underscores
	string sReturn;
	sReturn = "__typedef_" + sTypedefName + "__";
	// make uppercase
	transform(sReturn.begin(), sReturn.end(), sReturn.begin(), _toupper);
	// replace "nonconforming characters"
	string::size_type pos;
	while ((pos = sReturn.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) != string::npos)
		sReturn[pos] = '_';

	return sReturn;
}

/** \brief generates a variable name for the return variable of a function
 *  \return a variable name for the return variable
 */
string CBENameFactory::GetReturnVariable()
{
	return string("_dice_return");
}

/** \brief generates a variable name for the opcode variable
 *  \return a variable name for the opcode variable
 */
string CBENameFactory::GetOpcodeVariable()
{
	return string("_dice_opcode");
}

/** \brief generates a variable name for the server loop reply code variable
 *  \return a variable name for the reply code variable
 */
string CBENameFactory::GetReplyCodeVariable()
{
	return string("_dice_reply");
}

/** \brief generates the variable name for the return variable of the server loop
 *  \return a variable name for the return variable
 */
string CBENameFactory::GetSrvReturnVariable()
{
	return string("_dice_srv_return");
}

/** \brief generate the constant name of the function's opcode
 *  \param pFunction the operation to generate the opcode for
 *  \return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [\<library name\>_]\<interface name\>_\<function name\>_opcode
 */
string CBENameFactory::GetOpcodeConst(CBEFunction * pFunction)
{
	if (!pFunction)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s failed because function is 0\n", __func__);
		return string();
	}

	string sReturn;
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
	// if pClass != 0 search for FE function
	if (pClass)
	{
		CFunctionGroup *pGroup = pClass->FindFunctionGroup(pFunction);
		if (pGroup)
		{
			return GetOpcodeConst(pGroup->GetOperation());
		}
	}

	CBENameSpace *pNameSpace = (pClass) ?
		pClass->GetSpecificParent<CBENameSpace>() : 0;
	if (pNameSpace)
		sReturn += (pNameSpace->GetName()) + "_";
	if (pClass)
		sReturn += (pClass->GetName()) + "_";
	sReturn += pFunction->GetName() + "_opcode";
	// make upper case
	transform(sReturn.begin(), sReturn.end(), sReturn.begin(), _toupper);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBENameFactory::%s(BE: %s) = %s\n", __func__,
		pFunction->GetName().c_str(), sReturn.c_str());
	return sReturn;
}

/** \brief generate the constant name of the function's opcode
 *  \param pFEOperation the operation to generate the opcode for
 *  \param bSecond true if a second opcode const name is required
 *  \return a string containing the opcode constant name
 *
 * A opcode constant is usually named:
 * [\<library name\>_]\<interface name\>_\<function name\>_opcode
 */
string CBENameFactory::GetOpcodeConst(CFEOperation * pFEOperation, bool bSecond)
{
	if (!pFEOperation)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s failed because FE function is 0\n",
			__func__);
		return string();
	}

	string sReturn;
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CFELibrary *pFELibrary = (pFEInterface) ?
		pFEInterface->GetSpecificParent<CFELibrary>() : 0;
	if (pFELibrary)
		sReturn += (pFELibrary->GetName()) + "_";
	if (pFEInterface)
		sReturn += (pFEInterface->GetName()) + "_";
	sReturn += pFEOperation->GetName() + "_opcode";
	if (bSecond)
		sReturn += "_2";
	// make upper case
	transform(sReturn.begin(), sReturn.end(), sReturn.begin(), _toupper);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s(FE: %s) = %s\n", __func__,
		pFEOperation->GetName().c_str(), sReturn.c_str());
	return sReturn;
}

/** \brief generate the constant name of the interface's base opcode
 *  \param pClass the interface to generate the opcode for
 *  \return a string containing the base-opcode constant name
 *
 * A opcode constant is usually named:
 * [\<library name\>_]\<interface name\>_base_opcode
 */
string CBENameFactory::GetOpcodeConst(CBEClass * pClass)
{
	if (!pClass)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBENameFactory::%s failed because class is 0\n", __func__);
		return string();
	}

	string sReturn;
	CBENameSpace *pNameSpace = pClass->GetSpecificParent<CBENameSpace>();
	if (pNameSpace)
		sReturn += (pNameSpace->GetName()) + "_";
	sReturn += pClass->GetName() + "_base_opcode";
	// make upper case
	transform(sReturn.begin(), sReturn.end(), sReturn.begin(), _toupper);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBENameFactory::%s(C: %s) = %s\n", __func__,
		pClass->GetName().c_str(), sReturn.c_str());
	return sReturn;
}

/** \brief generates the inline prefix
 *  \return a string containing the prefix (usually "inline")
 *
 * Default to "static inline" to avoid warnings about non-defined previous
 * prototype. This does not allow to use the function where the header is not
 * included but that can be fixed by using -i extern.
 *
 * For C++ only return inline for externally used functions.
 */
string CBENameFactory::GetInlinePrefix()
{
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return string("inline");
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_INLINE_EXTERN))
		return string("extern inline");
	else if (CCompiler::IsOptionSet(PROGRAM_GENERATE_INLINE_STATIC))
		return string("static inline");
	else
		return string("inline");
}

/** \brief general function for accessing strings of derived name factories
 *  \param nStringCode the identifier of the requested string
 *  \param pParam additional unknown parameter
 *  \return the requested string
 *
 * If using an instance of this class, the explicit functions cover all
 * requested strings.  This functions should only be accessed in derived name
 * factories.
 */
string CBENameFactory::GetString(int /*nStringCode*/,
	void* /*pParam*/)
{
	return string();
}

/** \brief generates the variable name for the CORBA_object parameter
 *  \return the name of the variable
 */
string CBENameFactory::GetCorbaObjectVariable()
{
	return string("_dice_corba_obj");
}

/** \brief generates the variable name for the CORBA_environment parameter
 *  \return the name of the variable
 */
string CBENameFactory::GetCorbaEnvironmentVariable()
{
	return string("_dice_corba_env");
}

/** \brief generates the variable name for a message buffer
 *  \return the name of the variable
 */
string CBENameFactory::GetMessageBufferVariable()
{
	return string("_dice_msg_buffer");
}

/** \brief generates the name of the message buffer's type
 *  \param pFEInterface the interface this name is for
 *  \return the name of the type
 */
string CBENameFactory::GetMessageBufferTypeName(CFEInterface * pFEInterface)
{
	return GetMessageBufferTypeName(pFEInterface->GetName());
}

/** \brief generates a name of a function's message buffer type
 *  \param pFEOperation the operation this name is for
 *  \return the name of the type
 */
string CBENameFactory::GetMessageBufferTypeName(CFEOperation * pFEOperation)
{
	CFEInterface *pFEInterface = pFEOperation->GetSpecificParent<CFEInterface>();
	string sBase = pFEInterface->GetName() + "_" + pFEOperation->GetName();
	return GetMessageBufferTypeName(sBase);
}

/** \brief generates the name of the message buffer's type
 *  \param sInterfaceName the name of the interface this message buffer is for
 *  \return the name of the type;
 */
string CBENameFactory::GetMessageBufferTypeName(string sInterfaceName)
{
	string sBase = GetMessageBufferTypeName();
	if (sInterfaceName.empty())
		return string("dice_") + sBase;

	return sInterfaceName + string("_") + sBase;
}

/** \brief generates the name of the message buffer's type
 *  \return the base name of the message buffer type
 *
 * This function is used internally to get the "base" of the type name _and_
 * its used if the the name is later altered by teh calling function (e.g. add
 * scopes, etc.)
 */
string CBENameFactory::GetMessageBufferTypeName()
{
	return "msg_buffer_t";
}

/** \brief generates the variable of the client side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 */
string CBENameFactory::GetTimeoutClientVariable(CBEFunction* /* pFunction */)
{
	return string("timeout");
}

/** \brief generates the variable of the component side timeout
 *  \param pFunction the function needing this variable
 *  \return the name of the variable
 */
string CBENameFactory::GetTimeoutServerVariable(CBEFunction* /* pFunction */)
{
	return string("timeout");
}

/** \brief generates the variable of the client side scheduling options
 *  \return the name of the variable
 */
string CBENameFactory::GetScheduleClientVariable()
{
	return string();
}

/** \brief generates the variable of the server side scheduling options
 *  \return the name of the variable
 */
string CBENameFactory::GetScheduleServerVariable()
{
	return string();
}

/** \brief generates the variable containing the component identifier
 *  \return  the name of the variable
 */
string CBENameFactory::GetComponentIDVariable()
{
	return string("componentID");
}

/** \brief generates the message buffers member for a specific type
 *  \param nFEType  the type fot which the member is searched
 *  \return the name of the message buffer's member responsible for the type
 */
string CBENameFactory::GetMessageBufferMember(int nFEType)
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
	case TYPE_STRING:
	case TYPE_WSTRING:
	case TYPE_ARRAY:
		sReturn = "_UNDEFINED_member";
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
 *  \return the name of the variable
 */
string CBENameFactory::GetOffsetVariable()
{
	return string("_dice_offset");
}

/** \brief generates the variable name of a temporary offset variable
 *  \return the name of the variable
 */
string CBENameFactory::GetTempOffsetVariable()
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

/** \brief generates the name of a server class internal variable
 *  \return a variable name
 */
string CBENameFactory::GetServerVariable()
{
	return string("_dice_server");
}

/** \brief get a scoped name for a tagged declarator or typedef
 *  \param pFERefType the class to use as reference in the hierarchy
 *  \param sName the name to scope
 *  \return the scoped name
 *
 * This function is used to generate a suitable name for a tagged decl,
 * typedef or type declaration with a flat namespace.
 */
string CBENameFactory::GetTypeName(CFEBase *pFERefType, string sName)
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
 *  \return the scoped named
 */
string CBENameFactory::GetConstantName(CFEConstDeclarator* pFEConstant)
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
 *  \param sPrefix a prefix for the dummy variable
 *  \return the name of the dummy variable
 *
 * We could keep a counter in the name-factory and increment it on every
 * invocation of this function, but then we could not connect the usage of a
 * dummy variable to the declaration.
 */
string CBENameFactory::GetDummyVariable(string sPrefix)
{
	return sPrefix + string("dummy");
}

/** \brief returns the variable name of a exception word variable
 *  \return the string _exception
 */
string CBENameFactory::GetExceptionWordVariable()
{
	return string("_exception");
}

/** \brief returns the variable name of a user defined exception
 *  \param sTypeName the name of the exception time
 *  \return the name of the variable
 */
string CBENameFactory::GetUserExceptionVariable(string sTypeName)
{
	return string("_exc_") + sTypeName;
}

/** \brief returns the name of the message buffer struct member
 *  \param nType the transfer direction of the struct
 *  \param sFuncName if not empty the operation's name should be added
 *  \param sClassName if not empty the class's name should be added
 *  \return the name of the union member
 */
string CBENameFactory::GetMessageBufferStructName(CMsgStructType nType,
	string sFuncName, string sClassName)
{
	string sReturn;
	assert((sFuncName.empty() && sClassName.empty()) ||
		(!sFuncName.empty() && !sClassName.empty()));
	if (!sClassName.empty())
		sReturn = sClassName;
	if (!sClassName.empty() && !sFuncName.empty())
		sReturn += "_";
	if (!sFuncName.empty())
		sReturn += sFuncName;
	if (CMsgStructType::In == nType)
		sReturn += "_in";
	else if (CMsgStructType::Out == nType)
		sReturn += "_out";
	else if (CMsgStructType::Exc == nType)
		sReturn += "_exc";
	else // generic
		sReturn += "_word";
	return sReturn;
}

/** \brief returns the name of a word sized member of the message buffer
 *  \return the generated name
 */
string
CBENameFactory::GetWordMemberVariable()
{
	return string("_word");
}

/** \brief returns the name of a word sized member of the message buffer
 *  \param nNumber the number of the member (zero based index)
 *  \return the generated name
 */
string
CBENameFactory::GetWordMemberVariable(int nNumber)
{
	std::ostringstream os;
	string sReturn = GetWordMemberVariable();
	os << nNumber;
	return sReturn + os.str();
}

/** \brief get the name for a local variable which is the size variable of
 *         some parameter
 *  \param pStack the declarator stack containing the parameter to get the
 *         local variable for
 *  \return the name of the size variable
 */
string
CBENameFactory::GetLocalSizeVariableName(
	CDeclStack* pStack)
{
	string sReturn = GetLocalVariableName(pStack);
	sReturn += "_size";
	return sReturn;
}

/** \brief get the name for a local variable using a declarator stack
 *  \param pStack the declarator stack pointing to the current loc of the var
 *  \return the name of the local variable
 */
string
CBENameFactory::GetLocalVariableName(
	CDeclStack* pStack)
{
	string sReturn = string("_dice");
	assert(pStack);
	CDeclStack::iterator iter = pStack->begin();
	for (; iter != pStack->end(); iter++)
		sReturn += string("_") + iter->pDeclarator->GetName();
	return sReturn;
}

/** \brief get the name for the padding member
 *  \param nPadType the type of the padding member
 *  \param nPadToType the type of the member to insert the padding before
 *  \return the name of this member
 */
string
CBENameFactory::GetPaddingMember(int nPadType,
	int nPadToType)
{
	string sReturn = string("_dice_pad_");
	string sType = GetTypeName(nPadToType, false);
	sReturn += sType;
	sReturn += "_with_";
	sType = GetTypeName(nPadType, false);
	sReturn += sType;
	// replace spaces in type names
	replace_if(sReturn.begin(), sReturn.end(), ::isspace, '_');
	return sReturn;
}

/** \brief get a prefix for the parameters of a C++ wrapper function
 *  \return the prefix
 */
string
CBENameFactory::GetWrapperVariablePrefix()
{
	return string("_dice_wrap_");
}
