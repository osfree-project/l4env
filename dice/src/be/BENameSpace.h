/**
 *  \file    dice/src/be/BENameSpace.h
 *  \brief   contains the declaration of the class CBENameSpace
 *
 *  \date    Tue Jun 25 2002
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

/** preprocessing symbol to check header file */

#ifndef BENAMESPACE_H
#define BENAMESPACE_H

#include "BEObject.h"
#include "BEContext.h" // FUNCTION_TYPE
#include "template.h"
#include <vector>

class CBEAttribute;
class CBEConstant;
class CBETypedef;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEClass;
class CBEType;
class CBEEnumType;

class CFELibrary;
class CFEInterface;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEAttribute;
class CFEConstructedType;

/** \class CBENameSpace
 *  \ingroup backend
 *  \brief represents a front-end library
 *
 * This class represents the back-end equivalent to the front-end library
 */
class CBENameSpace : public CBEObject
{
public:
    /** creates an instance of the namespace class */
    CBENameSpace();

public: // Public methods
    virtual void CreateBackEnd(CFELibrary *pFELibrary);
    virtual void AddToHeader(CBEHeaderFile* pHeader);
    virtual void AddToImpl(CBEImplementationFile* pImpl);

    /** \brief retrieves the name of the NameSpace
     *  \return the name of the lib
     */
    std::string GetName()
    { return m_sName; }

    /** \brief tries to match the name of the namespace
     *  \param sName the name to match against
     *  \return true if name matches parameter
     */
    bool Match(std::string sName)
    { return GetName() == sName; }

    CBEConstant* FindConstant(std::string sConstantName);
    CBETypedef* FindTypedef(std::string sTypeName, CBETypedef* pPrev = 0);
    CBEClass* FindClass(std::string sClassName, CBEClass *pPrev = 0);
    CBENameSpace* FindNameSpace(std::string sNameSpaceName);
    CBEType* FindTaggedType(int nType, std::string sTag);
    CBEEnumType* FindEnum(std::string sName);

    void AddOpcodesToFile(CBEHeaderFile* pFile);

    virtual void Write(CBEImplementationFile& pFile);
    virtual void Write(CBEHeaderFile& pFile);
    virtual void WriteElements(CBEImplementationFile& pFile);
    virtual void WriteElements(CBEHeaderFile& pFile);

    virtual CBEFunction* FindFunction(std::string sFunctionName,
	FUNCTION_TYPE nFunctionType);
    virtual bool IsTargetFile(CBEFile* pFile);

    virtual bool HasFunctionWithUserType(std::string sTypeName, CBEFile* pFile);

protected: // Protected methods
    virtual void CreateBackEnd(CFEInterface *pFEInterface);
    virtual void CreateBackEnd(CFETypedDeclarator *pFETypedef);
    virtual void CreateBackEnd(CFEConstDeclarator *pFEConstant);
    virtual void CreateBackEnd(CFEConstructedType *pFEType);
    virtual void CreateBackEnd(CFEAttribute *pFEAttribute);

    virtual void WriteConstant(CBEConstant *pConstant, CBEHeaderFile& pFile);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, CBEImplementationFile& pFile);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, CBEHeaderFile& pFile);
    virtual void WriteClass(CBEClass *pClass, CBEImplementationFile& pFile);
    virtual void WriteClass(CBEClass *pClass, CBEHeaderFile& pFile);
    virtual void WriteTypedef(CBETypedef *pTypedef, CBEHeaderFile& pFile);
    virtual void WriteTaggedType(CBEType *pType, CBEHeaderFile& pFile);

    virtual void CreateOrderedElementList();
    void InsertOrderedElement(CObject *pObj);

protected: // Protected attributes
    /** \var std::string m_sName
     *  \brief the name of the library
     */
    std::string m_sName;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;

public:
    /** \var CSearchableCollection<CBEConstant, std::string> m_Constants
     *  \brief contains the constants of this library
     */
    CSearchableCollection<CBEConstant, std::string> m_Constants;
    /** \var CSearchableCollection<CBETypedef, std::string> m_Typedefs
     *  \brief contains the typedefs of this library
     */
    CSearchableCollection<CBETypedef, std::string> m_Typedefs;
    /** \var CCollection<CBEAttribute> m_Attributes
     *  \brief contains the attributes of this library
     */
    CCollection<CBEAttribute> m_Attributes;
    /** \var CSearchableCollection<CBEClass, std::string> m_Classes
     *  \brief contains the classes of this namespace
     */
    CSearchableCollection<CBEClass, std::string> m_Classes;
    /** \var CCollection<CBENameSpace> m_NestedNamespaces
     *  \brief contains the nested namespaces
     */
    CCollection<CBENameSpace> m_NestedNamespaces;
    /** \var CCollection<CBEType> m_TypeDeclarations
     *  \brief contains the type declarations
     */
    CCollection<CBEType> m_TypeDeclarations;
};

#endif
