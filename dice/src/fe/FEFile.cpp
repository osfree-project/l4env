/**
 *	\file	dice/src/fe/FEFile.cpp
 *	\brief	contains the implementation of the class CFEFile
 *
 *	\date	01/31/2001
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

#include "fe/FEFile.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"

IMPLEMENT_DYNAMIC(CFEFile)
    
CFEFile::CFEFile(String sFileName, String sPath, int nIncludeLevel, int nStdInclude)
: m_vTaggedDecls(RUNTIME_CLASS(CFEConstructedType)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vLibraries(RUNTIME_CLASS(CFELibrary)),
  m_vInterfaces(RUNTIME_CLASS(CFEInterface)),
  m_vChildFiles(RUNTIME_CLASS(CFEFile))
{
    IMPLEMENT_DYNAMIC_BASE(CFEFile, CFEBase);
    // this procedure is interesting for included files:
    // if the statement was: #include "l4/sys/types.h" and the path where the file
    // was found is "../../include" the following string are created:
    //
    // m_sFileName = "l4/sys/types.h"
    // m_sFilenameWithoutExtension = "types"
    // m_sFileExtension = "h"
    // m_sFileWithPath = "../../include/l4/sys/types.h"
    m_sFileName = sFileName;
    if (!m_sFileName.IsEmpty())
    {
        // first strip of extension  (the string after the last '.')
        int iDot = m_sFileName.ReverseFind('.');
        if (iDot < 0)
        {
            m_sFilenameWithoutExtension = m_sFileName;
            m_sFileExtension.Empty();
        }
        else
        {
            m_sFilenameWithoutExtension = m_sFileName.Left(iDot);
            m_sFileExtension = m_sFileName.Mid(iDot + 1);
        }
        // now strip of the path
        int iSlash = m_sFilenameWithoutExtension.ReverseFind('/');
        if (iSlash >= 0)
        {
            m_sFilenameWithoutExtension = m_sFilenameWithoutExtension.Mid(iSlash + 1);
        }
        // now generate full filename with path
        if (!sPath.IsEmpty())
        {
            bool bLastSlash = sPath.CharAt(sPath.GetLength() - 1) == '/';
            m_sFileWithPath = sPath;
            if (!bLastSlash)
                m_sFileWithPath += "/";
            m_sFileWithPath += m_sFileName;
        }
        else
            m_sFileWithPath = m_sFileName;
    }
    else
    {
        m_sFilenameWithoutExtension.Empty();
        m_sFileExtension.Empty();
        m_sFileWithPath.Empty();
    }
    m_nIncludeLevel = nIncludeLevel;
    m_nStdInclude = nStdInclude;
}

CFEFile::CFEFile(CFEFile & src)
: CFEBase(src),
  m_vTaggedDecls(RUNTIME_CLASS(CFEConstructedType)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vLibraries(RUNTIME_CLASS(CFELibrary)),
  m_vInterfaces(RUNTIME_CLASS(CFEInterface)),
  m_vChildFiles(RUNTIME_CLASS(CFEFile))
{
    IMPLEMENT_DYNAMIC_BASE(CFEFile, CFEBase);

    m_sFileName = src.m_sFileName;
    m_sFileExtension = src.m_sFileExtension;
    m_sFilenameWithoutExtension = src.m_sFilenameWithoutExtension;
    m_sFileWithPath = src.m_sFileWithPath;
    m_nIncludeLevel = src.m_nIncludeLevel;
    m_nStdInclude = src.m_nStdInclude;
    m_vTaggedDecls.Add(&src.m_vTaggedDecls);
    m_vTaggedDecls.SetParentOfElements(this);
    m_vConstants.Add(&src.m_vConstants);
    m_vConstants.SetParentOfElements(this);
    m_vTypedefs.Add(&src.m_vTypedefs);
    m_vTypedefs.SetParentOfElements(this);
    m_vLibraries.Add(&src.m_vLibraries);
    m_vLibraries.SetParentOfElements(this);
    m_vInterfaces.Add(&src.m_vInterfaces);
    m_vInterfaces.SetParentOfElements(this);
    m_vChildFiles.Add(&src.m_vChildFiles);
    m_vChildFiles.SetParentOfElements(this);
}

/** cleans up the file object */
CFEFile::~CFEFile()
{

}

/** copies the object
 *	\return a reference to the new file object
 */
CObject *CFEFile::Clone()
{
    return new CFEFile(*this);
}

