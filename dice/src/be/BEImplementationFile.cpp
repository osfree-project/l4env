/**
 *	\file	dice/src/be/BEImplementationFile.cpp
 *	\brief	contains the implementation of the class CBEImplementationFile
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

#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEImplementationFile);

CBEImplementationFile::CBEImplementationFile()
{
    m_pHeaderFile = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEImplementationFile, CBEFile);
}

CBEImplementationFile::CBEImplementationFile(CBEImplementationFile & src):CBEFile(src)
{
    m_pHeaderFile = src.m_pHeaderFile;
    IMPLEMENT_DYNAMIC_BASE(CBEImplementationFile, CBEFile);
}

/**	\brief destructor
 */
CBEImplementationFile::~CBEImplementationFile()
{

}

/**	\brief sets the internal reference to the corresponding header file
 *	\param pHeaderFile points to the header file
 */
void CBEImplementationFile::SetHeaderFile(CBEHeaderFile * pHeaderFile)
{
    m_pHeaderFile = pHeaderFile;
}

/**	\brief retrieves a reference to the corresponding header file
 *	\return a reference to this implementation file's header file
 */
CBEHeaderFile *CBEImplementationFile::GetHeaderFile()
{
    return m_pHeaderFile;
}

/**	\brief prepares the header file for the back-end
 *	\param pFEFile the corresponding front-end file
 *	\param pContext the context of the code generation
 *	\return true if the creation was successful
 *
 * An implementation file's only included file is it's header file.
 * We do not use the "include" file name for this, since we assume that
 * the header file for the implementation file resides in the same
 * directory.
 */
bool CBEImplementationFile::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true);
    return true;
}

/**	\brief prepares the header file for the back-end
 *	\param pFELibrary the corresponding front-end library
 *	\param pContext the context of the code generation
 *	\return true if creation was successful
 */
bool CBEImplementationFile::CreateBackEnd(CFELibrary * pFELibrary, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFELibrary, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true);
    return true;
}

/**	\brief prepares the back-end file for usage as per interface file
 *	\param pFEInterface the respective interface to prepare for
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 */
bool CBEImplementationFile::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEInterface, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true);
    return true;
}

/**	\brief prepares the back-end file for usage as per operation file
 *	\param pFEOperation the respective front-end operation to prepare for
 *	\param pContext the context of the code generation
 *	\return true if back-end was created correctly
 */
bool CBEImplementationFile::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEOperation, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true);

    return true;
}

/**	\brief writes the content of the implementation file to the target file
 *	\param pContext the context of the write operation
 *
 * Because  a header file only contains the function definitions, this implementation
 * only opens the file, writes the include statements and uses the base class' Write
 * function to print the functions.
 */
void CBEImplementationFile::Write(CBEContext * pContext)
{
     if (!Open(CFile::Write))
     {
         fprintf(stderr, "Could not open implementation file %s\n", (const char *) GetFileName());
         return;
     }
     // write includes
     WriteIncludesBeforeTypes(pContext);
     WriteIncludesAfterTypes(pContext);

     // call base class to write functions
     WriteClasses(pContext);
     WriteNameSpaces(pContext);
     WriteFunctions(pContext);

     // close file
     Close();
}

/** \brief writes the classes to the implementation file
 *  \param pContext the context of the write operation
 *
 * This function is called because if implemented by the base class the call
 * pClass->Write(this) will confuse the compiler. Thus we implement it in the header
 * file and here.
 */
void CBEImplementationFile::WriteClasses(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        pClass->Write(this, pContext);
    }
}

/** \brief writes the namespaces to the implementation file
 *  \param pContext the context of the write operation
 *
 * The reason for this function's existence is the same as for WriteClasses
 */
void CBEImplementationFile::WriteNameSpaces(CBEContext * pContext)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        pNameSpace->Write(this, pContext);
    }
}

/** \brief writes the functions to the target file
 *  \param pContext the context of the write operation
 */
void CBEImplementationFile::WriteFunctions(CBEContext * pContext)
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
