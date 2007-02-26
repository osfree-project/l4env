/**
 *    \file    dice/src/be/BEHeaderFile.cpp
 *    \brief   contains the implementation of the class CBEHeaderFile
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

#include "BEHeaderFile.h"
#include "BEContext.h"
#include "BEConstant.h"
#include "BETypedef.h"
#include "BEType.h"
#include "BEFunction.h"
#include "BEDeclarator.h"
#include "BENameSpace.h"
#include "BEClass.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "IncludeStatement.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

CBEHeaderFile::CBEHeaderFile()
{
}

CBEHeaderFile::CBEHeaderFile(CBEHeaderFile & src)
: CBEFile(src)
{
    m_sIncludeName = src.m_sIncludeName;
    COPY_VECTOR(CBEConstant, m_vConstants, iterC);
    COPY_VECTOR(CBETypedef, m_vTypedefs, iterT);
    COPY_VECTOR(CBEType, m_vTaggedTypes, iterTa);
}

/**    \brief destructor
 */
CBEHeaderFile::~CBEHeaderFile()
{
    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vTaggedTypes);
}

/**    \brief prepares the header file for the back-end
 *    \param pFEFile the corresponding front-end file
 *    \param pContext the context of the code generation
 *    \return true if the creation was successful
 *
 * This function should only add the file name and the names of the included
 * files to this instance.
 *
 * If the name of the front-end file included a relative path, this path
 * should be stripped of for the file name of this file. But it should be used
 * when writing include statements. E.g. a file included with #include
 * "l4/test.idl" should get the header file name "test-client.h" or
 * "test-server.h", but should be included using "l4/test-client.h" or
 * "l4/test-server.h".
 */
bool CBEHeaderFile::CreateBackEnd(CFEFile * pFEFile, CBEContext * pContext)
{
    assert(pFEFile);

    VERBOSE("%s for file %s called\n", __PRETTY_FUNCTION__, 
	pFEFile->GetFileName().c_str());

    CBENameFactory *pNF = pContext->GetNameFactory();
    m_sFileName = pNF->GetFileName(pFEFile, pContext);
    m_sIncludeName = pNF->GetIncludeFileName(pFEFile, pContext);
    m_nFileType = pContext->GetFileType();

    CFEFile *pFERoot = dynamic_cast<CFEFile*>(pFEFile->GetRoot());
    assert(pFERoot);
    vector<CIncludeStatement*>::iterator iterI = pFEFile->GetFirstInclude();
    CIncludeStatement *pInclude;
    while ((pInclude = pFEFile->GetNextInclude(iterI)) != 0)
    {
        // check if we shall add an include statement
        if (pInclude->IsPrivate())
            continue;
        // find the corresponding file
        CFEFile *pIncFile = pFERoot->FindFile(pInclude->GetIncludedFileName());
        // get name for include (with prefix, etc.)
        string sIncName;
        if (pIncFile)
            sIncName = pNF->GetIncludeFileName(pIncFile, pContext);
        else
            sIncName = pNF->GetIncludeFileName(pInclude->GetIncludedFileName(),
		pContext);
        // if the compiler option is FILE_ALL, then we only add non-IDL files
        if (pContext->IsOptionSet(PROGRAM_FILE_ALL) && pInclude->IsIDLFile())
            continue;

//         TRACE("Add include to file %s: %s (from %s:%d)\n",
//             GetFileName().c_str(), sIncName.c_str(),
//             pInclude->GetSourceFileName().c_str(),
//             pInclude->GetSourceLine());
        AddIncludedFileName(sIncName, pInclude->IsIDLFile(), 
	    pInclude->IsStdInclude(), pInclude);
    }

    VERBOSE("%s for file %s returns true\n", __PRETTY_FUNCTION__, 
	GetFileName().c_str());
    return true;
}

/**    \brief prepares the header file for the back-end
 *    \param pFELibrary the corresponding front-end library
 *    \param pContext the context of the code generation
 *    \return true if creation was successful
 */
