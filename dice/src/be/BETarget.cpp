/**
 *  \file    dice/src/be/BETarget.cpp
 *  \brief   contains the implementation of the class CBETarget
 *
 *  \date    01/11/2002
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

#include "be/BETarget.h"
#include "be/BENameSpace.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEWaitFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEConstant.h"
#include "be/BETypedef.h"
#include "be/BEFunction.h"
#include "be/BERoot.h"
#include "be/BEDeclarator.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "Compiler.h"
#include <cassert>
#include <iostream>

CBETarget::CBETarget()
: m_HeaderFiles(0, this),
  m_ImplementationFiles(0, this)
{
}

CBETarget::CBETarget(CBETarget & src)
: CBEObject(src),
  m_HeaderFiles(src.m_HeaderFiles),
  m_ImplementationFiles(src.m_ImplementationFiles)
{
    m_HeaderFiles.Adopt(this);
    m_ImplementationFiles.Adopt(this);
}

/** \brief destructor of target class */
CBETarget::~CBETarget()
{ }

/** \brief writes the header files
 */
void CBETarget::WriteHeaderFiles()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    vector<CBEHeaderFile*>::iterator iter;
    for (iter = m_HeaderFiles.begin();
	 iter != m_HeaderFiles.end();
	 iter++)
    {
        (*iter)->Write();
    }
}

/** \brief writes the implementation files
 */
void CBETarget::WriteImplementationFiles()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    vector<CBEImplementationFile*>::iterator iter;
    for (iter = m_ImplementationFiles.begin();
	 iter != m_ImplementationFiles.end();
	 iter++)
    {
        (*iter)->Write();
    }
}

/** \brief adds the constant of the front-end file to the back-end file
 *  \param pFile the back-end file
 *  \param pFEFile the front-end file
 *  \return true if successful
 *
 * This implementation iterates over the interfaces and libs of the current
 * file. It also iterates over the included files if the program options allow
 * it. And the implementation also call AddConstantToFile for all constants of
 * the file.
 */
bool CBETarget::AddConstantToFile(CBEFile& pFile, CFEFile * pFEFile)
{
    if (!pFEFile)
    {
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETarget::%s aborted because front-end file is 0\n", __func__);
        return true;
    }
    if (!pFEFile->IsIDLFile())
    {
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETarget::%s aborted because front-end file is not IDL file\n",
	    __func__);
        return true;
    }

    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFEFile->m_Interfaces.begin();
	 iterI != pFEFile->m_Interfaces.end();
	 iterI++)
    {
        if (!AddConstantToFile(pFile, *iterI))
            return false;
    }

    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFEFile->m_Libraries.begin();
	 iterL != pFEFile->m_Libraries.end();
	 iterL++)
    {
        if (!AddConstantToFile(pFile, *iterL))
            return false;
    }

    vector<CFEConstDeclarator*>::iterator iterC;
    for (iterC = pFEFile->m_Constants.begin();
	 iterC != pFEFile->m_Constants.end();
	 iterC++)
    {
        if (!AddConstantToFile(pFile, *iterC))
            return false;
    }

    if (DoAddIncludedFiles())
    {
	vector<CFEFile*>::iterator iterF;
	for (iterF = pFEFile->m_ChildFiles.begin();
	    iterF != pFEFile->m_ChildFiles.end();
	    iterF++)
	{
	    if (!AddConstantToFile(pFile, *iterF))
		return false;
	}
    }

    return true;
}

/** \brief adds the constants of the front-end library to the back-end file
 *  \param pFile the back-end file
 *  \param pFELibrary the front-end library
 *  \return true if successful
 *
 * This implementation iterates over the interfaces and nested libs of the
 * current library.  It also calls AddConstantToFile for every constant of the
 * library.
 */
bool CBETarget::AddConstantToFile(CBEFile& pFile, CFELibrary * pFELibrary)
{
    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFELibrary->m_Interfaces.begin();
	 iterI != pFELibrary->m_Interfaces.end();
	 iterI++)
    {
        if (!AddConstantToFile(pFile, *iterI))
            return false;
    }

    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFELibrary->m_Libraries.begin();
	 iterL != pFELibrary->m_Libraries.end();
	 iterL++)
    {
        if (!AddConstantToFile(pFile, *iterL))
            return false;
    }

    vector<CFEConstDeclarator*>::iterator iterC;
    for (iterC = pFELibrary->m_Constants.begin();
	 iterC != pFELibrary->m_Constants.end();
	 iterC++)
    {
        if (!AddConstantToFile(pFile, *iterC))
            return false;
    }

    return true;
}

