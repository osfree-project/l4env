/**
 *	\file	dice/src/be/BEClass.h
 *	\brief	contains the declaration of the class CBEClass
 *
 *	\date	Tue Jun 25 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#ifndef BECLASS_H
#define BECLASS_H

#include <be/BEObject.h>
#include "Vector.h"

class CBEContext;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEAttribute;
class CBEConstant;
class CBEType;
class CBETypedef;
class CBETypedDeclarator;
class CBEMsgBufferType;
class CBEFunction;
class CBENameSpace;
class CBEFile;

class CFEInterface;
class CFEAttribute;
class CFEConstDeclarator;
class CFETypedDeclarator;
class CFEOperation;
class CFEConstructedType;

/** \class CFunctionGroup
 *  \ingroup backend
 *  \brief helper class to group multiple BE functions belonging to one FE function
 */
class CFunctionGroup : public CObject
{
DECLARE_DYNAMIC(CFunctionGroup);
public:
    /** \brief creates the function group
     *  \param pFEOperation the reference to the front-end function
     */
    CFunctionGroup(CFEOperation *pFEOperation);
    virtual ~CFunctionGroup();

    String GetName();
    CFEOperation *GetOperation();
    void AddFunction(CBEFunction *pFunction);
    VectorElement *GetFirstFunction();
    CBEFunction *GetNextFunction(VectorElement *& pIter);

protected:
    /** \var CFEOperation *m_pFEOperation
     *  \brief the reference to the "origin" of this group
     */
    CFEOperation *m_pFEOperation;

    /** \var Vector m_vFunctions
     *  \brief the back-end function belonging to the front-end function
     */
    Vector m_vFunctions;
};

/** \class CBEClass
 *  \ingroup backend
 *  \brief represents the front-end interface
 *
 * This class is the back-end equivalent to the front-end interface.
 */
class CBEClass : public CBEObject
{
DECLARE_DYNAMIC(CBEClass);
public:
    /** creates an instance of this class */
	CBEClass();
	~CBEClass();

public: // Public methods
	virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual int GetParameterCount(int nFEType, bool& bSameCount, int nDirection = 0);
    virtual int GetStringParameterCount(int nDirection = 0, int nMustAttrs = 0, int nMustNotAttrs = 0);
    virtual int GetSize(int nDirection, CBEContext *pContext);
    virtual String GetName();

    virtual CBEAttribute* GetNextAttribute(VectorElement *&pIter);
    virtual VectorElement* GetFirstAttribute();
    virtual void RemoveAttribute(CBEAttribute *pAttribute);
    virtual CBEAttribute* FindAttribute(int nAttrType);
    virtual void AddAttribute(CBEAttribute *pAttribute);

    virtual CBEClass* GetNextBaseClass(VectorElement*&pIter);
    virtual VectorElement* GetFirstBaseClass();
    virtual void RemoveBaseClass(CBEClass *pClass);
    virtual void AddBaseClass(CBEClass *pClass);

    virtual CBEClass* GetNextDerivedClass(VectorElement*&pIter);
    virtual VectorElement* GetFirstDerivedClass();
    virtual void RemoveDerivedClass(CBEClass *pClass);
    virtual void AddDerivedClass(CBEClass *pClass);

    virtual CBEConstant* GetNextConstant(VectorElement*&pIter);
    virtual VectorElement* GetFirstConstant();
    virtual void RemoveConstant(CBEConstant *pConstant);
    virtual void AddConstant(CBEConstant *pConstant);
	virtual CBEConstant* FindConstant(String sConstantName);

    virtual CBETypedDeclarator* GetNextTypedef(VectorElement *&pIter);
    virtual VectorElement* GetFirstTypedef();
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual void AddTypedef(CBETypedef *pTypedef);
    virtual CBETypedDeclarator* FindTypedef(String sTypeName);

    virtual CBEFunction* GetNextFunction(VectorElement *&pIter);
    virtual VectorElement* GetFirstFunction();
    virtual void RemoveFunction(CBEFunction *pFunction);
    virtual void AddFunction(CBEFunction *pFunction);
    virtual CBEFunction* FindFunction(String sFunctionName);

    virtual bool AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual int GetClassNumber();
    virtual CBENameSpace* GetNameSpace();
    virtual int Optimize(int nLevel, CBEContext *pContext);

    virtual void Write(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);

