/**
 *    \file    dice/src/be/BENameSpace.h
 *    \brief   contains the declaration of the class CBENameSpace
 *
 *    \date    Tue Jun 25 2002
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

/** preprocessing symbol to check header file */

#ifndef BENAMESPACE_H
#define BENAMESPACE_H

#include <be/BEObject.h>
#include <vector>
using namespace std;


class CBEAttribute;
class CBEConstant;
class CBETypedef;
class CBEContext;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEClass;
class CBEType;

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
    ~CBENameSpace();

public: // Public methods
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual string GetName();

    virtual CBEConstant* GetNextConstant(vector<CBEConstant*>::iterator &iter);
    virtual vector<CBEConstant*>::iterator GetFirstConstant();
    virtual void RemoveConstant(CBEConstant *pConstant);
    virtual void AddConstant(CBEConstant *pConstant);
    virtual CBEConstant* FindConstant(string sConstantName);

    virtual CBETypedef* GetNextTypedef(vector<CBETypedef*>::iterator &iter);
    virtual vector<CBETypedef*>::iterator GetFirstTypedef();
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual void AddTypedef(CBETypedef *pTypedef);

    virtual void AddAttribute(CBEAttribute *pAttribute);
    virtual CBEAttribute* GetNextAttribute(vector<CBEAttribute*>::iterator &iter);
    virtual vector<CBEAttribute*>::iterator GetFirstAttribute();
    virtual void RemoveAttribute(CBEAttribute *pAttribute);

    virtual CBEClass* GetNextClass(vector<CBEClass*>::iterator &iter);
    virtual vector<CBEClass*>::iterator GetFirstClass();
    virtual void RemoveClass(CBEClass *pClass);
    virtual void AddClass(CBEClass *pClass);
    virtual CBEClass* FindClass(string sClassName);

    virtual CBENameSpace* GetNextNameSpace(vector<CBENameSpace*>::iterator &iter);
    virtual vector<CBENameSpace*>::iterator GetFirstNameSpace();
    virtual void RemoveNameSpace(CBENameSpace *pNameSpace);
    virtual void AddNameSpace(CBENameSpace* pNameSpace);
    virtual CBENameSpace* FindNameSpace(string sNameSpaceName);

    virtual bool AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext);

    virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void Write(CBEHeaderFile *pFile, CBEContext *pContext);

    virtual CBEFunction* FindFunction(string sFunctionName);
    virtual CBETypedef* FindTypedef(string sTypeName);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool IsTargetFile(CBEHeaderFile * pFile);

    virtual CBEType* FindTaggedType(int nType, string sTag);
    virtual CBEType* GetNextTaggedType(vector<CBEType*>::iterator &iter);
    virtual vector<CBEType*>::iterator GetFirstTaggedType();
    virtual void RemoveTaggedType(CBEType *pType);
    virtual void AddTaggedType(CBEType *pType);
    virtual bool HasFunctionWithUserType(string sTypeName, CBEFile *pFile, CBEContext *pContext);

protected: // Protected methods
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext);

    virtual void WriteConstant(CBEConstant *pConstant, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteClass(CBEClass *pClass, CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void WriteClass(CBEClass *pClass, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTypedef(CBETypedef *pTypedef, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTaggedType(CBEType *pType, CBEHeaderFile *pFile, CBEContext *pContext);

    virtual void CreateOrderedElementList(void);
    void InsertOrderedElement(CObject *pObj);

protected: // Protected attributes
    /** \var vector<CBEClass*> m_vClasses
     *  \brief contains the classes of this namespace
     */
    vector<CBEClass*> m_vClasses;
    /** \var vector<CBENameSpace*> m_vNestedNamespaces
     *  \brief contains the nested namespaces
     */
    vector<CBENameSpace*> m_vNestedNamespaces;
    /** \var vector<CBEConstant*> m_vConstants
     *  \brief contains the constants of this library
     */
    vector<CBEConstant*> m_vConstants;
    /** \var vector<CBETypedef*> m_vTypedefs
     *  \brief contains the typedefs of this library
     */
    vector<CBETypedef*> m_vTypedefs;
    /** \var vector<CBEType*> m_vTypeDeclarations
     *  \brief contains the type declarations
     */
    vector<CBEType*> m_vTypeDeclarations;
    /** \var vector<CBEAttribute*> m_vAttributes
     *  \brief contains the attributes of this library
     */
    vector<CBEAttribute*> m_vAttributes;
    /** \var string m_sName
     *  \brief the name of the library
     */
    string m_sName;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;
};

#endif