/** \brief adds the constants of the front-end interface to the back-end file
 *  \param pFile the back-end file
 *  \param pFEInterface the front-end interface
 *  \return true if successful
 *
 * This implementation iterates over the constants of the current interface
 */
bool CBETarget::AddConstantToFile(CBEFile& pFile, CFEInterface * pFEInterface)
{
    if (!pFEInterface)
    {
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "CBETarget::%s (interface) aborted because FE interface is 0\n",
	    __func__);
        return true;
    }

    vector<CFEConstDeclarator*>::iterator iter;
    for (iter = pFEInterface->m_Constants.begin();
	 iter != pFEInterface->m_Constants.end();
	 iter++)
    {
        if (!AddConstantToFile(pFile, *iter))
            return false;
    }

    return true;
}

/** \brief adds the constants of the front-end to the back-end file
 *  \param pFile the file to add the function to
 *  \param pFEConstant the constant to generate the back-end constant from
 *  \return true if generation went alright
 *
 * This implementation generates a back-end constant and adds it to the header file.
 */
bool CBETarget::AddConstantToFile(CBEFile& pFile, CFEConstDeclarator * pFEConstant)
{
    try
    {
	CBEHeaderFile& pF = dynamic_cast<CBEHeaderFile&>(pFile);
        CBERoot *pRoot = GetSpecificParent<CBERoot>();
        assert(pRoot);
        CBEConstant *pConstant = pRoot->FindConstant(pFEConstant->GetName());
        if (!pConstant)
        {
            pConstant = CCompiler::GetClassFactory()->GetNewConstant();
            pF.m_Constants.Add(pConstant);
	    pConstant->CreateBackEnd(pFEConstant);
        }
        else
            pF.m_Constants.Add(pConstant);
    }
    catch (std::bad_cast)
    { } // not a header file

    return true;
}

/** \brief adds the type definition of the front-end file to the back-end file
 *  \param pFile the back-end file
 *  \param pFEFile the front-end file
 *  \return true if successful
 *
 * This implementation iterates over the interfaces and libs of the current file. It
 * also iterates over the included files if the program options allow it. And it calls
 * AddTypedefToFile for the type definitions of the file
 */
bool CBETarget::AddTypedefToFile(CBEFile& pFile, CFEFile * pFEFile)
{
    if (!pFEFile)
        return true;
    if (!pFEFile->IsIDLFile())
        return true;

    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFEFile->m_Interfaces.begin();
	 iterI != pFEFile->m_Interfaces.end();
	 iterI++)
    {
        if (!AddTypedefToFile(pFile, *iterI))
            return false;
    }

    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFEFile->m_Libraries.begin();
	 iterL != pFEFile->m_Libraries.end();
	 iterL++)
    {
        if (!AddTypedefToFile(pFile, *iterL))
            return false;
    }

    vector<CFETypedDeclarator*>::iterator iterT;
    for (iterT = pFEFile->m_Typedefs.begin();
	 iterT != pFEFile->m_Typedefs.end();
	 iterT++)
    {
        if (!AddTypedefToFile(pFile, *iterT))
            return false;
    }

    if (DoAddIncludedFiles())
    {
	vector<CFEFile*>::iterator iterF;
	for (iterF = pFEFile->m_ChildFiles.begin();
	    iterF != pFEFile->m_ChildFiles.end();
	    iterF++)
	{
	    if (!AddTypedefToFile(pFile, *iterF))
		return false;
        }
    }

    return true;
}

/** \brief adds the type definitions of the front-end library to the back-end file
 *  \param pFile the back-end file
 *  \param pFELibrary the front-end library
 *  \return true if successful
 *
 * This implementation iterates over the interfaces and nested libs of the
 * current library.  And it calls AddTypedefToFile for the type definitions in
 * the library.
 */
bool CBETarget::AddTypedefToFile(CBEFile& pFile, CFELibrary * pFELibrary)
{
    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFELibrary->m_Interfaces.begin();
	 iterI != pFELibrary->m_Interfaces.end();
	 iterI++)
    {
        if (!AddTypedefToFile(pFile, *iterI))
            return false;
    }

    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFELibrary->m_Libraries.begin();
	 iterL != pFELibrary->m_Libraries.end();
	 iterL++)
    {
        if (!AddTypedefToFile(pFile, *iterL))
            return false;
    }

    vector<CFETypedDeclarator*>::iterator iterT;
    for (iterT = pFELibrary->m_Typedefs.begin();
	 iterT != pFELibrary->m_Typedefs.end();
	 iterT++)
    {
        if (!AddTypedefToFile(pFile, *iterT))
            return false;
    }

    return true;
}

