/**
 *    \file    dice/src/be/BEImplementationFile.cpp
 * \brief   contains the implementation of the class CBEImplementationFile
 *
 *    \date    01/11/2002
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

#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEContext.h"
#include "BEFunction.h"
#include "BEClass.h"
#include "BENameSpace.h"
#include "IncludeStatement.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

CBEImplementationFile::CBEImplementationFile()
{
    m_pHeaderFile = 0;
}

CBEImplementationFile::CBEImplementationFile(CBEImplementationFile & src):CBEFile(src)
{
    m_pHeaderFile = src.m_pHeaderFile;
}

/** \brief destructor
 */
CBEImplementationFile::~CBEImplementationFile()
{

}

/** \brief sets the internal reference to the corresponding header file
 *  \param pHeaderFile points to the header file
 */
void CBEImplementationFile::SetHeaderFile(CBEHeaderFile * pHeaderFile)
{
    m_pHeaderFile = pHeaderFile;
}

/** \brief retrieves a reference to the corresponding header file
 *  \return a reference to this implementation file's header file
 */
CBEHeaderFile *CBEImplementationFile::GetHeaderFile()
{
    return m_pHeaderFile;
}

/** \brief prepares the header file for the back-end
 *  \param pFEFile the corresponding front-end file
 *  \param pContext the context of the code generation
 *  \return true if the creation was successful
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
        AddIncludedFileName(pHeader->GetFileName(), true, false, pFEFile);
    return true;
}

/** \brief prepares the header file for the back-end
 *  \param pFELibrary the corresponding front-end library
 *  \param pContext the context of the code generation
 *  \return true if creation was successful
 */
bool CBEImplementationFile::CreateBackEnd(CFELibrary * pFELibrary, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFELibrary, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true, false, pFELibrary);
    return true;
}

/** \brief prepares the back-end file for usage as per interface file
 *  \param pFEInterface the respective interface to prepare for
 *  \param pContext the context of the code generation
 *  \return true if code generation was successful
 */
bool CBEImplementationFile::CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEInterface, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true, false, pFEInterface);
    return true;
}

/** \brief prepares the back-end file for usage as per operation file
 *  \param pFEOperation the respective front-end operation to prepare for
 *  \param pContext the context of the code generation
 *  \return true if back-end was created correctly
 */
bool CBEImplementationFile::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    m_sFileName = pContext->GetNameFactory()->GetFileName(pFEOperation, pContext);
    m_nFileType = pContext->GetFileType();
    CBEHeaderFile *pHeader = GetHeaderFile();
    if (pHeader)
        AddIncludedFileName(pHeader->GetFileName(), true, false, pFEOperation);

    return true;
}

/** \brief writes the content of the implementation file to the target file
 *  \param pContext the context of the write operation
 *
 * Because  a header file only contains the function definitions, this implementation
 * only opens the file, writes the include statements and uses the base class' Write
 * function to print the functions.
 */
void CBEImplementationFile::Write(CBEContext * pContext)
{
    string sOutputDir = pContext->GetOutputDir();
    string sFilename;
    if (!sOutputDir.empty())
        sFilename = sOutputDir;
    sFilename += GetFileName();
    if (!Open(sFilename, CFile::Write))
    {
        fprintf(stderr, "Could not open implementation file %s\n",
            sFilename.c_str());
        return;
    }
    // sort our members/elements depending on source line number
    // into extra vector
    CreateOrderedElementList();

    // write intro
    WriteIntro(pContext);

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        nCurrType = 0;
        if (dynamic_cast<CIncludeStatement*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 2;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 3;
        else if (dynamic_cast<CBEFunction*>(*iter))
            nCurrType = 4;
        // newline when changing types
        if (nCurrType != nLastType)
        {
            // brace functions with extern C
            if (nLastType == 4)
            {
                Print("#ifdef __cplusplus\n");
                Print("}\n");
                Print("#endif\n\n");
            }
            Print("\n");
            nLastType = nCurrType;
            // brace functions with extern C
            if (nCurrType == 4)
            {
                Print("#ifdef __cplusplus\n");
                Print("extern \"C\" {\n");
                Print("#endif\n\n");
            }
        }
        // add pre-processor directive to denote source line
        if (pContext->IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE))
        {
            Print("# %d \"%s\"\n", (*iter)->GetSourceLine(),
                (*iter)->GetSourceFileName().c_str());
        }
        // now really write the element
        switch (nCurrType)
        {
        case 1:
            WriteInclude((CIncludeStatement*)(*iter), pContext);
            break;
        case 2:
            WriteClass((CBEClass*)(*iter), pContext);
            break;
        case 3:
            WriteNameSpace((CBENameSpace*)(*iter), pContext);
            break;
        case 4:
            WriteFunction((CBEFunction*)(*iter), pContext);
            break;
        default:
            break;
        }
    }
    // if last element was function, close braces
    if (nLastType == 4)
    {
        Print("#ifdef __cplusplus\n");
        Print("}\n");
        Print("#endif\n\n");
    }

    // write helper functions, if any
    WriteHelperFunctions(pContext);

    // close file
    Close();
}

/** \brief writes a class
 *  \param pClass the class to write
 *  \param pContext the context of the write operation
 */
void CBEImplementationFile::WriteClass(CBEClass *pClass, CBEContext *pContext)
{
    assert(pClass);
    pClass->Write(this, pContext);
}

/** \brief writes the namespace
 *  \param pNameSpace the namespace to write
 *  \param pContext the context of the write operation
 */
void CBEImplementationFile::WriteNameSpace(CBENameSpace *pNameSpace, CBEContext *pContext)
{
    assert(pNameSpace);
    pNameSpace->Write(this, pContext);
}

/** \brief writes the function
 *  \param pFunction the function to write
 *  \param pContext the context of the write operation
 */
void CBEImplementationFile::WriteFunction(CBEFunction *pFunction, CBEContext *pContext)
{
    assert(pFunction);
    if (pFunction->DoWriteFunction(this, pContext))
        pFunction->Write(this, pContext);
}
