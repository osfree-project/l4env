/**
 *  \file    dice/src/be/BEClass.h
 *  \brief   contains the declaration of the class CBEClass
 *
 *  \date    Tue Jun 25 2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BEObject.h"
#include "BEContext.h" /* for FUNCTION_TYPE */
#include "BEFunction.h"
#include "Attribute-Type.h" /* for ATTR_TYPE */
#include "template.h"
#include <vector>
#include <map>
using std::map;

class CBEHeaderFile;
class CBEImplementationFile;
class CBEAttribute;
class CBEConstant;
class CBEType;
class CBETypedef;
class CBETypedDeclarator;
class CBEEnumType;
class CBEFunction;
class CBENameSpace;
class CBEFile;
class CBEMsgBuffer;
class CBESrvLoopFunction;
class CBEException;

class CFEInterface;
class CFEAttribute;
class CFEConstDeclarator;
class CFETypedDeclarator;
class CFEOperation;
class CFEConstructedType;
class CFEAttributeDeclarator;

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
    ~CFunctionGroup();

    std::string GetName();
    CFEOperation *GetOperation();

protected:
    /** \var CFEOperation *m_pFEOperation
     *  \brief the reference to the "origin" of this group
     */
    CFEOperation *m_pFEOperation;

public:
    /** \var CCollection<CBEFunction> m_Functions
     *  \brief the back-end function belonging to the front-end function
     */
    CCollection<CBEFunction> m_Functions;
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
    ~CBEClass();

public: // Public methods
    void CreateBackEnd(CFEInterface *pFEInterface);
    void AddToHeader(CBEHeaderFile* pHeader);
    void AddToImpl(CBEImplementationFile* pImpl);
    int GetParameterCount(int nFEType, bool& bSameCount, DIRECTION_TYPE nDirection);
    int GetStringParameterCount(DIRECTION_TYPE nDirection,
	ATTR_TYPE nMustAttrs = ATTR_NONE, ATTR_TYPE nMustNotAttrs = ATTR_NONE);
    int GetSize(DIRECTION_TYPE nDirection);
    std::string GetName();

    /** \brief try to match with the name
     *  \param sName the name to match against
     *  \return true if given name matches internal name
     */
    bool Match(std::string sName)
    { return GetName() == sName; }

    void Write(CBEHeaderFile& pFile);
    void Write(CBEImplementationFile& pFile);
    void WriteClassName(CBEFile& pFile);

    void AddOpcodesToFile(CBEHeaderFile* pFile);
    int GetClassNumber();

    CFunctionGroup* FindFunctionGroup(CBEFunction *pFunction);
    CBESrvLoopFunction* GetSrvLoopFunction();
    CBEType* FindTaggedType(int nType, std::string sTag);
    CBETypedef* FindTypedef(std::string sTypeName, CBETypedef* pPrev = 0);
    CBEFunction* FindFunction(std::string sFunctionName, FUNCTION_TYPE nFunctionType);
    CBEEnumType* FindEnum(std::string sName);

    bool IsTargetFile(CBEFile* pFile);

    bool HasFunctionWithUserType(std::string sTypeName, CBEFile* pFile);
	bool HasParameterWithAttributes(ATTR_TYPE nAttribute1, ATTR_TYPE nAttribute2);

	bool HasMallocParameters();
    bool HasFunctionWithAttribute(ATTR_TYPE nAttribute);

    /** \brief retrieve a handle to the message buffer
     *  \return a reference to the message buffer
     */
    CBEMsgBuffer* GetMessageBuffer()
    { return m_pMsgBuffer; }