/** \brief adds the type definitions of the front-end interface to the back-end file
 *  \param pFile the back-end file
 *  \param pFEInterface the front-end interface
 *  \return true if successful
 *
 * This implementation iterates over the type definitions of the current
 * interface.  It also adds one message buffer per interface to the header
 * file.
 */
bool CBETarget::AddTypedefToFile(CBEFile& pFile, CFEInterface * pFEInterface)
{
    vector<CFETypedDeclarator*>::iterator iter;
    for (iter = pFEInterface->m_Typedefs.begin();
	 iter != pFEInterface->m_Typedefs.end();
	 iter++)
    {
        if (!AddTypedefToFile(pFile, *iter))
            return false;
    }

    return true;
}

/** \brief adds the type definitions of the front-end to the back-end file
 *  \param pFile the file to add the function to
 *  \param pFETypedDeclarator the typedef to generate the back-end typedef from
 *  \return true if generation went alright
 *
 * This implementation adds the type definition to the header file, but skips the implementation file.
 * It searches for the typedef at the root and then adds a reference to its own collection.
 */
bool CBETarget::AddTypedefToFile(CBEFile& pFile,
                 CFETypedDeclarator * pFETypedDeclarator)
{
    try
    {
	CBEHeaderFile& pF = dynamic_cast<CBEHeaderFile&>(pFile);
        CBERoot *pRoot = GetSpecificParent<CBERoot>();
        assert(pRoot);
        CFEDeclarator *pDecl = pFETypedDeclarator->m_Declarators.First();
        CBETypedef *pTypedef = pRoot->FindTypedef(pDecl->GetName());
        assert(pTypedef);
        pF.m_Typedefs.Add(pTypedef);
    }
    catch (std::bad_cast)
    { } // not a header file
    return true;
}

/** \brief finds a header file which belongs to a certain front-end file
 *  \param pFEFile the front-end file
 *  \param nFileType the type of file to find (client/server)
 *  \return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a
 * file-name based on the front-end file and this name is used to find the
 * file.
 */
CBEHeaderFile* 
CBETarget::FindHeaderFile(CFEFile * pFEFile,
    FILE_TYPE nFileType)
{
    // get file name
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sFileName = pNF->GetFileName(pFEFile, nFileType);
    // search file
    return m_HeaderFiles.Find(sFileName);
}

/** \brief finds a header file which belongs to a certain front-end library
 *  \param pFELibrary the front-end library
 *  \param nFileType the filetype of the header file
 *  \return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a
 * file-name based on the front-end library and this name is used to find the
 * file.
 */
CBEHeaderFile* 
CBETarget::FindHeaderFile(CFELibrary * pFELibrary,
    FILE_TYPE nFileType)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // get file name
    CFEFile *pFEFile = pFELibrary->GetSpecificParent<CFEFile>(0);
    string sFileName = pNF->GetFileName(pFEFile, nFileType);
    // search file
    return m_HeaderFiles.Find(sFileName);
}

/** \brief finds a header file which belongs to a certain front-end interface
 *  \param pFEInterface the front-end interface
 *  \param nFileType the filetype of the header file (client, component)
 *  \return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a
 * file-name based on the front-end interface and this name is used to find
 * the file.
 */
CBEHeaderFile* 
CBETarget::FindHeaderFile(CFEInterface * pFEInterface,
    FILE_TYPE nFileType)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // get file name
    CFEFile *pFEFile = pFEInterface->GetSpecificParent<CFEFile>(0);
    string sFileName = pNF->GetFileName(pFEFile, nFileType);
    // search file
    return m_HeaderFiles.Find(sFileName);
}

/** \brief finds a header file which belongs to a certain front-end operation
 *  \param pFEOperation the front-end operation
 *  \param nFileType the type of the header file
 *  \return a reference to the respective file; 0 if not found
 *
 * The search uses the file name to find a file. The name-factory generates a
 * file-name based on the front-end operation and this name is used to find
 * the file.
 */
CBEHeaderFile* 
CBETarget::FindHeaderFile(CFEOperation * pFEOperation,
    FILE_TYPE nFileType)
{
    // get file name
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    CFEFile *pFEFile = pFEOperation->GetSpecificParent<CFEFile>(0);
    string sFileName = pNF->GetFileName(pFEFile, nFileType);
    // search file
    return m_HeaderFiles.Find(sFileName);
}

/** \brief tries to find the typedef with a type of the given name
 *  \param sTypeName the name to search for
 *  \return a reference to the searched typedef or 0
 *
 * To find a typedef, we iterate over the header files and check them.
 * (Implementation file cannot contain typedefs).
 */