    virtual CFunctionGroup* FindFunctionGroup(CBEFunction *pFunction);

	virtual void RemoveTypeDeclaration(CBETypedDeclarator *pTypeDecl);
	virtual void AddTypeDeclaration(CBETypedDeclarator *pTypeDecl);
	virtual bool IsTargetFile(CBEHeaderFile * pFile);
	virtual bool IsTargetFile(CBEImplementationFile * pFile);

     virtual CBEType* FindTaggedType(int nType, String sTag);
     virtual CBEType* GetNextTaggedType(VectorElement* &pIter);
     virtual VectorElement* GetFirstTaggedType();
     virtual void RemoveTaggedType(CBEType *pType);
     virtual void AddTaggedType(CBEType *pType);
     virtual bool HasFunctionWithUserType(String sTypeName, CBEFile *pFile, CBEContext *pContext);
     virtual CBEMsgBufferType* GetMessageBuffer();
     virtual int GetParameterCount(int nMustAttrs, int nMustNotAttrs, int nDirection = 0);

protected:
    virtual bool CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext);
	virtual bool AddInterfaceFunctions(CFEInterface* pFEInterface, CBEContext* pContext);

	virtual bool CreateAliasForClass(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddOpcodesToFile(CFEOperation *pFEOperation, CBEHeaderFile *pFile, CBEContext *pContext);

    virtual void WriteTypedefs(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTaggedTypes(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteConstants(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteFunctions(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteFunctions(CBEImplementationFile *pFile, CBEContext *pContext);

    virtual CFunctionGroup* GetNextFunctionGroup(VectorElement *& pIter);
    virtual VectorElement* GetFirstFunctionGroup();
    virtual void RemoveFunctionGroup(CFunctionGroup *pGroup);
    virtual void AddFunctionGroup(CFunctionGroup *pGroup);

    virtual int GetOperationNumber(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool IsPredefinedID(Vector *pFunctionIDs, int nNumber);
    virtual int GetMaxOpcodeNumber(CFEInterface *pFEInterface, Vector *pFunctionIDs);
    virtual int GetUuid(CFEOperation *pFEOperation);
    virtual bool HasUuid(CFEOperation *pFEOperation);
    virtual int GetInterfaceNumber(CFEInterface *pFEInterface);
    virtual int FindInterfaceWithNumber(CFEInterface *pFEInterface, int nNumber, Vector *pCollection);
    virtual int FindPredefinedNumbers(Vector *pCollection, Vector *pNumbers, CBEContext *pContext);
    virtual int CheckOpcodeCollision(CFEInterface *pFEInterface, int nOpNumber, Vector *pCollection, CFEOperation *pFEOperation, CBEContext *pContext);
    virtual void AddBaseName(String sName);


protected: // Protected members
    /**	\var String m_sName
     *	\brief the name of the class
     */
    String m_sName;
    /** \var CBEMsgBufferType *m_pMsgBuffer
     *  \brief keep message buffer in seperate variable
     */
    CBEMsgBufferType *m_pMsgBuffer;
    /**	\var Vector m_vFunctions
     *	\brief the operation functions, which are used for calculations
     */
    Vector m_vFunctions;
    /** \var Vector m_vConstants
     *  \brief the constants of the Class
     */
    Vector m_vConstants;
    /** \var Vector m_vTypedefs
     *  \brief the type definition of the Class
     */
    Vector m_vTypedefs;
    /** \var Vector m_vTypeDeclarations
     *  \brief contains the tagged type declarations
     */
    Vector m_vTypeDeclarations;
    /** \var Vector m_vAttributes
     *  \brief contains the attributes of the Class
     */
    Vector m_vAttributes;
    /** \var Vector m_vBaseClasses
     *  \brief contains references to the base classes
     */
    Vector m_vBaseClasses;
    /** \var Vector m_vDerivedClasses
     *  \brief contains references to the derived classes
     */
    Vector m_vDerivedClasses;
    /** \var Vector m_vFunctionGroups
     *  \brief contains function groups (BE-functions grouped by their FE-functions)
     */
    Vector m_vFunctionGroups;
    /** \var String *m_sBaseNames
     *  \brief contains the names to the base interfaces
     */
    String **m_sBaseNames;
    /** \var int m_nBaseNameSize
     *  \brief contains the size of the m_sBaseNames array
     */
    int m_nBaseNameSize;
};

#endif