protected:
    void CreateBackEndConst(CFEConstDeclarator *pFEConstant);
    void CreateBackEndTypedef(CFETypedDeclarator *pFETypedef);
    void CreateBackEndAttrDecl(CFEAttributeDeclarator *pFEAttrDecl);
    void CreateBackEndAttribute(CFEAttribute *pFEAttribute);
    void CreateBackEndTaggedDecl(CFEConstructedType *pFEType);
    void CreateBackEndException(CFETypedDeclarator *pFEException);

    void CreateFunctionsNoClassDependency(CFEOperation *pFEOperation);
    void CreateFunctionsClassDependency(CFEOperation *pFEOperation);
    virtual void CreateObject();
    virtual void CreateEnvironment();

    void AddInterfaceFunctions(CFEInterface* pFEInterface);
    void AddMessageBuffer(CFEInterface* pFEInterface);

    void CreateAliasForClass(CFEInterface *pFEInterface);
    void AddOpcodesToFile(CFEOperation *pFEOperation, CBEHeaderFile* pFile);
	void AddOpcodesToFile(CFEOperation *pFEOperation, int nNumber, std::string sName, CBEHeaderFile* pFile);

    void WriteElements(CBEHeaderFile& pFile);
    void WriteElements(CBEImplementationFile& pFile);
    void WriteFunctions(CBEHeaderFile& pFile);
    void WriteFunctions(CBEImplementationFile& pFile);
    void WriteBaseClasses(CBEFile& pFile);

    virtual void WriteMemberVariables(CBEHeaderFile& pFile);
    virtual void WriteConstructor(CBEHeaderFile& pFile);
    virtual void WriteDestructor(CBEHeaderFile& pFile);

    void WriteTypedef(CBETypedef *pTypedef, CBEHeaderFile& pFile);
    void WriteTaggedType(CBEType *pType, CBEHeaderFile& pFile);
    void WriteConstant(CBEConstant *pConstant, CBEHeaderFile& pFile);
    void WriteFunction(CBEFunction *pFunction, CBEHeaderFile& pFile);
    void WriteFunction(CBEFunction *pFunction, CBEImplementationFile& pFile);
    virtual void WriteHelperFunctions(CBEHeaderFile& pFile);
    virtual void WriteHelperFunctions(CBEImplementationFile& pFile);
    virtual void WriteDefaultFunction(CBEHeaderFile& pFile);

    int GetOperationNumber(CFEOperation *pFEOperation);
    bool IsPredefinedID(map<unsigned int, std::string> *pFunctionIDs,
	int nNumber);
    int GetMaxOpcodeNumber(CFEInterface *pFEInterface);
    int GetUuid(CFEOperation *pFEOperation);
    bool HasUuid(CFEOperation *pFEOperation);
    int GetInterfaceNumber(CFEInterface *pFEInterface);
    int FindInterfaceWithNumber(CFEInterface *pFEInterface,
        int nNumber,
        vector<CFEInterface*> *pCollection);
    int FindPredefinedNumbers(vector<CFEInterface*> *pCollection,
        map<unsigned int, std::string> *pNumbers);
    int CheckOpcodeCollision(CFEInterface *pFEInterface,
        int nOpNumber,
        vector<CFEInterface*> *pCollection,
        CFEOperation *pFEOperation);
    void CheckOpcodeCollision(CFEInterface *pFEInterface);
    void CheckOpcodeCollision(CFEInterface *pFirst, CFEInterface *pSecond);
    void AddBaseClass(std::string sName);
    int GetFunctionCount();
    int GetFunctionWriteCount(CBEFile& pFile);

    virtual void MsgBufferInitialization();

    void CreateOrderedElementList();
    void InsertOrderedElement(CObject *pObj);

    void WriteExternCStart(CBEFile& pFile);
    void WriteExternCEnd(CBEFile& pFile);
    void WriteLineDirective(CBEFile& pFile, CObject *pObj);

protected: // Protected members
    /** \var std::string m_sName
     *  \brief the name of the class
     */
    std::string m_sName;
    /** \var CBEMsgBuffer *m_pMsgBuffer
     *  \brief a reference to the server's message buffer
     */
    CBEMsgBuffer *m_pMsgBuffer;
    /** \var vector<CObject*> m_vOrderedElements
     *  \brief contains ordered list of elements
     */
    vector<CObject*> m_vOrderedElements;
    /** \var CBETypedDeclarator *m_pCorbaObject
     *  \brief contains a reference to the CORBA Object parameter
     */
    CBETypedDeclarator *m_pCorbaObject;
    /** \var CBETypedDeclarator *m_pCorbaEnv
     *  \brief contains a reference to the CORBA Environment parameter
     */
    CBETypedDeclarator *m_pCorbaEnv;

public:
    /** \var vector<CBEClass*> m_BaseClasses
     *  \brief contains references to the base classes
     */
    vector<CBEClass*> m_BaseClasses;
    /** \var CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes
     *  \brief contains the attributes of the Class
     */
    CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CSearchableCollection<CBEConstant, std::string> m_Constants
     *  \brief the constants of the Class
     */
    CSearchableCollection<CBEConstant, std::string> m_Constants;
    /** \var CCollection<CBEType> m_TypeDeclarations
     *  \brief contains the tagged type declarations
     */
    CCollection<CBEType> m_TypeDeclarations;
    /** \var CCollection<CBETypedef> m_Typedefs
     *  \brief the type definition of the Class
     */
    CCollection<CBETypedef> m_Typedefs;
    /** \var CCollection<CBEFunction> m_Functions
     *  \brief the operation functions, which are used for calculations
     */
    CCollection<CBEFunction> m_Functions;
    /** \var CCollection<CFunctionGroup> m_FunctionGroups
     *  \brief contains function groups (BE-functions grouped by their FE-functions)
     */
    CCollection<CFunctionGroup> m_FunctionGroups;
    /** \var CCollection<CBEClass> m_DerivedClasses
     *  \brief contains references to the derived classes
     */
    CCollection<CBEClass> m_DerivedClasses;
};

#endif
