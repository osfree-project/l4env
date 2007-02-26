/**
 *	\file	dice/src/be/BEHeaderFile.cpp
 *	\brief	contains the implementation of the class CBEHeaderFile
 *
 *	\date	01/11/2002
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

#include "be/BEHeaderFile.h"
#include "be/BEContext.h"
#include "be/BEConstant.h"
#include "be/BETypedef.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"
#include "be/BENameSpace.h"
#include "be/BEClass.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEHeaderFile);

CBEHeaderFile::CBEHeaderFile()
: m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedef))
{
    IMPLEMENT_DYNAMIC_BASE(CBEHeaderFile, CBEFile);
}

CBEHeaderFile::CBEHeaderFile(CBEHeaderFile & src)
: CBEFile(src),
  m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedef))
{
    m_sIncludeName = src.m_sIncludeName;
    IMPLEMENT_DYNAMIC_BASE(CBEHeaderFile, CBEFile);
}

/**	\brief destructor
 */
CBEHeaderFile::~CBEHeaderFile()
{

}

/**	\brief prepares the header file for the back-end
 *	\param pFEFile the corresponding front-end file
 *	\param pContext the context of the code generation
 *	\return true if the creation was successful
 *
 * This function should only add the file name and the names of the included files to this instance.
 *
 * If the name of the front-end file included a relative path, this path
 * should be stripped of for the file name of this file. But it should be
 * used when writing include statements. E.g. a file included with #include "l4/test.idl"
 * should get the header file name "test-client.h" or "test-server.h", but should be
 * included using "l4/test-client.h" or "l4/test-server.h".
 */
bool CBEHeaderFile::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    if (!pFEFile)
    {
        VERBOSE("CBEHeaderFile::CreateBE failed because front-end file is 0\n");
        return false;
    }
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
    m_sIncludeName = pContext->GetNameFactory()->GetIncludeFileName(pFEFile, pContext);
    m_nFileType = pContext->GetFileType();

    VectorElement *pIter = pFEFile->GetFirstIncludeFile();
    CFEFile *pIncFile;
    while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
    {
        String sIncName = pContext->GetNameFactory()->GetIncludeFileName(pIncFile, pContext);
        // if the compiler option is FILE_ALL, then we only add non-IDL files
        if (pContext->IsOptionSet(PROGRAM_FILE_ALL) && (pIncFile->IsIDLFile()))
            continue;
        AddIncludedFileName(sIncName, pIncFile->IsIDLFile());
    }

    return true;
}

/**	\brief prepares the header file for the back-end
 *	\param pFELibrary the corresponding front-end library
 *	\param pContext the context of the code generation
 *	\return true if creation was successful
 */
bool CBEHeaderFile::CreateBackEnd(CFELibrary * pFELibrary, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFELibrary, pContext);
    m_sIncludeName = pContext->GetNameFactory()->GetIncludeFileName(pFELibrary, pContext);
    m_nFileType = pContext->GetFileType();
    return true;
}

/**	\brief prepares the back-end file for usage as per interface file
 *	\param pFEInterface the respective interface to prepare for
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 */
bool CBEHeaderFile::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEInterface, pContext);
    m_sIncludeName = pContext->GetNameFactory()->GetIncludeFileName(pFEInterface, pContext);
    m_nFileType = pContext->GetFileType();
    return true;
}

/**	\brief prepares the back-end file for usage as per operation file
 *	\param pFEOperation the respective front-end operation to prepare for
 *	\param pContext the context of the code generation
 *	\return true if back-end was created correctly
 */
bool CBEHeaderFile::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEOperation, pContext);
    m_sIncludeName= pContext->GetNameFactory()->GetIncludeFileName(pFEOperation, pContext);
    m_nFileType = pContext->GetFileType();
    return true;
}

/**	\brief adds a new constant to the header file
 *	\param pConstant the constant to add
 */
void CBEHeaderFile::AddConstant(CBEConstant * pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.Add(pConstant);
}

/**	\brief removes a constant from the header file
 *	\param pConstant the constant to remove
 */
void CBEHeaderFile::RemoveConstant(CBEConstant * pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.Remove(pConstant);
}

/**	\brief retrieves a pointer to the first constant
 *	\return a pointer to the first constant
 */
VectorElement *CBEHeaderFile::GetFirstConstant()
{
    return m_vConstants.GetFirst();
}

/**	\brief retrieves the next constant the iterator points to
 *	\param pIter the iterator pointing to the next constant
 *	\return a reference to the next constant
 */