/** \brief adds another (included) file to this file hierarchy
 *  \param pNewChild the (included) file to add
 */
void CFEFile::AddChild(CFEFile * pNewChild)
{
    m_vChildFiles.Add(pNewChild);
    pNewChild->SetParent(this);
}

/**
 *	\brief checks if this component is at the first include level
 *	\return true if the include level is bigger than 0
 */
bool CFEFile::IsIncluded()
{
    return (m_nIncludeLevel > 0);
}

/**
 *	\brief return the include level of the component
 *	\return the include level
 */
int CFEFile::GetIncludeLevel()
{
    return m_nIncludeLevel;
}

/** returns a pointer to the first included file
 *	\return a pointer to the first included file
 */
VectorElement *CFEFile::GetFirstIncludeFile()
{
    return m_vChildFiles.GetFirst();
}

/** returns a refrence to the next included file
 *	\param iter the pointer to the next included file
 *	\return a refrence to the next included file
 */
CFEFile *CFEFile::GetNextIncludeFile(VectorElement * &iter)
{
    if (!iter)
       return 0;
    CFEFile *pRet = (CFEFile *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextIncludeFile(iter);
    return pRet;
}

/**	adds an interface to this file
 *	\param pInterface the new interface to add
 */
void CFEFile::AddInterface(CFEInterface * pInterface)
{
    m_vInterfaces.Add(pInterface);
    pInterface->SetParent(this);
}

/** returns a pointer to the first interface
 *	\return a pointer to the first interface
 */
VectorElement *CFEFile::GetFirstInterface()
{
    return m_vInterfaces.GetFirst();
}

/** returns a reference to the next interface
 *	\param iter a pointer to the next interface
 *	\return a reference to the next interface
 */
CFEInterface *CFEFile::GetNextInterface(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEInterface *pRet = (CFEInterface *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextInterface(iter);
    return pRet;
}

/** adds an library to this file
 *	\param pLibrary the new library to add
 */
void CFEFile::AddLibrary(CFELibrary * pLibrary)
{
	m_vLibraries.Add(pLibrary);
	pLibrary->SetParent(this);
}

/**	returns a pointer to the first library
 *	\return a pointer to the first library
 */
VectorElement *CFEFile::GetFirstLibrary()
{
    return m_vLibraries.GetFirst();
}

/**	returns a reference to the next library in this file
 *	\param iter a pointer to the next library in this file
 *	\return a reference to the next library in this file
 */
CFELibrary *CFEFile::GetNextLibrary(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFELibrary *pRet = (CFELibrary *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextLibrary(iter);
    return pRet;
}

/**	adds a type definition to this file
 *	\param pTypedef the new type definition
 */
void CFEFile::AddTypedef(CFETypedDeclarator * pTypedef)
{
    m_vTypedefs.Add(pTypedef);
    pTypedef->SetParent(this);
}

/** returns a pointer to the first type definition
 *	\return a pointer to the first type definition
 */
VectorElement *CFEFile::GetFirstTypedef()
{
    return m_vTypedefs.GetFirst();
}

/** \brief returns a reference to the next type defintion
 *  \param iter a pointer to the next type defintion
 *  \return a reference to the next type defintion
 */
CFETypedDeclarator *CFEFile::GetNextTypedef(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFETypedDeclarator *pRet = (CFETypedDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextTypedef(iter);
    return pRet;
}

/** adds the declaration of a constant to this file
 *	\param pConstant the new constant to add
 */
void CFEFile::AddConstant(CFEConstDeclarator * pConstant)
{
	m_vConstants.Add(pConstant);
	pConstant->SetParent(this);
}

/** returns a pointer to the first constant of this file
 *	\return a pointer to the first constant of this file
 */
VectorElement *CFEFile::GetFirstConstant()
{
    return m_vConstants.GetFirst();
}

/** returns a reference to the next constant definition in this file
 *	\param iter the pointer to the next constant definition
 *	\return a reference to the next constant definition in this file
 */
CFEConstDeclarator *CFEFile::GetNextConstant(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEConstDeclarator *pRet = (CFEConstDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextConstant(iter);
    return pRet;
}

/** \brief adds the definition of a tagged struct or union to this file
 *	\param pTaggedDecl the new declaration of this file
 */
void CFEFile::AddTaggedDecl(CFEConstructedType * pTaggedDecl)
{
	m_vTaggedDecls.Add(pTaggedDecl);
	pTaggedDecl->SetParent(this);
}

/** returns a pointer to the first tagged declarator
 *	\return a pointer to the first tagged declarator
 */
VectorElement *CFEFile::GetFirstTaggedDecl()
{
    return m_vTaggedDecls.GetFirst();
}

/** returns a reference to the next tagged declarator
 *	\param iter the pointer to the next tagged declarator
 *	\return a reference to the next tagged declarator
 */
CFEConstructedType *CFEFile::GetNextTaggedDecl(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEConstructedType *pRet = (CFEConstructedType *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
        return GetNextTaggedDecl(iter);
    return pRet;
}

/** returns the file's name
 *	\return the file's name
 */
String CFEFile::GetFileName()
{
    return m_sFileName;
}

/** returns a reference to the user defined type
 *	\param sName the name of the type to search for
 *	\return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator *CFEFile::FindUserDefinedType(String sName)
{
    if (sName.IsEmpty())
        return 0;
    // first search own typedefs
    VectorElement *pIter = GetFirstTypedef();
    CFETypedDeclarator *pTypedDecl;
    while ((pTypedDecl = GetNextTypedef(pIter)) != 0)
    {
        if (pTypedDecl->FindDeclarator(sName))
            return pTypedDecl;
    }
    // then search interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pTypedDecl = pInterface->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // then search libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pTypedDecl = pLibrary->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // next search included files
    pIter = GetFirstIncludeFile();
    CFEFile *pFile;
    while ((pFile = GetNextIncludeFile(pIter)) != 0)
    {
        if ((pTypedDecl = pFile->FindUserDefinedType(sName)))
            return pTypedDecl;
    }
    // none found
    return 0;
}

/** \brief searches for a tagged declarator
 *  \param sName the tag (name) of the tagged decl
 *  \return a reference to the tagged declarator or NULL if none found
 */
CFEConstructedType* CFEFile::FindTaggedDecl(String sName)
{
    // own tagged decls
    VectorElement *pIter = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(pIter)) != 0)
    {
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedStructType)))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // search interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pTaggedDecl = pInterface->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search libs
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pTaggedDecl = pLibrary->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // search included files
    pIter = GetFirstIncludeFile();
    CFEFile *pFile;
    while ((pFile = GetNextIncludeFile(pIter)) != 0)
    {
        if ((pTaggedDecl = pFile->FindTaggedDecl(sName)) != 0)
            return pTaggedDecl;
    }
    // nothing found:
    return 0;
}

/** returns a reference to the user defined type
 *	\param sName the name of the type to search for
 *	\return a reference to the user defined type, or 0 if it does not exist
 */
CFETypedDeclarator* CFEFile::FindUserDefinedType(const char *sName)
{
    return FindUserDefinedType(String(sName));
}

/**	\brief returns a reference to the interface
 *	\param sName the name of the interface to search for
 *	\return a reference to the interface, or 0 if not found
 *
 * If the name is a scoped name, we have to get the scope name and
 * use it to find the library for it. And then use the library to find
 * the rest of the name.
 */
CFEInterface *CFEFile::FindInterface(String sName)
{
    if (sName.IsEmpty())
        return 0;
    // if scoped name
    int nScopePos;
    if ((nScopePos = sName.Find("::")) >= 0)
    {
        String sRest = sName.Right(sName.GetLength()-nScopePos-2);
        String sScope = sName.Left(nScopePos);
        if (sScope.IsEmpty())
        {
            // has been a "::<name>"
            return FindInterface(sRest);
        }
        else
        {
            CFELibrary *pFELibrary = FindLibrary(sScope);
            if (pFELibrary == 0)
                return 0;
            return pFELibrary->FindInterface(sRest);
        }
    }
    // first search the interfaces in this file
    VectorElement *pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if (pInterface->GetName() == sName)
            return pInterface;
    }
    // search libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pInterface = pLibrary->FindInterface(sName)))
            return pInterface;
    }
    // the search the included files
    pIter = GetFirstIncludeFile();
    CFEFile *pFile;
    while ((pFile = GetNextIncludeFile(pIter)) != 0)
    {
        if ((pInterface = pFile->FindInterface(sName)))
            return pInterface;
    }
    // none found
    return 0;
}