bool 
CBEHeaderFile::CreateBackEnd(CFELibrary * pFELibrary, 
    CBEContext * pContext)
{
    VERBOSE("%s for library %s called\n", __PRETTY_FUNCTION__,
        pFELibrary->GetName().c_str());

    CBENameFactory *pNF = pContext->GetNameFactory();
    m_sFileName = pNF->GetFileName(pFELibrary, pContext);
    m_sIncludeName = pNF->GetIncludeFileName(pFELibrary, pContext);
    m_nFileType = pContext->GetFileType();
    
    VERBOSE("%s for library %s returns true\n", __PRETTY_FUNCTION__,
        pFELibrary->GetName().c_str());
    return true;
}

/**    \brief prepares the back-end file for usage as per interface file
 *    \param pFEInterface the respective interface to prepare for
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool 
CBEHeaderFile::CreateBackEnd(CFEInterface * pFEInterface, 
    CBEContext * pContext)
{
    VERBOSE("%s for interface %s called\n", __PRETTY_FUNCTION__,
        pFEInterface->GetName().c_str());

    CBENameFactory *pNF = pContext->GetNameFactory();
    m_sFileName = pNF->GetFileName(pFEInterface, pContext);
    m_sIncludeName = pNF->GetIncludeFileName(pFEInterface, pContext);
    m_nFileType = pContext->GetFileType();
    
    VERBOSE("%s for interface %s returns true\n", __PRETTY_FUNCTION__,
        pFEInterface->GetName().c_str());
    return true;
}

/**    \brief prepares the back-end file for usage as per operation file
 *    \param pFEOperation the respective front-end operation to prepare for
 *    \param pContext the context of the code generation
 *    \return true if back-end was created correctly
 */
bool
CBEHeaderFile::CreateBackEnd(CFEOperation * pFEOperation,
    CBEContext * pContext)
{
    VERBOSE("%s for operation %s called\n", __PRETTY_FUNCTION__,
        pFEOperation->GetName().c_str());
    
    CBENameFactory *pNF = pContext->GetNameFactory();
    m_sFileName = pNF->GetFileName(pFEOperation, pContext);
    m_sIncludeName= pNF->GetIncludeFileName(pFEOperation, pContext);
    m_nFileType = pContext->GetFileType();

    VERBOSE("%s for operation %s returns true\n", __PRETTY_FUNCTION__,
        pFEOperation->GetName().c_str());
    return true;
}

/**    \brief adds a new constant to the header file
 *    \param pConstant the constant to add
 */
void CBEHeaderFile::AddConstant(CBEConstant * pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.push_back(pConstant);
}

/**    \brief removes a constant from the header file
 *    \param pConstant the constant to remove
 */
void CBEHeaderFile::RemoveConstant(CBEConstant * pConstant)
{
    if (!pConstant)
        return;
    vector<CBEConstant*>::iterator iter;
    for (iter = m_vConstants.begin(); iter != m_vConstants.end(); iter++)
    {
        if (*iter == pConstant)
        {
            m_vConstants.erase(iter);
            return;
        }
    }
}

/**    \brief retrieves a pointer to the first constant
 *    \return a pointer to the first constant
 */
vector<CBEConstant*>::iterator CBEHeaderFile::GetFirstConstant()
{
    return m_vConstants.begin();
}

/**    \brief retrieves the next constant the iterator points to
 *    \param iter the iterator pointing to the next constant
 *    \return a reference to the next constant
 */
