/**
 *    \file    dice/src/be/BEClass.h
 *    \brief   contains the declaration of the class CBEClass
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

#ifndef BECLASS_H
#define BECLASS_H

#include <be/BEObject.h>
#include <vector>
using namespace std;

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
class CFEAttributeDeclarator;

/** \struct CPredefinedFunctionID
 *  \ingroup backend
 *  \brief helper class to specify a user defined function ID (uuid attribute)
 */
typedef struct {
    /** \var string m_sName
     *  \brief the name of the function
     */
    string m_sName;
    /** \var int m_nNumber
     *  \brief its function id
     */
    int m_nNumber;
} CPredefinedFunctionID;

/** \class CFunctionGroup
 *  \ingroup backend
 *  \brief helper class to group multiple BE functions belonging to one FE function
 */
class CFunctionGroup : public CObject
{

public:
    /** \brief creates the function group
     *  \param pFEOperation the reference to the front-end function
     */
    CFunctionGroup(CFEOperation *pFEOperation);
    virtual ~CFunctionGroup();

    string GetName();
    CFEOperation *GetOperation();
    void AddFunction(CBEFunction *pFunction);
    vector<CBEFunction*>::iterator GetFirstFunction();
    CBEFunction *GetNextFunction(vector<CBEFunction*>::iterator &iter);

protected:
    /** \var CFEOperation *m_pFEOperation
     *  \brief the reference to the "origin" of this group
     */
    CFEOperation *m_pFEOperation;

    /** \var vector<CBEFunction*> m_vFunctions
     *  \brief the back-end function belonging to the front-end function
     */
    vector<CBEFunction*> m_vFunctions;
};

/** \class CBEClass
 *  \ingroup backend
 *  \brief represents the front-end interface
 *
 * This class is the back-end equivalent to the front-end interface.
 */
class CBEClass : public CBEObject
{

public:
    /** creates an instance of this class */
    CBEClass();
    virtual ~CBEClass();

public: // Public methods
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual int GetParameterCount(int nFEType, bool& bSameCount, int nDirection = 0);
    virtual int GetStringParameterCount(int nDirection = 0, int nMustAttrs = 0, int nMustNotAttrs = 0);
    virtual int GetSize(int nDirection, CBEContext *pContext);
    virtual string GetName();

    virtual CBEAttribute* GetNextAttribute(vector<CBEAttribute*>::iterator &iter);
    virtual vector<CBEAttribute*>::iterator GetFirstAttribute();
    virtual void RemoveAttribute(CBEAttribute *pAttribute);
    virtual CBEAttribute* FindAttribute(int nAttrType);
    virtual void AddAttribute(CBEAttribute *pAttribute);

    virtual CBEClass* GetNextBaseClass(vector<CBEClass*>::iterator &iter);
    virtual vector<CBEClass*>::iterator GetFirstBaseClass();
    virtual void RemoveBaseClass(CBEClass *pClass);
    virtual void AddBaseClass(CBEClass *pClass);

    virtual CBEClass* GetNextDerivedClass(vector<CBEClass*>::iterator &iter);
    virtual vector<CBEClass*>::iterator GetFirstDerivedClass();
    virtual void RemoveDerivedClass(CBEClass *pClass);
    virtual void AddDerivedClass(CBEClass *pClass);

    virtual CBEConstant* GetNextConstant(vector<CBEConstant*>::iterator &iter);
    virtual vector<CBEConstant*>::iterator GetFirstConstant();
    virtual void RemoveConstant(CBEConstant *pConstant);
    virtual void AddConstant(CBEConstant *pConstant);
    virtual CBEConstant* FindConstant(string sConstantName);

    virtual CBETypedDeclarator* GetNextTypedef(vector<CBETypedDeclarator*>::iterator &iter);
    virtual vector<CBETypedDeclarator*>::iterator GetFirstTypedef();
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual void AddTypedef(CBETypedef *pTypedef);
    virtual CBETypedDeclarator* FindTypedef(string sTypeName);

    virtual CBEFunction* GetNextFunction(vector<CBEFunction*>::iterator &iter);
    virtual vector<CBEFunction*>::iterator GetFirstFunction();
    virtual void RemoveFunction(CBEFunction *pFunction);
    virtual void AddFunction(CBEFunction *pFunction);
    virtual CBEFunction* FindFunction(string sFunctionName);

    virtual bool AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual int GetClassNumber();
    virtual CBENameSpace* GetNameSpace();

    virtual void Write(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void Write(CBEImplementationFile *pFile, CBEContext *pContext);

    virtual CFunctionGroup* FindFunctionGroup(CBEFunction *pFunction);

    virtual void RemoveTypeDeclaration(CBETypedDeclarator *pTypeDecl);
    virtual void AddTypeDeclaration(CBETypedDeclarator *pTypeDecl);
    virtual bool IsTargetFile(CBEHeaderFile * pFile);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);