/**	\brief returns a reference to the interface
 *	\param sName the name of the interface to search for
 *	\return a reference to the interface, or 0 if not found
 */
CFEInterface* CFEFile::FindInterface(const char* sName)
{
    return FindInterface(String(sName));
}

/**	\brief searches for a library
 *	\param sName the name to search for
 *	\return a reference to the found lib or 0 if not found
 */
CFELibrary *CFEFile::FindLibrary(String sName)
{
     if (sName.IsEmpty())
         return 0;

     VectorElement *pIter = GetFirstLibrary();
     CFELibrary *pLib, *pLib2;
     while ((pLib = GetNextLibrary(pIter)) != 0)
     {
         if (pLib->GetName() == sName)
             return pLib;
         // no matter if the name if 0, test nested libs
         if (pLib2 = pLib->FindLibrary(sName))
             return pLib2;
     }

     // search included/imported files
     pIter = GetFirstIncludeFile();
     CFEFile *pFEFile;
     while ((pFEFile = GetNextIncludeFile(pIter)) != 0)
     {
         if (pLib = pFEFile->FindLibrary(sName))
             return pLib;
     }

     return 0;
}

/**	\brief searches for a library
 *	\param sName the name to search for
 *	\return a reference to the found lib or 0 if not found
 */
CFELibrary* CFEFile::FindLibrary(const char* sName)
{
    return FindLibrary(String(sName));
}