CBEConstant *CBEHeaderFile::GetNextConstant(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEConstant *pRet = (CBEConstant *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextConstant(pIter);
    return pRet;
}

/**	\brief adds a new type definition to the header file
 *	\param pTypedef the type definition to add
 */
void CBEHeaderFile::AddTypedef(CBETypedef * pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.Add(pTypedef);
}

/**	\brief removes a type definition from the file
 *	\param pTypedef the type definition to remove
 */
void CBEHeaderFile::RemoveTypedef(CBETypedef * pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.Remove(pTypedef);
}

/**	\brief retrieves a pointer to the first type definition
 *	\return a pointer to the first type definition
 */
VectorElement *CBEHeaderFile::GetFirstTypedef()
{
    return m_vTypedefs.GetFirst();
}

/**	\brief returns a reference to the next type definition
 *	\param pIter the pointer to the next type definition
 *	\return a reference to the next type definition
 */
CBETypedef *CBEHeaderFile::GetNextTypedef(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBETypedef *pRet = (CBETypedef *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTypedef(pIter);
    return pRet;
}

/**	\brief writes the content of the header file
 *	\param pContext the context of the write operation
 *
 * The content of the header file includes the functions, constants and type definitions.
 *
 * Before we can actually write anything we have to create the file.
 *
 * The content of a header file is always braced by a symbol, so a multiple include of this
 * file will not result in multiple constant, type or function declarations.
 *
 * In C we start with the constants, then the type definitions and after that we write the
 * functions using the base class' Write operation.  For the server's side we print the typedefs
 * before the includes. This way we can use the message buffer type of the derived server-loop
 * for the functions of the base interface.
 */
void CBEHeaderFile::Write(CBEContext * pContext)
{
    if (!Open(CFile::Write))
    {
        fprintf(stderr, "Could not open header file %s\n", (const char *) GetFileName());
        return;
    }
    // write include define
    String sDefine = pContext->GetNameFactory()->GetHeaderDefine(GetFileName(), pContext);
    if (!sDefine.IsEmpty())
    {
        Print("#if !defined(%s)\n", (const char *) sDefine);
        Print("#define %s\n", (const char *) sDefine);
    }
    // pretty print: newline
    Print("\n");

    WriteIncludesBeforeTypes(pContext);
    // write type definitions
    WriteTypedefs(pContext);
    // pretty print: newline

    // write includes
    WriteIncludesAfterTypes(pContext);

    // write constants
    WriteConstants(pContext);
    // pretty print: newline

    // call base class' Write function to write the functions
    WriteClasses(pContext);
    WriteNameSpaces(pContext);
    WriteFunctions(pContext);
    // pretty print: newline

    // write include define closing statement
    if (!sDefine.IsEmpty())
    {
        Print("#endif /* %s */\n", (const char *) sDefine);
    }
    // pretty print: newline
    Print("\n");

    // close file
    Close();
}

/**	\brief writes the constants to the target file
 *	\param pContext the context of the write operation
 */
void CBEHeaderFile::WriteConstants(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstConstant();
    CBEConstant *pConst;
    bool bCount = false;
    while ((pConst = GetNextConstant(pIter)) != 0)
    {
        pConst->Write(this, pContext);
        bCount = true;
    }
    if (bCount)
        Print("\n");
}

/**	\brief writes the type definitions to the target file
 *	\param pContext the context of the write operation
 */
void CBEHeaderFile::WriteTypedefs(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    bool bCount = false;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        pTypedef->WriteDeclaration(this, pContext);
        bCount = true;
    }
    if (bCount)
        Print("\n");
}

/**	\brief tries to find the typedef with a type of the given name
 *	\param sTypeName the name to search for
 *	\return a reference to the searched typedef or 0
 *
 * To find a typedef, we iterate over the typedefs and check the names.
 */
CBETypedef *CBEHeaderFile::FindTypedef(String sTypeName)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        VectorElement *pIterD = pTypedef->GetFirstDeclarator();
        CBEDeclarator *pDecl;
        while ((pDecl = pTypedef->GetNextDeclarator(pIterD)) != 0)
        {
            if (pDecl->GetName() == sTypeName)
                return pTypedef;
        }
    }
    return 0;
}

/** \brief writes the classes to this header file
 *  \param pContext the context of the write operation
 *
 * This function is overloaded from CBEFile::Write, because the compiler will not know
 * which of CBEClass's Write function to call if CBEFile calls pClass->Write(this). That's
 * why we have to reproduce the code here.
 */
void CBEHeaderFile::WriteClasses(CBEContext *pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        pClass->Write(this, pContext);
    }
}

/** \brief writes the namespaces to the header file
 *  \param pContext the context of the write operation
 *
 * The reason for this function is the same as for WriteClasses
 */
void CBEHeaderFile::WriteNameSpaces(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        pNameSpace->Write(this, pContext);
    }
}

/** \brief writes the functions to the target file
 *  \param pContext the context of the write
 */
void CBEHeaderFile::WriteFunctions(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(pIter)) != 0)
    {
		if (pFunction->DoWriteFunction(this, pContext))
		{
			pFunction->Write(this, pContext);
		}
    }        
}

/** \brief writes includes, which have to appear before any type definition
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteIncludesBeforeTypes(CBEContext * pContext)
{
    Print("/* needed for CORBA types */\n");
    Print("#include \"dice/dice.h\"\n");
    Print("\n");
}

/** \brief returns the file name used in include statements
 *  \return the file name used in include statements
 */
String CBEHeaderFile::GetIncludeFileName()
{
    return m_sIncludeName;
}