CBEConstant *CBEHeaderFile::GetNextConstant(vector<CBEConstant*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/**    \brief adds a new type definition to the header file
 *    \param pTypedef the type definition to add
 */
void CBEHeaderFile::AddTypedef(CBETypedef * pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.push_back(pTypedef);
}

/**    \brief removes a type definition from the file
 *    \param pTypedef the type definition to remove
 */
void CBEHeaderFile::RemoveTypedef(CBETypedef * pTypedef)
{
    if (!pTypedef)
        return;
    vector<CBETypedef*>::iterator iter;
    for (iter = m_vTypedefs.begin(); iter != m_vTypedefs.end(); iter++)
    {
        if (*iter == pTypedef)
        {
            m_vTypedefs.erase(iter);
            return;
        }
    }
}

/**    \brief retrieves a pointer to the first type definition
 *    \return a pointer to the first type definition
 */
vector<CBETypedef*>::iterator CBEHeaderFile::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/**    \brief returns a reference to the next type definition
 *    \param iter the pointer to the next type definition
 *    \return a reference to the next type definition
 */
CBETypedef *CBEHeaderFile::GetNextTypedef(vector<CBETypedef*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/**    \brief writes the content of the header file
 *    \param pContext the context of the write operation
 *
 * The content of the header file includes the functions, constants and type
 * definitions.
 *
 * Before we can actually write anything we have to create the file.
 *
 * The content of a header file is always braced by a symbol, so a multiple
 * include of this file will not result in multiple constant, type or function
 * declarations.
 *
 * In C we start with the constants, then the type definitions and after that
 * we write the functions using the base class' Write operation.  For the
 * server's side we print the typedefs before the includes. This way we can
 * use the message buffer type of the derived server-loop for the functions of
 * the base interface.
 */
void CBEHeaderFile::Write(CBEContext * pContext)
{
    string sOutputDir = pContext->GetOutputDir();
    string sFilename;
    if (!sOutputDir.empty())
        sFilename = sOutputDir;
    sFilename += GetFileName();
    if (!Open(sFilename, CFile::Write))
    {
        fprintf(stderr, "Could not open header file %s\n", sFilename.c_str());
        return;
    }
    // sort our members/elements depending on source line number
    // into extra vector
    CreateOrderedElementList();

    // write intro
    WriteIntro(pContext);
    // write include define
    string sDefine = pContext->GetNameFactory()->GetHeaderDefine(GetFileName(), pContext);
    if (!sDefine.empty())
    {
        Print("#if !defined(%s)\n", sDefine.c_str());
        Print("#define %s\n", sDefine.c_str());
    }
    // pretty print: newline
    Print("\n");

    // default includes always come first, because they define standard headers
    // needed by other includes
    WriteDefaultIncludes(pContext);

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if (dynamic_cast<CIncludeStatement*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 2;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 3;
        else if (dynamic_cast<CBEConstant*>(*iter))
            nCurrType = 4;
        else if (dynamic_cast<CBETypedef*>(*iter))
            nCurrType = 5;
        else if (dynamic_cast<CBEType*>(*iter))
            nCurrType = 6;
        else if (dynamic_cast<CBEFunction*>(*iter))
        {
	    /* only write functions if this is client header or component
	     * header */
	    if (IsOfFileType(FILETYPE_CLIENTHEADER) ||
                IsOfFileType(FILETYPE_COMPONENTHEADER))
                nCurrType = 7;
        }
        else
            nCurrType = 0;
        if (nCurrType != nLastType)
        {
            // brace functions with extern C
            if (nLastType == 6)
            {
                Print("#ifdef __cplusplus\n");
                Print("}\n");
                Print("#endif\n\n");
            }
            Print("\n");
            nLastType = nCurrType;
            // brace functions with extern C
            if (nCurrType == 6)
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
            WriteConstant((CBEConstant*)(*iter), pContext);
            break;
        case 5:
            WriteTypedef((CBETypedef*)(*iter), pContext);
            break;
        case 6:
            WriteTaggedType((CBEType*)(*iter), pContext);
            break;
        case 7:
            WriteFunction((CBEFunction*)(*iter), pContext);
            break;
        default:
            break;
        }
    }
    // if last element was function, close braces
    if (nLastType == 6)
    {
        Print("#ifdef __cplusplus\n");
        Print("}\n");
        Print("#endif\n\n");
    }

    // write helper functions, if any
    /* only write functions if this is client header or component header */
    if (IsOfFileType(FILETYPE_CLIENTHEADER) ||
        IsOfFileType(FILETYPE_COMPONENTHEADER))
    {
        WriteHelperFunctions(pContext);
    }

    // write include define closing statement
    if (!sDefine.empty())
    {
        Print("#endif /* %s */\n", sDefine.c_str());
    }
    // pretty print: newline
    Print("\n");

    // close file
    Close();
}

/**    \brief tries to find the typedef with a type of the given name
 *    \param sTypeName the name to search for
 *    \return a reference to the searched typedef or 0
 *
 * To find a typedef, we iterate over the typedefs and check the names.
 */
CBETypedef *CBEHeaderFile::FindTypedef(string sTypeName)
{
    vector<CBETypedef*>::iterator iter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(iter)) != 0)
    {
        vector<CBEDeclarator*>::iterator iterD = pTypedef->GetFirstDeclarator();
        CBEDeclarator *pDecl;
        while ((pDecl = pTypedef->GetNextDeclarator(iterD)) != 0)
        {
            if (pDecl->GetName() == sTypeName)
                return pTypedef;
        }
    }
    return 0;
}