    virtual CBEType* FindTaggedType(int nType, string sTag);
    virtual CBEType* GetNextTaggedType(vector<CBEType*>::iterator &iter);
    virtual vector<CBEType*>::iterator GetFirstTaggedType();
    virtual void RemoveTaggedType(CBEType *pType);
    virtual void AddTaggedType(CBEType *pType);
    virtual bool HasFunctionWithUserType(string sTypeName, CBEFile *pFile, CBEContext *pContext);
    virtual CBEMsgBufferType* GetMessageBuffer();
    virtual int GetParameterCount(int nMustAttrs, int nMustNotAttrs, int nDirection = 0);

    virtual bool HasParametersWithAttribute(int nAttribute1, int nAttribute2 = 0, int nAttribute3 = 0);

protected:
    virtual bool CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEAttributeDeclarator *pFEAttrDecl, CBEContext *pContext);
    virtual bool CreateFunctionsNoClassDependency(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool CreateFunctionsClassDependency(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext);
    virtual bool AddInterfaceFunctions(CFEInterface* pFEInterface, CBEContext* pContext);

    virtual bool CreateAliasForClass(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddOpcodesToFile(CFEOperation *pFEOperation, CBEHeaderFile *pFile, CBEContext *pContext);

    virtual void WriteTypedef(CBETypedDeclarator *pTypedef, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteTaggedType(CBEType *pType, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteConstant(CBEConstant *pConstant, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteFunction(CBEFunction *pFunction, CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteFunction(CBEFunction *pFunction, CBEImplementationFile *pFile, CBEContext *pContext);
    virtual void WriteHelperFunctions(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual void WriteHelperFunctions(CBEImplementationFile *pFile, CBEContext *pContext);

    virtual CFunctionGroup* GetNextFunctionGroup(vector<CFunctionGroup*>::iterator &iter);
    virtual vector<CFunctionGroup*>::iterator GetFirstFunctionGroup();
    virtual void RemoveFunctionGroup(CFunctionGroup *pGroup);
    virtual void AddFunctionGroup(CFunctionGroup *pGroup);

    virtual int GetOperationNumber(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool IsPredefinedID(vector<CPredefinedFunctionID> *pFunctionIDs, int nNumber);
    virtual int GetMaxOpcodeNumber(CFEInterface *pFEInterface);
    virtual int GetUuid(CFEOperation *pFEOperation);
    virtual bool HasUuid(CFEOperation *pFEOperation);
    virtual int GetInterfaceNumber(CFEInterface *pFEInterface);
    virtual int FindInterfaceWithNumber(CFEInterface *pFEInterface,
        int nNumber,
        vector<CFEInterface*> *pCollection);
    virtual int FindPredefinedNumbers(vector<CFEInterface*> *pCollection,
        vector<CPredefinedFunctionID> *pNumbers,
        CBEContext *pContext);
    virtual int CheckOpcodeCollision(CFEInterface *pFEInterface,
        int nOpNumber,
        vector<CFEInterface*> *pCollection,
        CFEOperation *pFEOperation,
        CBEContext *pContext);
    virtual void AddBaseName(string sName);
    virtual int GetFunctionCount(void);
    virtual int GetFunctionWriteCount(CBEFile *pFile, CBEContext *pContext);

    virtual void CreateOrderedElementList(void);
    void InsertOrderedElement(CObject *pObj);

protected: // Protected members
    /**    \var string m_sName
     *    \brief the name of the class
     */
    string m_sName;
    /** \var CBEMsgBufferType *m_pMsgBuffer
     *  \brief keep message buffer in seperate variable
     */
    CBEMsgBufferType *m_pMsgBuffer;
    /** \var vector<CBEFunction*> m_vFunctions
     *  \brief the operation functions, which are used for calculations
     */
    vector<CBEFunction*> m_vFunctions;
    /** \var vector<CBEConstant*> m_vConstants
     *  \brief the constants of the Class
     */
    vector<CBEConstant*> m_vConstants;
    /** \var  vector<CBETypedDeclarator*> m_vTypedefs
     *  \brief the type definition of the Class
     */
    vector<CBETypedDeclarator*> m_vTypedefs;
    /** \var vector<CBEType*> m_vTypeDeclarations
     *  \brief contains the tagged type declarations
     */
    vector<CBEType*> m_vTypeDeclarations;
    /** \var vector<CBEAttribute*> m_vAttributes
     *  \brief contains the attributes of the Class
     */
    vector<CBEAttribute*> m_vAttributes;
    /** \var vector<CBEClass*> m_vBaseClasses
     *  \brief contains references to the base classes
     */
    vector<CBEClass*> m_vBaseClasses;
    /** \var vector<CBEClass*> m_vDerivedClasses
     *  \brief contains references to the derived classes
     */
    vector<CBEClass*> m_vDerivedClasses;
    /** \var vector<CFunctionGroup*> m_vFunctionGroups
     *  \brief contains function groups (BE-functions grouped by their FE-functions)
     */
    vector<CFunctionGroup*> m_vFunctionGroups;
    /** \var vector<string> m_vBaseNames
     *  \brief contains the names to the base interfaces
     */
    vector<string> m_vBaseNames;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;
};

#endif
