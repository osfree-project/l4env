/**
 *	\file	dice/src/be/BENameSpace.h
 *	\brief	contains the declaration of the class CBENameSpace
 *
 *	\date	Tue Jun 25 2002
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

/** preprocessing symbol to check header file */

#ifndef BENAMESPACE_H
#define BENAMESPACE_H

#include <be/BEObject.h>

#include "Vector.h"

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
DECLARE_DYNAMIC(CBENameSpace);
public:
    /** creates an instance of the namespace class */
	CBENameSpace();
	~CBENameSpace();

public: // Public methods
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual String GetName();

    virtual CBEConstant* GetNextConstant(VectorElement *&pIter);
    virtual VectorElement* GetFirstConstant();
    virtual void RemoveConstant(CBEConstant *pConstant);
    virtual void AddConstant(CBEConstant *pConstant);

    virtual CBETypedef* GetNextTypedef(VectorElement *&pIter);
    virtual VectorElement* GetFirstTypedef();
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual void AddTypedef(CBETypedef *pTypedef);

    virtual void AddAttribute(CBEAttribute *pAttribute);
    virtual CBEAttribute* GetNextAttribute(VectorElement *&pIter);
    virtual VectorElement* GetFirstAttribute();
    virtual void RemoveAttribute(CBEAttribute *pAttribute);

    virtual CBEClass* GetNextClass(VectorElement* &pIter);
    virtual VectorElement* GetFirstClass();
    virtual void RemoveClass(CBEClass *pClass);
    virtual void AddClass(CBEClass *pClass);
    virtual CBEClass* FindClass(String sClassName);

    virtual CBENameSpace* GetNextNameSpace(VectorElement* &pIter);
    virtual VectorElement* GetFirstNameSpace();
    virtual void RemoveNameSpace(CBENameSpace *pNameSpace);
    virtual void AddNameSpace(CBENameSpace* pNameSpace);
    virtual CBENameSpace* FindNameSpace(String sNameSpaceName);

    virtual bool AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual int Optimize(int nLevel, CBEContext *pContext);

    virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void Write(CBEHeaderFile *pFile, CBEContext *pContext);

    virtual CBEFunction* FindFunction(String sFunctionName);
    virtual CBETypedef* FindTypedef(String sTypeName);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool IsTargetFile(CBEHeaderFile * pFile);

    virtual CBEType* FindTaggedType(int nType, String sTag);
    virtual CBEType* GetNextTaggedType(VectorElement* &pIter);
    virtual VectorElement* GetFirstTaggedType();
    virtual void RemoveTaggedType(CBEType *pType);
    virtual void AddTaggedType(CBEType *pType);
    virtual bool HasFunctionWithUserType(String sTypeName, CBEFile *pFile, CBEContext *pContext);

protected: // Protected methods
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext);

    virtual void WriteConstants(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteNameSpaces(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void WriteNameSpaces(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteClasses(CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void WriteClasses(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTypedefs(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTaggedTypes(CBEHeaderFile *pFile, CBEContext *pContext);

protected: // Protected attributes
    /** \var Vector m_vClasses
     *  \brief contains the classes of this namespace
     */
    Vector m_vClasses;
    /** \var Vector m_vNestedNamespaces
     *  \brief contains the nested namespaces
     */
    Vector m_vNestedNamespaces;
    /** \var Vector m_vConstants
     *  \brief contains the constants of this library
     */
    Vector m_vConstants;
    /** \var Vector m_vTypedefs
     *  \brief contains the typedefs of this library
     */
    Vector m_vTypedefs;
    /** \var Vector m_vTypeDeclarations
     *  \brief contains the type declarations
     */
    Vector m_vTypeDeclarations;
    /** \var Vector m_vAttributes
     *  \brief contains the attributes of this library
     */
    Vector m_vAttributes;
    /** \var String m_sName
     *  \brief the name of the library
     */
    String m_sName;
};

#endif