CBETypedef *CBETarget::FindTypedef(string sTypeName)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETarget::%s(%s) called\n",
	__func__, sTypeName.c_str());

    CBETypedef *pRet = 0;
    vector<CBEHeaderFile*>::iterator iter;
    for (iter = m_HeaderFiles.begin();
	 iter != m_HeaderFiles.end();
	 iter++)
    {
        pRet = (*iter)->m_Typedefs.Find(sTypeName);
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBETarget::%s returns %p\n",
	__func__, pRet);
    return pRet;
}

/** \brief tries to find the function with the given function name
 *  \param sFunctionName the name to search for
 *  \param nFunctionType the type of the function to find
 *  \return a reference to the found function or 0 if not found
 *
 * To find a function, we iterate over the header files and the implementation
 * files.
 */
CBEFunction *CBETarget::FindFunction(string sFunctionName,
    FUNCTION_TYPE nFunctionType)
{
    CBEFunction *pRet = 0;
    // search header files
    vector<CBEHeaderFile*>::iterator iterH;
    for (iterH = m_HeaderFiles.begin();
	 iterH != m_HeaderFiles.end() && !pRet;
	 iterH++)
    {
        pRet = (*iterH)->FindFunction(sFunctionName, nFunctionType);
    }
    // search implementation files
    vector<CBEImplementationFile*>::iterator iterI;
    for (iterI = m_ImplementationFiles.begin();
	 iterI != m_ImplementationFiles.end() && !pRet;
	 iterI++)
    {
        pRet = (*iterI)->FindFunction(sFunctionName, nFunctionType);
    }
    return pRet;
}

/** \brief creates the back-end classes for this target
 *  \param pFEFile is the respective front-end file to use as reference
 *  \return true if successful
 */
void
CBETarget::CreateBackEnd(CFEFile *pFEFile)
{
    // if argument is 0, we assume a mistaken include file
    if (!pFEFile)
    {
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s aborted because front-end file is 0\n",
	    __func__);
        return;
    }
    // if file is not IDL file we simply return "no error", because C files
    // might also be included files
    if (!pFEFile->IsIDLFile())
    {
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s aborted because front-end file (%s) is not IDL file\n", 
	    __func__, pFEFile->GetFileName().c_str());
        return;
    }

    // create header file(s)
    CreateBackEndHeader(pFEFile);
    // create implementation file(s)
    CreateBackEndImplementation(pFEFile);
}

/** \brief prints all generated target file name to the given output
 *  \param output the output stream to write to
 *  \param nCurCol the current column where to start output (indent)
 *  \param nMaxCol the maximum number of columns
 */
void CBETarget::PrintTargetFiles(ostream& output, int &nCurCol, int nMaxCol)
{
    // iterate over implementation files
    vector<CBEHeaderFile*>::iterator iterH;
    for (iterH = m_HeaderFiles.begin();
	 iterH != m_HeaderFiles.end();
	 iterH++)
    {
        PrintTargetFileName(output, (*iterH)->GetFileName(), nCurCol, nMaxCol);
    }
    // iterate over header files
    vector<CBEImplementationFile*>::iterator iterI;
    for (iterI = m_ImplementationFiles.begin();
	 iterI != m_ImplementationFiles.end();
	 iterI++)
    {
        PrintTargetFileName(output, (*iterI)->GetFileName(), nCurCol, nMaxCol);
    }
}

/** \brief prints the current filename
 *  \param output the output stream to print to
 *  \param sFilename the filename to print
 *  \param nCurCol the current output column
 *  \param nMaxCol the maximum output column
 */
void CBETarget::PrintTargetFileName(ostream& output, string sFilename,
    int &nCurCol, int nMaxCol)
{
    nCurCol += sFilename.length();
    if (nCurCol > nMaxCol)
    {
	output << "\\\n ";
        nCurCol = sFilename.length()+1;
    }
    output << sFilename << " ";
}

/** \brief tests if this target has a function with a given type
 *  \param sTypeName the name of the user defined type
 *  \return true if the function has this type
 *
 * The methods searches for a function, which has a parameter of the given
 * user defined type. Since all functions are declared in the header files,
 * we only need to search those.
 */
bool CBETarget::HasFunctionWithUserType(string sTypeName)
{
    vector<CBEHeaderFile*>::iterator iter;
    for (iter = m_HeaderFiles.begin();
	 iter != m_HeaderFiles.end();
	 iter++)
    {
        if ((*iter)->HasFunctionWithUserType(sTypeName))
            return true;
    }
    return false;
}

/** \brief check if we add the object of the included files to this target
 *  \return true if we add the objects of the included files
 *
 * The default implementation tests for PROGRAM_FILE_ALL.
 */
bool CBETarget::DoAddIncludedFiles()
{
    return CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL);
}