/** \brief writes includes, which have to appear before any type definition
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteDefaultIncludes(CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        Print("#define DICE_TESTSUITE 1\n");
    if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER))
        Print("#define DICE_TRACE_SERVER 1\n");
    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
        Print("#define DICE_TRACE_CLIENT 1\n");
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
        Print("#define DICE_TRACE_MSGBUF 1\n");
    Print("/* needed for CORBA types */\n");
    Print("#include \"dice/dice.h\"\n");
    Print("\n");
}

/** \brief returns the file name used in include statements
 *  \return the file name used in include statements
 */
string CBEHeaderFile::GetIncludeFileName()
{
    return m_sIncludeName;
}

/** \brief returns a pointer to the first tagged type
 *  \return a pointer to the first tagged type
 */
vector<CBEType*>::iterator CBEHeaderFile::GetFirstTaggedType()
{
    return m_vTaggedTypes.begin();
}

/** \brief return reference to next tagged type
 *  \param iter the pointer to the next tagged type
 *  \return reference to next tagged type
 */
CBEType* CBEHeaderFile::GetNextTaggedType(vector<CBEType*>::iterator &iter)
{
    if (iter == m_vTaggedTypes.end())
        return 0;
    return *iter++;
}

/** \brief adds a tagged type
 *  \param pTaggedType the tagged type to add
 */
void CBEHeaderFile::AddTaggedType(CBEType *pTaggedType)
{
    if (!pTaggedType)
        return;
    m_vTaggedTypes.push_back(pTaggedType);
}

/** \brief removes a tagged type from the header file
 *  \param pTaggedType the tagged type to remove
 */
void CBEHeaderFile::RemoveTaggedType(CBEType *pTaggedType)
{
    if (!pTaggedType)
        return;
    vector<CBEType*>::iterator iter;
    for (iter = m_vTaggedTypes.begin(); iter != m_vTaggedTypes.end(); iter++)
    {
        if (*iter == pTaggedType)
        {
            m_vTaggedTypes.erase(iter);
            return;
        }
    }
}

/** \brief searches for a tagged type
 *  \param sTypeName the name of the type to search
 *  \return a reference to the tagged type if found
 *
 * A tagged type always has a tagged, which is used to compare names
 */