/** returns a reference to the found constant declarator
 *	\param sName the name of the constant declarator to search for
 *	\return a reference to the found constant declarator, or 0 if not found
 */
CFEConstDeclarator *CFEFile::FindConstDeclarator(String sName)
{
    if (sName.IsEmpty())
        return 0;
    // first search this file's constants
    VectorElement *pIter = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(pIter)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    // then search included files
    pIter = GetFirstIncludeFile();
    CFEFile *pFile;
    while ((pFile = GetNextIncludeFile(pIter)) != 0)
    {
        if ((pConst = pFile->FindConstDeclarator(sName)))
            return pConst;
    }
    // then search the interfaces
    pIter = GetFirstInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextInterface(pIter)) != 0)
    {
        if ((pConst = pInterface->FindConstant(sName)))
            return pConst;
    }
    // then search the libraries
    pIter = GetFirstLibrary();
    CFELibrary *pLibrary;
    while ((pLibrary = GetNextLibrary(pIter)) != 0)
    {
        if ((pConst = pInterface->FindConstant(sName)))
            return pConst;
    }
    // if none found, return 0
    return 0;
}

/** checks the file-name for a given extension (has to be last)
 *	\param sExtension the extension to search for
 *	\return true if found, false if not
 *
 * This function is case insensitive.
 */
bool CFEFile::HasExtension(String sExtension)
{
    // if no extension, this file cannot be of this extension
    return !(m_sFileExtension.CompareNoCase(sExtension));
}

/**	\brief checks if this file is an IDL file
 *	\return true if this file is an IDL file
 *
 * The check is done using the xtension of the input file. This should be "idl" (case-insensitive)
 * or it also might be &lt;stdin&gt; if read from the standard input.
 */
bool CFEFile::IsIDLFile()
{
    if (!HasExtension(String("idl")))
    {
        if (m_sFilenameWithoutExtension.CompareNoCase("<stdin>"))
            return false;
    }
    return true;
}

/** returns the filename without the extension
 *	\return the filename without the extension
 */
String CFEFile::GetFileNameWithoutExtension()
{
    return m_sFilenameWithoutExtension;
}

/** \brief checks the consitency of this file
 *  \return true if everything is fine, false otherwise
 *
 * A file is ok, when all it's included files are ok in the first place. Then it
 * checks if the defined types do already exists. Then the constants are checked
 * and finally all contained interfaces and libraries are checked by starting a
 * self-test.
 */
bool CFEFile::CheckConsistency()
{
     // included files
     VectorElement *pIter = GetFirstIncludeFile();
     CFEFile *pFile;
     while ((pFile = GetNextIncludeFile(pIter)) != 0)
     {
         if (!(pFile->CheckConsistency()))
             return false;
     }
     // check types
     pIter = GetFirstTypedef();
     CFETypedDeclarator *pTypedef;
     while ((pTypedef = GetNextTypedef(pIter)) != 0)
     {
         if (!(pTypedef->CheckConsistency()))
             return false;
     }
     // check constants
     pIter = GetFirstConstant();
     CFEConstDeclarator *pConst;
     while ((pConst = GetNextConstant(pIter)) != 0)
     {
         if (!(pConst->CheckConsistency()))
             return false;
     }
     // check interfaces
     pIter = GetFirstInterface();
     CFEInterface *pInterface;
     while ((pInterface = GetNextInterface(pIter)) != 0)
     {
         if (!(pInterface->CheckConsistency()))
             return false;
     }
     // check libraries
     pIter = GetFirstLibrary();
     CFELibrary *pLib;
     while ((pLib = GetNextLibrary(pIter)) != 0)
     {
         if (!(pLib->CheckConsistency()))
             return false;
     }
     // everything ran through, so we might consider this file clean
     return true;
}