CBEType *CBEHeaderFile::FindTaggedType(string sTypeName)
{
    vector<CBEType*>::iterator iter = GetFirstTaggedType();
    CBEType *pTaggedType;
    while ((pTaggedType = GetNextTaggedType(iter)) != 0)
    {
        if (pTaggedType->HasTag(sTypeName))
            return pTaggedType;
    }
    return 0;
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEHeaderFile::CreateOrderedElementList(void)
{
    // first call base class
    CBEFile::CreateOrderedElementList();

    // add own vectors
    // typedef
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pT;
    while ((pT = GetNextTypedef(iterT)) != NULL)
    {
        InsertOrderedElement(pT);
    }
    // tagged types
    vector<CBEType*>::iterator iterTa = GetFirstTaggedType();
    CBEType* pTa;
    while ((pTa = GetNextTaggedType(iterTa)) != NULL)
    {
        InsertOrderedElement(pTa);
    }
    // consts
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pC;
    while ((pC = GetNextConstant(iterC)) != NULL)
    {
        InsertOrderedElement(pC);
    }
}

/** \brief writes a class
 *  \param pClass the class to write
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteClass(CBEClass *pClass, CBEContext *pContext)
{
    assert(pClass);
    pClass->Write(this, pContext);
}

/** \brief writes the namespace
 *  \param pNameSpace the namespace to write
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteNameSpace(CBENameSpace *pNameSpace, CBEContext *pContext)
{
    assert(pNameSpace);
    pNameSpace->Write(this, pContext);
}

/** \brief writes the function
 *  \param pFunction the function to write
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteFunction(CBEFunction *pFunction, CBEContext *pContext)
{
    assert(pFunction);
    if (pFunction->DoWriteFunction(this, pContext))
        pFunction->Write(this, pContext);
}

/** \brief  writes a constant
 *  \param pConstant the constant to write
 *  \param pContext the conte of the write operation
 */
void CBEHeaderFile::WriteConstant(CBEConstant *pConstant, CBEContext *pContext)
{
    assert(pConstant);
    pConstant->Write(this, pContext);
}

/** \brief write a typedef
 *  \param pTypedef the typedef to write
 *  \param pContext the context of the write operation
 */
void CBEHeaderFile::WriteTypedef(CBETypedef *pTypedef, CBEContext *pContext)
{
    assert(pTypedef);
    pTypedef->WriteDeclaration(this, pContext);
}

/** \brief writes a tagged type
 *  \param pType the type to write
 *  \param pContext the context of the write operation
 */
 void CBEHeaderFile::WriteTaggedType(CBEType *pType, CBEContext *pContext)
 {
    assert(pType);
    // get tag
    string sTag;
    if (dynamic_cast<CBEStructType*>(pType))
        sTag = ((CBEStructType*)pType)->GetTag();
    if (dynamic_cast<CBEUnionType*>(pType))
        sTag = ((CBEUnionType*)pType)->GetTag();
    sTag = pContext->GetNameFactory()->GetTypeDefine(sTag, pContext);
    Print("#ifndef %s\n", sTag.c_str());
    Print("#define %s\n", sTag.c_str());
    pType->Write(this, pContext);
    Print(";\n");
    Print("#endif /* !%s */\n", sTag.c_str());
    Print("\n");
}

/** \brief retrieves the maximum line number in the file
 *  \return the maximum line number in this file
 *
 * If line number is not that, i.e., is zero, then we iterate the elements and
 * check their end line number. The maximum is out desired maximum line
 * number.
 */
int
CBEHeaderFile::GetSourceLineEnd()
{
    if (m_nSourceLineNbEnd != 0)
	return m_nSourceLineNbEnd;

    // get maximum of members in base class;
    CBEFile::GetSourceLineEnd();
    
    // constants
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pC;
    while ((pC = GetNextConstant(iterC)) != 0)
    {
	int sLine = pC->GetSourceLine();
	int eLine = pC->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // typedef
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pT;
    while ((pT = GetNextTypedef(iterT)) != NULL)
    {
	int sLine = pT->GetSourceLine();
	int eLine = pT->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }
    // tagged types
    vector<CBEType*>::iterator iterTa = GetFirstTaggedType();
    CBEType *pTa;
    while ((pTa = GetNextTaggedType(iterTa)) != NULL)
    {
	int sLine = pTa->GetSourceLine();
	int eLine = pTa->GetSourceLineEnd();
	m_nSourceLineNbEnd = MAX(sLine, MAX(eLine, m_nSourceLineNbEnd));
    }

    return m_nSourceLineNbEnd;
}