/** for debugging purposes only */
void CFEFile::Dump()
{
    printf("Dump: CFEFile (%s)\n", (const char *) GetFileName());
    printf("Dump: CFEFile (%s): included files:\n", (const char *) GetFileName());
    VectorElement *pIter = GetFirstIncludeFile();
    CFEBase *pElement;
    while ((pElement = GetNextIncludeFile(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): typedefs:\n", (const char *) GetFileName());
    pIter = GetFirstTypedef();
    while ((pElement = GetNextTypedef(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): constants:\n", (const char *) GetFileName());
    pIter = GetFirstConstant();
    while ((pElement = GetNextConstant(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): interfaces:\n", (const char *) GetFileName());
    pIter = GetFirstInterface();
    while ((pElement = GetNextInterface(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEFile (%s): libraries:\n", (const char *) GetFileName());
    pIter = GetFirstLibrary();
    while ((pElement = GetNextLibrary(pIter)) != 0)
    {
        pElement->Dump();
    }
}

/** write this object to a file
 *	\param pFile the file to serialize from/to
 */
void CFEFile::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<idl>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", (const char *) GetFileName());
        // write included files
        VectorElement *pIter = GetFirstIncludeFile();
        CFEBase *pElement;
        while ((pElement = GetNextIncludeFile(pIter)) != 0)
        {
            pFile->PrintIndent("<include>%s</include>\n", (const char *) ((CFEFile *) pElement)->GetFileName());
        }
        // write constants
        pIter = GetFirstConstant();
        while ((pElement = GetNextConstant(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write typedefs
        pIter = GetFirstTypedef();
        while ((pElement = GetNextTypedef(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write tagged decls
        pIter = GetFirstTaggedDecl();
        while ((pElement = GetNextTaggedDecl(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write libraries
        pIter = GetFirstLibrary();
        while ((pElement = GetNextLibrary(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write interfaces
        pIter = GetFirstInterface();
        while ((pElement = GetNextInterface(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        pFile->DecIndent();
        pFile->PrintIndent("</idl>\n");
    }
}

/**	\brief counts the constants of the file
 *	\param bCountIncludes true if included files should be countedt as well
 *	\return number of constants in this file
 */
int CFEFile::GetConstantCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = 0;
     VectorElement *pIter = GetFirstConstant();
     CFEConstDeclarator *pDecl;
     while ((pDecl = GetNextConstant(pIter)) != 0)
     {
         nCount++;
     }

     if (!bCountIncludes)
         return nCount;

     pIter = GetFirstIncludeFile();
     CFEFile *pFile;
     while ((pFile = GetNextIncludeFile(pIter)) != 0)
     {
         nCount += pFile->GetConstantCount();
     }

     return nCount;
}

/**	\brief count the typedefs of the file
 *	\param bCountIncludes true if included files should be counted as well
 *	\return number of typedefs in file
 */
int CFEFile::GetTypedefCount(bool bCountIncludes)
{
     if (!IsIDLFile())
         return 0;

     int nCount = 0;
     VectorElement *pIter = GetFirstTypedef();
     CFETypedDeclarator *pDecl;
     while ((pDecl = GetNextTypedef(pIter)) != 0)
     {
         nCount++;
     }

     if (!bCountIncludes)
         return nCount;

     pIter = GetFirstIncludeFile();
     CFEFile *pFile;
     while ((pFile = GetNextIncludeFile(pIter)) != 0)
     {
         nCount += pFile->GetTypedefCount();
     }

     return nCount;
}

/**	\brief checks if this file is a standard include file
 *	\return true if this is a standard include file
 */
bool CFEFile::IsStdIncludeFile()
{
    return (m_nStdInclude == 1);
}

/**	\brief returns the filename including the path
 *	\return a reference to m_sFileWithPath
 */
String CFEFile::GetFullFileName()
{
    return m_sFileWithPath;
}
