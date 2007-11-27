/**
 *    \file    dice/src/be/BEClass.cpp
 *    \brief   contains the implementation of the class CBEClass
 *
 *    \date    Tue Jun 25 2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEClass.h"

#include "BEFunction.h"
#include "BEOperationFunction.h"
#include "BEConstant.h"
#include "BETypedef.h"
#include "BEEnumType.h"
#include "BEAttribute.h"
#include "BEContext.h"
#include "BERoot.h"
#include "BEComponent.h"
#include "BEOperationFunction.h"
#include "BEInterfaceFunction.h"
#include "BECallFunction.h"
#include "BECppCallWrapperFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEMarshalFunction.h"
#include "BEMarshalExceptionFunction.h"
#include "BEReplyFunction.h"
#include "BEComponentFunction.h"
#include "BESndFunction.h"
#include "BEWaitFunction.h"
#include "BEWaitAnyFunction.h"
#include "BESrvLoopFunction.h"
#include "BEDispatchFunction.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEClient.h"
#include "BEOpcodeType.h"
#include "BEExpression.h"
#include "BENameSpace.h"
#include "BEDeclarator.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEUserDefinedType.h"
#include "BEMsgBuffer.h"
#include "BEClass.h"
#include "BEException.h"
#include "BEClassFactory.h"
#include "BENameFactory.h"

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEOperation.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEIntAttribute.h"
#include "fe/FERangeAttribute.h"
#include "fe/FEConstructedType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FEAttributeDeclarator.h"
#include "fe/FESimpleType.h"
#include "fe/FEAttribute.h"
#include "fe/FEFile.h"

#include "Compiler.h"
#include "Error.h"
#include "Messages.h"

#include <string>
#include <cassert>
#include <iostream>
#include <typeinfo> // for bad_cast

// CFunctionGroup IMPLEMENTATION

CFunctionGroup::CFunctionGroup(CFEOperation *pFEOperation)
: m_Functions(0, (CObject*)0)
{
	m_pFEOperation = pFEOperation;
}

/** \brief destroys a function group object
 *
 * This doesn't really do anything: The string object cleans up itself, and
 * the vector contains references to objects we do not want to delete. What we have
 * to remove are the vector-elements.
 */
CFunctionGroup::~CFunctionGroup()
{ }

/** \brief retrieves the name of the group
 *  \return the name of the group
 */
string CFunctionGroup::GetName()
{
	return m_pFEOperation->GetName();
}

/** \brief access the front-end operation member
 *  \return a reference to the front-end operation member
 */
CFEOperation *CFunctionGroup::GetOperation()
{
	return m_pFEOperation;
}

// CBEClass IMPLEMENTATION

CBEClass::CBEClass()
: m_Attributes(0, this),
	m_Constants(0, this),
	m_TypeDeclarations(0, this),
	m_Typedefs(0, this),
	m_Functions(0, this),
	m_FunctionGroups(0, this),
	m_DerivedClasses(0, (CObject*)0)
{
	m_pMsgBuffer = 0;
	m_pCorbaObject = 0;
	m_pCorbaEnv = 0;
}

/** \brief destructor of target class */
CBEClass::~CBEClass()
{
	if (m_pMsgBuffer)
		delete m_pMsgBuffer;
	if (m_pCorbaObject)
		delete m_pCorbaObject;
	if (m_pCorbaEnv)
		delete m_pCorbaEnv;
}

/** \brief returns the name of the class
 *  \return the name of the class
 */
string CBEClass::GetName()
{
	return m_sName;
}

/** \brief returns the number of functions in this class
 *  \return the number of functions in this class
 */
int CBEClass::GetFunctionCount()
{
	return m_Functions.size();
}

/** \brief returns the number of functions in this class which are written
 *  \return the number of functions in this class which are written
 */
int CBEClass::GetFunctionWriteCount(CBEFile& pFile)
{
	return std::count_if(m_Functions.begin(), m_Functions.end(), WriteCount(&pFile));
}

/** \brief adds a new base class from a name
 *  \param sName the name of the class to add
 *
 * search for the class and if found add it
 */
void CBEClass::AddBaseClass(std::string sName)
{
	// if we cannot find class it is not there, because this should be
	// called way after all classes are created
	CBEClass *pBaseClass = FindClass(sName);
	assert(pBaseClass);
	m_BaseClasses.push_back(pBaseClass);
	// if we add a base class, we add us to that class' derived classes
	pBaseClass->m_DerivedClasses.Add(this);
}

/** \brief creates the members of this class
 *  \param pFEInterface the front-end interface to use as source
 *  \return true if successful
 */
void CBEClass::CreateBackEnd(CFEInterface * pFEInterface)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(interf: %s) called\n",
		__func__, pFEInterface->GetName().c_str());

	// call CBEObject's CreateBackEnd method
	CBEObject::CreateBackEnd(pFEInterface);

	// set target file name
	CFEFile *pFEFile = pFEInterface->GetSpecificParent<CFEFile>();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEClass::%s(interf: %s) set target file, if has parent file %s\n",
		__func__, pFEInterface->GetName().c_str(),
		pFEFile ? pFEFile->GetFileName().c_str() : "(no file)");
	SetTargetFileName(pFEInterface);
	// set own name
	m_sName = pFEInterface->GetName();
	// add attributes
	vector<CFEAttribute*>::iterator iterA;
	for (iterA = pFEInterface->m_Attributes.begin();
		iterA != pFEInterface->m_Attributes.end();
		iterA++)
	{
		CreateBackEndAttribute(*iterA);
	}
	// we can resolve this if we only add the base names now, but when the
	// base classes are first used, add the actual references.
	// add references to base Classes
	vector<CFEIdentifier*>::iterator iterBI;
	for (iterBI = pFEInterface->m_BaseInterfaceNames.begin();
		iterBI != pFEInterface->m_BaseInterfaceNames.end();
		iterBI++)
	{
		// add base class
		AddBaseClass((*iterBI)->GetName());
	}
	// add constants
	vector<CFEConstDeclarator*>::iterator iterC;
	for (iterC = pFEInterface->m_Constants.begin();
		iterC != pFEInterface->m_Constants.end();
		iterC++)
	{
		CreateBackEndConst(*iterC);
	}
	// add typedefs
	vector<CFETypedDeclarator*>::iterator iterTD;
	for (iterTD = pFEInterface->m_Typedefs.begin();
		iterTD != pFEInterface->m_Typedefs.end();
		iterTD++)
	{
		CreateBackEndTypedef(*iterTD);
	}
	// add exceptions
	for (iterTD = pFEInterface->m_Exceptions.begin();
		iterTD != pFEInterface->m_Exceptions.end();
		iterTD++)
	{
		CreateBackEndException(*iterTD);
	}
	// add tagged decls
	vector<CFEConstructedType*>::iterator iterT;
	for (iterT = pFEInterface->m_TaggedDeclarators.begin();
		iterT != pFEInterface->m_TaggedDeclarators.end();
		iterT++)
	{
		CreateBackEndTaggedDecl(*iterT);
	}
	// add types for Class (only for C)
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
		CreateAliasForClass(pFEInterface);
	// first create operation functions
	vector<CFEOperation*>::iterator iterO;
	for (iterO = pFEInterface->m_Operations.begin();
		iterO != pFEInterface->m_Operations.end();
		iterO++)
	{
		CreateFunctionsNoClassDependency(*iterO);
	}
	// create class' message buffer
	AddMessageBuffer(pFEInterface);
	// create functions that depend on class' message buffer
	for (iterO = pFEInterface->m_Operations.begin();
		iterO != pFEInterface->m_Operations.end();
		iterO++)
	{
		CreateFunctionsClassDependency(*iterO);
	}
	// add attribute interface members
	vector<CFEAttributeDeclarator*>::iterator iterAD;
	for (iterAD = pFEInterface->m_AttributeDeclarators.begin();
		iterAD != pFEInterface->m_AttributeDeclarators.end();
		iterAD++)
	{
		CreateBackEndAttrDecl(*iterAD);
	}
	// add functions for interface
	AddInterfaceFunctions(pFEInterface);
	// add CORBA_Object and CORBA_Environment for server side C++
	// implementation
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		CreateObject();
		CreateEnvironment();
	}
	// check if any two base interfaces have same opcodes
	CheckOpcodeCollision(pFEInterface);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief add the message buffer
 *  \param pFEInterface the front-end interface to use as reference
 *  \return true if successful
 *
 * The class message buffer is used by functions in the interface, e.g.,
 * unmarshal and marshal functions.  Thus is should be declared just before
 * these functions.  On the other hand, the server message buffer might
 * contain other types, which are declared inside the interface and therefore
 * after the message buffer is defined.  For this reason the message buffer
 * should be defined at the end of the interface.
 *
 * To achieve this, we declare a typedef to a union declaration (note: no
 * definition) and define the message buffer after the interface.
 *
 */
void CBEClass::AddMessageBuffer(CFEInterface *pFEInterface)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(if: %s) called\n", __func__,
		pFEInterface->GetName().c_str());

	if (m_pMsgBuffer)
		delete m_pMsgBuffer;
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	m_pMsgBuffer = pCF->GetNewMessageBuffer();
	m_pMsgBuffer->SetParent(this);
	m_pMsgBuffer->CreateBackEnd(pFEInterface);

	// add platform specific members
	m_pMsgBuffer->AddPlatformSpecificMembers(this);
	// function specific initialization
	MsgBufferInitialization();
	// sort message buffer
	m_pMsgBuffer->Sort(this);
	// do post creation stuff
	m_pMsgBuffer->PostCreate(this, pFEInterface);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s returns\n", __func__);
}

/** \brief make function specific initialization
 *  \return true if successful
 */
void CBEClass::MsgBufferInitialization()
{
	// iterate function groups of class, pick a function and use it to
	// initialize struct
	vector<CFunctionGroup*>::iterator iter;
	for (iter = m_FunctionGroups.begin();
		iter != m_FunctionGroups.end();
		iter++)
	{
		// iterate the functions of the function group
		vector<CBEFunction*>::iterator iF;
		for (iF = (*iter)->m_Functions.begin();
			iF != (*iter)->m_Functions.end();
			iF++)
		{
			if ((dynamic_cast<CBECallFunction*>(*iF) != 0) ||
				(dynamic_cast<CBESndFunction*>(*iF) != 0) ||
				(dynamic_cast<CBEWaitFunction*>(*iF) != 0))
			{
				(*iF)->MsgBufferInitialization(m_pMsgBuffer);
			}
		}
	}
}

/** \brief adds the functions for an interface
 *  \param pFEInterface the interface to add the functions for
 *  \return true if successful
 */
void CBEClass::AddInterfaceFunctions(CFEInterface* pFEInterface)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	/* wait any */
	CBEInterfaceFunction *pFunction = pCF->GetNewWaitAnyFunction();
	m_Functions.Add(pFunction);
	pFunction->CreateBackEnd(pFEInterface, true);
	/* recv any for client */
	pFunction = pCF->GetNewRcvAnyFunction();
	m_Functions.Add(pFunction);
	pFunction->CreateBackEnd(pFEInterface, false);
	/* recv any for server */
	pFunction = pCF->GetNewRcvAnyFunction();
	m_Functions.Add(pFunction);
	pFunction->CreateBackEnd(pFEInterface, true);
	/* reply and wait any */
	pFunction = pCF->GetNewReplyAnyWaitAnyFunction();
	m_Functions.Add(pFunction);
	pFunction->CreateBackEnd(pFEInterface, true);
	if (!(CCompiler::IsOptionSet(PROGRAM_NO_DISPATCHER) &&
			CCompiler::IsOptionSet(PROGRAM_NO_SERVER_LOOP)))
	{
		pFunction = pCF->GetNewDispatchFunction();
		m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEInterface, true);
	}
	if (!CCompiler::IsOptionSet(PROGRAM_NO_SERVER_LOOP))
	{
		pFunction = pCF->GetNewSrvLoopFunction();
		m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEInterface, true);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief creates an alias type for the class
 *  \param pFEInterface the interface to use as reference
 *
 * In C we have an alias of CORBA_Object type to the name if the interface.
 * In C++ this is not needed, because the class is derived from CORBA_Object
 */
void CBEClass::CreateAliasForClass(CFEInterface *pFEInterface)
{
	assert(pFEInterface);
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(interface) called\n", __func__);
	// create the BE type
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedef *pTypedef = pCF->GetNewTypedef();
	m_Typedefs.Add(pTypedef);
	// create CORBA_Object type
	CBEUserDefinedType *pType = static_cast<CBEUserDefinedType*>(
		pCF->GetNewType(TYPE_USER_DEFINED));
	pType->SetParent(pTypedef);
	pType->CreateBackEnd(string("CORBA_Object"));
	// finally create typedef
	pTypedef->CreateBackEnd(pType, pFEInterface->GetName(), pFEInterface);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd

	// set source line and file
	pTypedef->m_sourceLoc = pFEInterface->m_sourceLoc;

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(interface) returns\n", __func__);
}

/** \brief internal function to create a constant
 *  \param pFEConstant the respective front-end constant
 */
void CBEClass::CreateBackEndConst(CFEConstDeclarator *pFEConstant)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(const) called\n",
		__func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEConstant *pConstant = pCF->GetNewConstant();
	m_Constants.Add(pConstant);
	pConstant->SetParent(this);
	pConstant->CreateBackEnd(pFEConstant);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(const) returns\n", __func__);
}

/** \brief internal function to create a typedefinition
 *  \param pFETypedef the respective front-end type definition
 */
void CBEClass::CreateBackEndTypedef(CFETypedDeclarator *pFETypedef)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(typedef) called\n", __func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedef *pTypedef = pCF->GetNewTypedef();
	m_Typedefs.Add(pTypedef);
	pTypedef->SetParent(this);
	pTypedef->CreateBackEnd(pFETypedef);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(typedef) returns\n", __func__);
}

/** \brief internal function to create a functions for attribute declarator
 *  \param pFEAttrDecl the respective front-end attribute declarator definition
 */
void CBEClass::CreateBackEndAttrDecl(CFEAttributeDeclarator *pFEAttrDecl)
{
	string exc = string(__func__);
	if (!pFEAttrDecl)
	{
		exc += " failed because attribute declarator is 0";
		throw new error::create_error(exc);
	}

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(attr-decl) called\n",
		__func__);

	// add function "<param_type_spec> [out]__get_<simple_declarator>();" for
	// each decl if !READONLY:
	//   add function "void [in]__set_<simple_declarator>(<param_type_spec>
	//   'first letter of <decl>');" for each decl
	vector<CFEDeclarator*>::iterator iterD;
	for (iterD = pFEAttrDecl->m_Declarators.begin();
		iterD != pFEAttrDecl->m_Declarators.end();
		iterD++)
	{
		// get function
		string sName = string("_get_");
		sName += (*iterD)->GetName();
		CFETypeSpec *pFEType = pFEAttrDecl->GetType()->Clone();
		CFEOperation *pFEOperation = new CFEOperation(pFEType, sName, 0);
		pFEType->SetParent(pFEOperation);
		// get parent interface
		CFEInterface *pFEInterface =
			pFEAttrDecl->GetSpecificParent<CFEInterface>();
		assert(pFEInterface);
		pFEInterface->m_Operations.Add(pFEOperation);
		CreateFunctionsNoClassDependency(pFEOperation);
		CreateFunctionsClassDependency(pFEOperation);

		// set function
		if (!pFEAttrDecl->m_Attributes.Find(ATTR_READONLY))
		{
			sName = string("_set_");
			sName += (*iterD)->GetName();
			CFEAttribute *pFEAttr = new CFEAttribute(ATTR_IN);
			vector<CFEAttribute*> *pFEAttributes = new vector<CFEAttribute*>();
			pFEAttributes->push_back(pFEAttr);
			pFEType = new CFESimpleType(TYPE_VOID);
			CFETypeSpec *pFEParamType = pFEAttrDecl->GetType()->Clone();
			CFEDeclarator *pFEParamDecl = new CFEDeclarator(DECL_IDENTIFIER,
				(*iterD)->GetName().substr(0,1));
			vector<CFEDeclarator*> *pFEParameters = new vector<CFEDeclarator*>();
			pFEParameters->push_back(pFEParamDecl);
			CFETypedDeclarator *pFEParam = new CFETypedDeclarator(TYPEDECL_PARAM,
				pFEParamType, pFEParameters, pFEAttributes);
			pFEParamType->SetParent(pFEParam);
			pFEParamDecl->SetParent(pFEParam);
			delete pFEAttributes;
			delete pFEParameters;
			// create function
			vector<CFETypedDeclarator*> *pParams =
				new vector<CFETypedDeclarator*>();
			pParams->push_back(pFEParam);
			pFEOperation = new CFEOperation(pFEType, sName, pParams);
			delete pParams;
			pFEType->SetParent(pFEOperation);
			pFEAttr->SetParent(pFEOperation);
			pFEParam->SetParent(pFEOperation);
			pFEInterface->m_Operations.Add(pFEOperation);

			CreateFunctionsNoClassDependency(pFEOperation);
			CreateFunctionsClassDependency(pFEOperation);
		}
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *
 * A function has to be generated depending on its attributes. If it is a call
 * function, we have to generate different back-end function than for a message
 * passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are
 * specified. Never both may appear.
 *
 * In this method we create functions independant of the class' message buffer
 */
void CBEClass::CreateFunctionsNoClassDependency(CFEOperation *pFEOperation)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called.\n", __func__);

	CFunctionGroup *pGroup = new CFunctionGroup(pFEOperation);
	m_FunctionGroups.Add(pGroup);
	CBEClassFactory *pCF = CBEClassFactory::Instance();

	string exc = string(__func__);

	if (!(pFEOperation->m_Attributes.Find(ATTR_IN)) &&
		!(pFEOperation->m_Attributes.Find(ATTR_OUT)))
	{
		// the call case:
		// we need the functions call, unmarshal, reply-and-wait, skeleton,
		// reply-and-recv
		CBEOperationFunction *pFunction = pCF->GetNewCallFunction();
		m_Functions.Add(pFunction);
		pGroup->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, false);
		// for C++ we need two call wrapper functions in the class
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		{
			// first wrapper
			CBECppCallWrapperFunction *pWrapper = pCF->GetNewCppCallWrapperFunction();
			m_Functions.Add(pWrapper);
			pGroup->m_Functions.Add(pWrapper);
			pWrapper->CreateBackEnd(pFEOperation, false, 1);
			// second wrapper
			pWrapper = pCF->GetNewCppCallWrapperFunction();
			m_Functions.Add(pWrapper);
			pGroup->m_Functions.Add(pWrapper);
			pWrapper->CreateBackEnd(pFEOperation, false, 3);
		}

		// for server side: reply-and-wait, reply-and-recv, skeleton
		pFunction = pCF->GetNewComponentFunction();
		m_Functions.Add(pFunction);
		pGroup->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, true);

		if (pFEOperation->m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
		{
			pFunction = pCF->GetNewReplyFunction();
			m_Functions.Add(pFunction);
			pGroup->m_Functions.Add(pFunction);
			pFunction->CreateBackEnd(pFEOperation, true);
		}
	}
	else
	{
		// the MP case
		// we need the functions send, recv, wait
		bool bComponent = (pFEOperation->m_Attributes.Find(ATTR_OUT));
		// sender: send
		CBEOperationFunction *pFunction = pCF->GetNewSndFunction();
		m_Functions.Add(pFunction);
		pGroup->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, bComponent);

		// receiver: wait, recv, unmarshal
		pFunction = pCF->GetNewWaitFunction();
		m_Functions.Add(pFunction);
		pGroup->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, !bComponent);

		pFunction = pCF->GetNewRcvFunction();
		m_Functions.Add(pFunction);
		pGroup->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, !bComponent);

		// if we send oneway to the server we need a component function
		if (pFEOperation->m_Attributes.Find(ATTR_IN))
		{
			pFunction = pCF->GetNewComponentFunction();
			m_Functions.Add(pFunction);
			pGroup->m_Functions.Add(pFunction);
			pFunction->CreateBackEnd(pFEOperation, true);
		}
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s returns.\n", __func__);
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *
 * A function has to be generated depending on its attributes. If it is a call
 * function, we have to generate different back-end function than for a
 * message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are
 * specified.  Never both may appear.
 *
 * Here we create functions, which depend on the class' message buffer.
 */
void
CBEClass::CreateFunctionsClassDependency(CFEOperation *pFEOperation)
{
	string exc = string(__func__);
	// get function group of pFEOperation (should have been create above)
	vector<CFunctionGroup*>::iterator iterFG;
	for (iterFG = m_FunctionGroups.begin();
		iterFG != m_FunctionGroups.end();
		iterFG++)
	{
		if ((*iterFG)->GetOperation() == pFEOperation)
			break;
	}
	assert(iterFG != m_FunctionGroups.end());

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	if (!(pFEOperation->m_Attributes.Find(ATTR_IN)) &&
		!(pFEOperation->m_Attributes.Find(ATTR_OUT)))
	{
		// the call case:
		// we need the functions unmarshal, marshal, and, if necessary,
		// marshal_exc
		CBEOperationFunction *pFunction = pCF->GetNewUnmarshalFunction();
		m_Functions.Add(pFunction);
		(*iterFG)->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, true);

		pFunction = pCF->GetNewMarshalFunction();
		m_Functions.Add(pFunction);
		(*iterFG)->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, true);

		if (!pFEOperation->m_RaisesDeclarators.empty())
		{
			pFunction = pCF->GetNewMarshalExceptionFunction();
			m_Functions.Add(pFunction);
			(*iterFG)->m_Functions.Add(pFunction);
			pFunction->CreateBackEnd(pFEOperation, true);
		}
	}
	else
	{
		// the MP case
		// we need the functions send, recv, wait, unmarshal
		bool bComponent = (pFEOperation->m_Attributes.Find(ATTR_OUT));

		CBEOperationFunction *pFunction = pCF->GetNewUnmarshalFunction();
		m_Functions.Add(pFunction);
		(*iterFG)->m_Functions.Add(pFunction);
		pFunction->CreateBackEnd(pFEOperation, !bComponent);
	}
}

/** \brief interna function to create an attribute
 *  \param pFEAttribute the respective front-end attribute
 */
void
CBEClass::CreateBackEndAttribute(CFEAttribute *pFEAttribute)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(attr) called\n",
		__func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEAttribute *pAttribute = pCF->GetNewAttribute();
	m_Attributes.Add(pAttribute);
	pAttribute->CreateBackEnd(pFEAttribute);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief internal function to create an exception
 *  \param pFEException the repective front-end exception
 */
void
CBEClass::CreateBackEndException(CFETypedDeclarator* pFEException)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s called\n",
		__func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEException *pException = pCF->GetNewException();
	m_Typedefs.Add(pException);
	pException->CreateBackEnd(pFEException);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n",
		__func__);
}

/** \brief adds the Class to the header file
 *  \param pHeader file the header file to add to
 *  \return true if successful
 *
 * An Class adds its included types, constants and functions.
 */
void CBEClass::AddToHeader(CBEHeaderFile* pHeader)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(header: %s) for class %s called\n", __func__,
		pHeader->GetFileName().c_str(), GetName().c_str());
	// add this class to the file
	if (IsTargetFile(pHeader))
		pHeader->m_Classes.Add(this);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(header: %s) for class %s returns true\n", __func__,
		pHeader->GetFileName().c_str(), GetName().c_str());
}

/** \brief adds this class (or its members) to an implementation file
 *  \param pImpl the implementation file to add this class to
 *  \return true if successful
 *
 * if the options PROGRAM_FILE_FUNCTION is set, we have to add each function
 * seperately for the client implementation file. Otherwise we add the
 * whole class.
 */
void CBEClass::AddToImpl(CBEImplementationFile* pImpl)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(impl: %s) for class %s called\n", __func__,
		pImpl->GetFileName().c_str(), GetName().c_str());
	// check compiler option
	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION) &&
		pImpl->IsOfFileType(FILETYPE_CLIENT))
	{
		for_each(m_Functions.begin(), m_Functions.end(),
			std::bind2nd(std::mem_fun(&CBEFunction::AddToImpl), pImpl));
	}
	// add this class to the file
	if (IsTargetFile(pImpl))
		pImpl->m_Classes.Add(this);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(impl: %s) for class %s returns true\n", __func__,
		pImpl->GetFileName().c_str(), GetName().c_str());
}

/** \brief counts the parameters with a specific type
 *  \param nFEType the front-end type to test for
 *  \param bSameCount true if the count is the same for all functions (false \
 *         if functions have different count)
 *  \param nDirection the direction to count
 *  \return the number of parameters of this type
 */
int
CBEClass::GetParameterCount(int nFEType,
	bool& bSameCount,
	DIRECTION_TYPE nDirection)
{
	if (nDirection == DIRECTION_INOUT)
	{
		// count both and return max
		int nCountIn = GetParameterCount(nFEType, bSameCount, DIRECTION_IN);
		int nCountOut = GetParameterCount(nFEType, bSameCount, DIRECTION_OUT);
		return std::max(nCountIn, nCountOut);
	}

	int nCount = 0, nCurr;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"%s(%d, %s, %d) called: max is %d\n", __func__,
		nFEType, bSameCount ? "true" : "false", nDirection, nCount);
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		nCurr = (*iter)->GetParameterCount(nFEType, nDirection);
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"%s: checking %s: has %d parameter of type %d, max is %d\n", __func__,
			(*iter)->GetName().c_str(), nCurr, nFEType, nCount);
		if ((nCount > 0) && (nCurr != nCount) && (nCurr > 0))
			bSameCount = false;
		nCount = std::max(nCurr, nCount);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"%s: returns %d (same %s)\n", __func__,
		nCount, bSameCount ? "true" : "false");
	return nCount;
}

/** \brief counts the number of string parameter needed for this interface
 *  \param nDirection the direction to count
 *  \param nMustAttrs the attributes which have to be set for the parameters
 *  \param nMustNotAttrs the attributes which must not be set for the parameters
 *  \return the number of strings needed
 */
int
CBEClass::GetStringParameterCount(DIRECTION_TYPE nDirection,
	ATTR_TYPE nMustAttrs,
	ATTR_TYPE nMustNotAttrs)
{
	if (nDirection == DIRECTION_INOUT)
	{
		int nStringsIn = GetStringParameterCount(DIRECTION_IN, nMustAttrs,
			nMustNotAttrs);
		int nStringsOut = GetStringParameterCount(DIRECTION_OUT, nMustAttrs,
			nMustNotAttrs);
		return ((nStringsIn > nStringsOut) ? nStringsIn : nStringsOut);
	}

	int nCount = 0, nCurr;

	vector<CFunctionGroup*>::iterator iterG;
	for (iterG = m_FunctionGroups.begin();
		iterG != m_FunctionGroups.end();
		iterG++)
	{
		vector<CBEFunction*>::iterator iterF;
		for (iterF = (*iterG)->m_Functions.begin();
			iterF != (*iterG)->m_Functions.end();
			iterF++)
		{
			if (!dynamic_cast<CBEOperationFunction*>(*iterF))
				continue;
			nCurr = (*iterF)->GetStringParameterCount(nDirection, nMustAttrs,
				nMustNotAttrs);
			nCount = std::max(nCount, nCurr);
		}
	}

	return nCount;
}

/** \brief calculates the size of the interface function
 *  \param nDirection the direction to count
 *  \return the number of bytes needed to transmit any of the functions
 */
int CBEClass::GetSize(DIRECTION_TYPE nDirection)
{
	if (nDirection == DIRECTION_INOUT)
	{
		int nSizeIn = GetSize(DIRECTION_IN);
		int nSizeOut = GetSize(DIRECTION_OUT);
		return ((nSizeIn > nSizeOut) ? nSizeIn : nSizeOut);
	}

	int nSize = 0, nCurr;

	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if (!dynamic_cast<CBEOperationFunction*>(*iter))
			continue;
		nCurr = (*iter)->GetSize(nDirection);
		nSize = std::max(nSize, nCurr);
	}

	return nSize;
}

/** \brief tries to find the function group for a specific function
 *  \param pFunction the function to search for
 *  \return a reference to the function group or 0
 */
CFunctionGroup* CBEClass::FindFunctionGroup(CBEFunction *pFunction)
{
	// iterate over function groups
	vector<CFunctionGroup*>::iterator iter;
	for (iter = m_FunctionGroups.begin();
		iter != m_FunctionGroups.end();
		iter++)
	{
		// iterate over its functions
		if (find((*iter)->m_Functions.begin(), (*iter)->m_Functions.end(),
				pFunction) != (*iter)->m_Functions.end())
			return *iter;
	}
	return 0;
}

/** \brief find another back-end function for a given func from the same group
 *  \param pFunction the given function
 *  \param nFunctionType the type of the searched function
 *  \return a reference to the found function or0 if not found
 */
CBEFunction* CBEClass::FindFunctionFor(CBEFunction *pFunction, FUNCTION_TYPE nFunctionType)
{
	CFunctionGroup *pFG = FindFunctionGroup(pFunction);
	if (!pFG)
		return 0;
	vector<CBEFunction*>::iterator iter;
	for (iter = pFG->m_Functions.begin(); iter != pFG->m_Functions.end(); iter++)
	{
		if ((*iter)->IsFunctionType(nFunctionType))
			return *iter;
	}
	return 0;
}

/** \brief tries to find a type definition
 *  \param sTypeName the name of the searched type
 *  \param pPrev points to previously found typedef
 *  \return a reference to the type definition
 *
 * We also have to check the message buffer type (if existent).
 */
CBETypedef* CBEClass::FindTypedef(std::string sTypeName, CBETypedef *pPrev)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEClass::%s(%s, %p) in %s called\n",
		__func__, sTypeName.c_str(), pPrev, GetName().c_str());

	vector<CBETypedef*>::iterator iter = m_Typedefs.begin();
	if (pPrev)
	{
		iter = std::find(m_Typedefs.begin(), m_Typedefs.end(), pPrev);
		if (iter != m_Typedefs.end())
			++iter;
		else
			iter = m_Typedefs.begin();
	}
	for (; iter != m_Typedefs.end();
		iter++)
	{
		if ((*iter)->m_Declarators.Find(sTypeName))
			return *iter;
		if ((*iter)->GetType() &&
			(*iter)->GetType()->HasTag(sTypeName))
			return *iter;
	}
	CBETypedef *pMsgBuf = GetMessageBuffer();
	if (pMsgBuf)
	{
		if (pMsgBuf->m_Declarators.Find(sTypeName))
			return pMsgBuf;
		if (pMsgBuf->GetType() &&
			pMsgBuf->GetType()->HasTag(sTypeName))
			return pMsgBuf;
	}
	/* look in base classes */
	CBETypedef *pTypedef;
	vector<CBEClass*>::iterator iterC = m_BaseClasses.begin();
	for (; iterC != m_BaseClasses.end(); iterC++)
	{
		if ((pTypedef = (*iterC)->FindTypedef(sTypeName, pPrev)))
			return pTypedef;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEClass::%s not found in class or base, try namespace\n",
		__func__);
	CBENameSpace *pNameSpace = GetSpecificParent<CBENameSpace>();
	if (pNameSpace)
		return pNameSpace->FindTypedef(sTypeName, pPrev);
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindTypedef(sTypeName, pPrev);
}

/** \brief searches for a constant
 *  \param sConstantName the name of the constant to search
 *  \return a reference to the found constat or 0 if not found
 */
CBEConstant* CBEClass::FindConstant(std::string sConstantName)
{
	CBEConstant *pConstant = m_Constants.Find(sConstantName);
	if (pConstant)
		return pConstant;
	/* look in base classes */
	vector<CBEClass*>::iterator iterC = m_BaseClasses.begin();
	for (; iterC != m_BaseClasses.end(); iterC++)
	{
		if ((pConstant = (*iterC)->FindConstant(sConstantName)))
			return pConstant;
	}

	CBENameSpace *pNameSpace = GetSpecificParent<CBENameSpace>();
	if (pNameSpace)
		return pNameSpace->FindConstant(sConstantName);
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindConstant(sConstantName);
}

/** \brief searches for a type using its tag
 *  \param nType the type of the searched type
 *  \param sTag the tag to search for
 *  \return a reference to the type
 */
CBEType* CBEClass::FindTaggedType(int nType, std::string sTag)
{
	vector<CBEType*>::iterator iter;
	for (iter = m_TypeDeclarations.begin();
		iter != m_TypeDeclarations.end();
		iter++)
	{
		int nFEType = (*iter)->GetFEType();
		if (nType != nFEType)
			continue;
		if (nFEType == TYPE_STRUCT ||
			nFEType == TYPE_UNION ||
			nFEType == TYPE_ENUM)
		{
			if ((*iter)->HasTag(sTag))
				return *iter;
		}
	}

	CBENameSpace *pNameSpace = GetSpecificParent<CBENameSpace>();
	if (pNameSpace)
		return pNameSpace->FindTaggedType(nType, sTag);
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindTaggedType(nType, sTag);
}

/** \brief tries to find a function
 *  \param sFunctionName the name of the function to search for
 *  \param nFunctionType the type of function to find
 *  \return a reference to the searched class or 0
 */
CBEFunction* CBEClass::FindFunction(std::string sFunctionName, FUNCTION_TYPE nFunctionType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(%s, %d) called\n", __func__,
		sFunctionName.c_str(), nFunctionType);

	// simply scan the function for a match
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEClass::%s checking function %s (compare to %s)\n", __func__,
			(*iter)->GetName().c_str(), sFunctionName.c_str());

		if ((*iter)->GetName() == sFunctionName &&
			(*iter)->IsFunctionType(nFunctionType))
			return *iter;
	}

	// checking base classes
	vector<CBEClass*>::iterator iterC;
	CBEFunction *pRet;
	for (iterC = m_BaseClasses.begin(); iterC != m_BaseClasses.end(); iterC++)
	{
		if ((pRet = (*iterC)->FindFunction(sFunctionName, nFunctionType)))
			return pRet;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s function %s not found, return 0\n",
		__func__, sFunctionName.c_str());
	return 0;
}

/** \brief tries to find an enum type with the given enumerator
 *  \param sName the name of the enumerator
 *  \return reference to the enum if found
 */
CBEEnumType* CBEClass::FindEnum(std::string sName)
{
	CBEEnumType *pEnum;
	vector<CBEType*>::iterator iter;
	for (iter = m_TypeDeclarations.begin();
		iter != m_TypeDeclarations.end();
		iter++)
	{
		pEnum = dynamic_cast<CBEEnumType*>(*iter);
		if (pEnum && pEnum->m_Members.Find(sName))
			return pEnum;
	}
	vector<CBETypedef*>::iterator iterT;
	for (iterT = m_Typedefs.begin();
		iterT != m_Typedefs.end();
		iterT++)
	{
		pEnum = dynamic_cast<CBEEnumType*>((*iterT)->GetType());
		if (pEnum && pEnum->m_Members.Find(sName))
			return pEnum;
	}

	CBENameSpace *pNameSpace = GetSpecificParent<CBENameSpace>();
	if (pNameSpace)
		return pNameSpace->FindEnum(sName);
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	return pRoot->FindEnum(sName);
}

/** \brief adds the opcodes of this class' functions to the header file
 *  \param pFile the file to write to
 *  \return true if successfule
 *
 * This implementation adds the base opcode for this class and all opcodes for
 * its functions.
 */
void CBEClass::AddOpcodesToFile(CBEHeaderFile* pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(header: %s) called\n",
		__func__, pFile->GetFileName().c_str());
	// check if the file is really our target file
	if (!IsTargetFile(pFile))
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEClass::%s wrong target file\n", __func__);
		return;
	}

	// first create classes in reverse order, so we can build correct parent
	// relationships
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEConstant *pOpcode = pCF->GetNewConstant();
	CBEOpcodeType *pType = pCF->GetNewOpcodeType();
	pType->SetParent(pOpcode);
	CBEExpression *pBrace = pCF->GetNewExpression();
	pBrace->SetParent(pOpcode);
	CBEExpression *pInterfaceCode = pCF->GetNewExpression();
	pInterfaceCode->SetParent(pBrace);
	CBEExpression *pValue = pCF->GetNewExpression();
	pValue->SetParent(pInterfaceCode);
	CBEExpression *pBits = pCF->GetNewExpression();
	pBits->SetParent(pInterfaceCode);

	// now call Create functions, which require correct parent relationshsips

	// create opcode type
	pType->CreateBackEnd();
	// get opcode number
	int nInterfaceNumber = GetClassNumber();
	// create value
	pValue->CreateBackEnd(nInterfaceNumber);
	// create shift bits
	string sShift = pNF->GetInterfaceNumberShiftConstant();
	pBits->CreateBackEnd(sShift);
	// create value << bits
	pInterfaceCode->CreateBackEndBinary(pValue, EXPR_LSHIFT, pBits);
	// brace it
	pBrace->CreateBackEndPrimary(EXPR_PAREN, pInterfaceCode);
	// create constant
	// create opcode name
	string sName = pNF->GetOpcodeConst(this);
	// add const to file
	pFile->m_Constants.Add(pOpcode);
	pOpcode->CreateBackEnd(pType, sName, pBrace, true/* always define*/);

	// iterate over functions
	vector<CFunctionGroup*>::iterator iter;
	for (iter = m_FunctionGroups.begin();
		iter != m_FunctionGroups.end();
		iter++)
	{
		AddOpcodesToFile((*iter)->GetOperation(), pFile);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief adds the opcode for a single function
 *  \param pFEOperation the function to add the opcode for
 *  \param nNumber the opcode number to add
 *  \param sName the name of the opcode
 *  \param pFile the file to add the opcode to
 */
void CBEClass::AddOpcodesToFile(CFEOperation *pFEOperation, int nNumber, std::string sName,
	CBEHeaderFile* pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::AddOpcodesToFile(operation: %s) called\n",
		pFEOperation->GetName().c_str());

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBENameFactory *pNF = CBENameFactory::Instance();
	// first create classes, so we can build parent relationship correctly
	CBEConstant *pOpcode = pCF->GetNewConstant();
	CBEOpcodeType *pType = pCF->GetNewOpcodeType();
	pType->SetParent(pOpcode);
	CBEExpression *pTopBrace = pCF->GetNewExpression();
	pTopBrace->SetParent(pOpcode);
	CBEExpression *pValue = pCF->GetNewExpression();
	pValue->SetParent(pTopBrace);
	CBEExpression *pBrace = pCF->GetNewExpression();
	pBrace->SetParent(pValue);
	CBEExpression *pFuncCode = pCF->GetNewExpression();
	pFuncCode->SetParent(pBrace);
	CBEExpression *pBase = pCF->GetNewExpression();
	pBase->SetParent(pValue);
	CBEExpression *pNumber = pCF->GetNewExpression();
	pNumber->SetParent(pFuncCode);
	CBEExpression *pBitMask = pCF->GetNewExpression();
	pBitMask->SetParent(pFuncCode);

	// now call the create functions, which require an correct parent
	// relationship
	// get base opcode name
	string sBase = pNF->GetOpcodeConst(this);
	pBase->CreateBackEnd(sBase);
	// create number
	pNumber->CreateBackEnd(nNumber);
	// if the function has an absolute uuid, then use it directly withoud
	// bitmask and base
	CFEIntAttribute *pFEAttribute = dynamic_cast<CFEIntAttribute*>(
		pFEOperation->m_Attributes.Find(ATTR_UUID));
	CFERangeAttribute *pFERangeAttribute = dynamic_cast<CFERangeAttribute*>(
		pFEOperation->m_Attributes.Find(ATTR_UUID_RANGE));
	if (HasUuid(pFEOperation) && (
			(pFEAttribute && pFEAttribute->IsAbsolute()) ||
			(pFERangeAttribute && pFERangeAttribute->IsAbsolute()) ) )
	{
		pTopBrace->CreateBackEndPrimary(EXPR_PAREN, pNumber);
		pNumber->SetParent(pTopBrace);
		// cleanup
		delete pBitMask;
		delete pFuncCode;
		delete pBrace;
		delete pValue;
	}
	else
	{
		// create bitmask
		string sBitMask =  pNF->GetFunctionBitMaskConstant();
		pBitMask->CreateBackEnd(sBitMask);
		// create function code
		pFuncCode->CreateBackEndBinary(pNumber, EXPR_BITAND, pBitMask);
		// create braces
		pBrace->CreateBackEndPrimary(EXPR_PAREN, pFuncCode);
		// create value
		pValue->CreateBackEndBinary(pBase, EXPR_PLUS, pBrace);
		// create top brace
		pTopBrace->CreateBackEndPrimary(EXPR_PAREN, pValue);
	}
	// create opcode type
	pType->CreateBackEnd();
	// create constant
	pFile->m_Constants.Add(pOpcode);
	pOpcode->CreateBackEnd(pType, sName, pTopBrace, true/*always defined*/);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}


/** \brief adds the opcode for a single function
 *  \param pFEOperation the function to add the opcode for
 *  \param pFile the file to add the opcode to
 */
void CBEClass::AddOpcodesToFile(CFEOperation *pFEOperation, CBEHeaderFile* pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::AddOpcodesToFile(operation: %s) called\n",
		pFEOperation->GetName().c_str());

	int nOperationNb = GetOperationNumber(pFEOperation);
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetOpcodeConst(pFEOperation);
	AddOpcodesToFile(pFEOperation, nOperationNb, sName, pFile);

	// if the operation has the uuid-range attribute, we have to add another
	// constants for the upper bound
	CFERangeAttribute *pFERangeAttribute = dynamic_cast<CFERangeAttribute*>(
		pFEOperation->m_Attributes.Find(ATTR_UUID_RANGE));
	if (pFERangeAttribute)
	{
		string sName = pNF->GetOpcodeConst(pFEOperation, true);
		AddOpcodesToFile(pFEOperation, pFERangeAttribute->GetUpperValue(), sName, pFile);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief calculates the number used as base opcode number
 *  \return the interface number
 *
 * AN interface number is first of all it's uuid. If it is not available
 * the base interfaces are counted and this interface's number is the
 * highest number of it's base interfaces + 1.
 */
int CBEClass::GetClassNumber()
{
	CBEAttribute *pUuidAttr = m_Attributes.Find(ATTR_UUID);
	if (pUuidAttr)
	{
		if (pUuidAttr->IsOfType(ATTR_CLASS_INT))
		{
			return pUuidAttr->GetIntValue();
		}
	}

	int nNumber = 1;
	vector<CBEClass*>::iterator iter;
	for (iter = m_BaseClasses.begin();
		iter != m_BaseClasses.end();
		iter++)
	{
		int nBaseNumber = (*iter)->GetClassNumber();
		if (nBaseNumber >= nNumber)
			nNumber = nBaseNumber+1;
	}
	return nNumber;
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *
 * With the C implementation the class simply calls the Write methods
 * of its constants, typedefs and functions. The message buffer is first
 * defined in a forward declaration and later really defined. This way it is
 * known when checking the functions, but can use types declared inside the
 * interface.
 *
 * However, this does not work for inlined functions. They need to fully know
 * the message buffer type when using it.
 */
void CBEClass::Write(CBEHeaderFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s called\n", __func__);

	// since message buffer is local for this class, the class declaration
	// wraps the message buffer
	// per default we derive from CORBA_Object
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
		IsTargetFile(&pFile))
	{
		// CPP TODO: should derive from CORBA_Object, _but_:
		// then we need to have a declaration of CORBA_Object which is not a
		// typedef of a pointer to CORBA_Object_base, which in turn messes up
		// the compilation of C++ files including generated header files for C
		// backend... We have to find some other way, such as a dice local
		// define for C++.
		pFile << "\tclass ";
		WriteClassName(pFile);
		WriteBaseClasses(pFile);
		pFile << "\n";
		pFile << "\t{\n";

		WriteMemberVariables(pFile);
		WriteConstructor(pFile);
		WriteDestructor(pFile);
	}

	// write message buffer type seperately
	CBEMsgBuffer* pMsgBuf = GetMessageBuffer();
	CBETarget *pTarget = pFile.GetSpecificParent<CBETarget>();
	// test for client and if type is needed
	bool bWriteMsgBuffer = (pMsgBuf != 0) &&
		pTarget->HasFunctionWithUserType(
			pMsgBuf->m_Declarators.First()->GetName());

	// write all constants, typedefs
	WriteElements(pFile);
	// now write message buffer (it might use types defined above)
	if (bWriteMsgBuffer)
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
		{
			pMsgBuf->WriteDeclaration(pFile);
			pFile << "\n";
		}
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
			IsTargetFile(&pFile))
		{
			pFile << "\tprotected:\n";
			++pFile;
			pMsgBuf->WriteDeclaration(pFile);
			--pFile;
		}
	}
	// now write functions (which need definition of msg buffer)
	WriteFunctions(pFile);
	WriteHelperFunctions(pFile);

	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
		IsTargetFile(&pFile))
		pFile << "\t};\n";

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief writes the class name, which might be different for clnt & srv
 *  \param pFile the file to write to
 */
void CBEClass::WriteClassName(CBEFile& pFile)
{
	pFile << GetName();
	if (pFile.IsOfFileType(FILETYPE_COMPONENT))
		pFile << "Server";
}

/** \brief writes the ist of base classes
 *  \param pFile the file to write to
 */
void CBEClass::WriteBaseClasses(CBEFile& pFile)
{
	bool bComma = false;
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_BaseClasses.begin();
		iterC != m_BaseClasses.end();
		iterC++)
	{
		if (bComma)
			pFile << ", ";
		else
			pFile << " : public ";
		pFile << (*iterC)->GetName();
		if (pFile.IsOfFileType(FILETYPE_COMPONENT))
			pFile << "Server";
		bComma = true;
	}
}

/** \brief write member variable for C++ classes
 *  \param pFile the file to write to
 *
 * Do not write member variables if deriving from a base interface, becuase
 * these variables have been defined there already.
 */
void CBEClass::WriteMemberVariables(CBEHeaderFile& pFile)
{
	if (!m_BaseClasses.empty())
		return;

	pFile << "\t/* members */\n";
	pFile << "\tprotected:\n";
	if (pFile.IsOfFileType(FILETYPE_CLIENTHEADER))
	{
		++pFile << "\t/* contains the address of the server */\n";
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sObj = pNF->GetServerVariable();
		pFile << "\tCORBA_Object_base " << sObj << ";\n";
		pFile << "\n";
		--pFile;
	}
	if (pFile.IsOfFileType(FILETYPE_COMPONENTHEADER))
	{
		assert(m_pCorbaObject);
		++pFile << "\t";
		m_pCorbaObject->WriteDeclaration(pFile);
		pFile << ";\n";

		assert(m_pCorbaEnv);
		pFile << "\t";
		m_pCorbaEnv->WriteDeclaration(pFile);
		pFile << ";\n";
		--pFile;
	}
}

/** \brief write constructor for C++ class
 *  \param pFile the file to write to
 *
 * At the client side do take the ID of the server as an argument. At the
 * server side to not write arguments.
 *
 * For both sides define a private copy constructor.
 */
void CBEClass::WriteConstructor(CBEHeaderFile& pFile)
{

	pFile << "\tpublic:\n";
	++pFile << "\t/* construct the client object */\n";
	pFile << "\t";
	WriteClassName(pFile);
	if (pFile.IsOfFileType(FILETYPE_CLIENT))
	{
		pFile << " (CORBA_Object_base _server)\n";
		pFile << "\t : ";
		// if we do not have a base class, we initialize our server member
		if (m_BaseClasses.empty())
		{
			CBENameFactory *pNF = CBENameFactory::Instance();
			string sObj = pNF->GetServerVariable();
			pFile << sObj << "(_server)";
		}
		else
		{
			// initialize the base classes
			bool bComma = false;
			vector<CBEClass*>::iterator iterC;
			for (iterC = m_BaseClasses.begin();
				iterC != m_BaseClasses.end();
				iterC++)
			{
				if (bComma)
					pFile << ",\n" << "\t   ";
				pFile << (*iterC)->GetName() << "(_server)";
				bComma = true;
			}
		}
	}
	else /* COMPONENT */
	{
		pFile << " (void)";

		if (m_BaseClasses.empty())
		{
			pFile << "\n";

			assert(m_pCorbaObject);
			string sObj = m_pCorbaObject->m_Declarators.First()->GetName();
			pFile << "\t : " << sObj << "(),\n";

			assert(m_pCorbaEnv);
			string sEnv = m_pCorbaEnv->m_Declarators.First()->GetName();
			pFile << "\t   " << sEnv << "()";
		}
		else
		{
			// initialize the base classes
			bool bComma = false;
			vector<CBEClass*>::iterator iterC;
			for (iterC = m_BaseClasses.begin();
				iterC != m_BaseClasses.end();
				iterC++)
			{
				if (bComma)
					pFile << ",\n" << "\t   ";
				else
					pFile << "\n\t : ";
				pFile << (*iterC)->GetName() << "Server()";
				bComma = true;
			}
		}
	}
	pFile << "\n";
	pFile << "\t{ }\n";
	pFile << "\n";
	--pFile;

	// if this class is derived from other interfaces, then there is no need
	// to define a private copy constructor, because the base class already
	// has one.
	if (m_BaseClasses.empty())
	{
		pFile << "\tprivate:\n";
		++pFile << "\t/* copy constructor to avoid implicit assignment of object */\n";
		pFile << "\t";
		WriteClassName(pFile);
		pFile << " (const ";
		WriteClassName(pFile);
		pFile << "&";
		pFile << ")\n";
		pFile << "\t{ }\n";
		pFile << "\n";
		--pFile;
	}
}

/** \brief write the destructore for C++ class
 *  \param pFile the file to write to
 */
void CBEClass::WriteDestructor(CBEHeaderFile& pFile)
{
	pFile << "\tpublic:\n";
	++pFile << "\t/* destruct client object */\n";
	pFile << "\tvirtual ~";
	WriteClassName(pFile);
	pFile << " ()\n";
	pFile << "\t{ }\n";
	pFile << "\n";
	--pFile;
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *
 * With the C implementation the class simply calls the Write methods
 * of its constants, typedefs and functions
 */
void CBEClass::Write(CBEImplementationFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s called\n", __func__);
	// write implementation for functions
	WriteElements(pFile);
	// write helper functions if any
	WriteHelperFunctions(pFile);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *
 * With the C implementation the class simply calls the Write methods
 * of its constants and typedefs. The functions are written separetly, because
 * they might be inline and need a full definition of the message buffer which
 * in turn needs all types to be declared.
 */
void CBEClass::WriteElements(CBEHeaderFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s for %s called\n", __func__, GetName().c_str());

	// sort our members/elements depending on source line number
	// into extra vector
	CreateOrderedElementList();

	// members are public
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
		IsTargetFile(&pFile))
	{
		pFile << "\tpublic:\n";
		++pFile;
	}

	// write target file
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if (dynamic_cast<CBEFunction*>(*iter))
			continue;

		WriteLineDirective(pFile, *iter);
		if (dynamic_cast<CBEConstant*>(*iter))
			WriteConstant((CBEConstant*)(*iter), pFile);
		else if (dynamic_cast<CBETypedef*>(*iter))
			WriteTypedef((CBETypedef*)(*iter), pFile);
		else if (dynamic_cast<CBEType*>(*iter))
			WriteTaggedType((CBEType*)(*iter), pFile);
	}

	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
		IsTargetFile(&pFile))
		--pFile;

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::Write(head, %s) finished\n", GetName().c_str());
}

/** \brief writes the function declarations for the header file
 *  \param pFile the header file to write to
 */
void
CBEClass::WriteFunctions(CBEHeaderFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(head, %s) called\n", __func__, GetName().c_str());

	int nFuncCount = GetFunctionWriteCount(pFile);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s %d functions\n",
		__func__, nFuncCount);

	if (nFuncCount > 0)
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			++pFile;
		WriteExternCStart(pFile);
	}
	// write target functions in ordered appearance
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if (dynamic_cast<CBEFunction*>(*iter))
		{
			WriteLineDirective(pFile, *iter);
			WriteFunction((CBEFunction*)(*iter), pFile);
		}
	}
	// write helper functions if any
	if (nFuncCount > 0)
	{
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
			--pFile;
		WriteExternCEnd(pFile);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(head, %s) finished\n",
		__func__, GetName().c_str());
}

/** \brief writes the function declarations for the implementation file
 *  \param pFile the implementation file to write to
 */
void
CBEClass::WriteFunctions(CBEImplementationFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::%s(impl, %s) called\n", __func__, GetName().c_str());

	int nFuncCount = GetFunctionWriteCount(pFile);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s %d functions\n",
		__func__, nFuncCount);

	if (nFuncCount > 0)
	{
		WriteExternCStart(pFile);
	}
	// write target functions in ordered appearance
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if (dynamic_cast<CBEFunction*>(*iter))
		{
			WriteLineDirective(pFile, *iter);
			WriteFunction((CBEFunction*)(*iter), pFile);
		}
	}
	// write helper functions if any
	if (nFuncCount > 0)
	{
		WriteExternCEnd(pFile);
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(impl, %s) finished\n",
		__func__, GetName().c_str());
}

/** \brief writes the class to the target file
 *  \param pFile the file to write to
 *
 * With the C implementation the class simply calls it's
 * function's Write method.
 */
void CBEClass::WriteElements(CBEImplementationFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::Write(impl, %s) called\n", GetName().c_str());

	// sort our members/elements depending on source line number
	// into extra vector
	CreateOrderedElementList();
	WriteFunctions(pFile);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEClass::Write(impl, %s) finished\n", GetName().c_str());
}

/** \brief writes the line directive
 *  \param pFile the file to write to
 *  \param pObj the object for which to write the line directive
 */
void
CBEClass::WriteLineDirective(CBEFile& pFile,
	CObject *pObj)
{
	if (!CCompiler::IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE))
		return;
	pFile << "# " << pObj->m_sourceLoc.getBeginLine() << " \"" <<
		pObj->m_sourceLoc.getFilename() << "\"\n";
}

/** \brief writes a constant
 *  \param pConstant the constant to write
 *  \param pFile the file to write to
 */
void CBEClass::WriteConstant(CBEConstant *pConstant,
	CBEHeaderFile& pFile)
{
	assert(pConstant);
	if (!pConstant->IsTargetFile(&pFile))
		return;

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s called\n", __func__);

	pConstant->Write(pFile);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief write a type definition
 *  \param pTypedef the type definition to write
 *  \param pFile the file to write to
 */
void CBEClass::WriteTypedef(CBETypedef *pTypedef,
	CBEHeaderFile& pFile)
{
	assert(pTypedef);
	if (!pTypedef->IsTargetFile(&pFile))
		return;

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s called\n", __func__);

	pTypedef->WriteDeclaration(pFile);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief write a function to the header file
 *  \param pFunction the function to write
 *  \param pFile the file to write to
 */
void CBEClass::WriteFunction(CBEFunction *pFunction,
	CBEHeaderFile& pFile)
{
	assert(pFunction);

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(%s, %s) called\n",
		__func__, pFunction->GetName().c_str(), pFile.GetFileName().c_str());

	if (pFunction->DoWriteFunction(&pFile))
	{
		pFunction->Write(pFile);
		pFile << "\n";
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief write a function to the implementation file
 *  \param pFunction the function to write
 *  \param pFile the file to write to
 */
void CBEClass::WriteFunction(CBEFunction *pFunction,
	CBEImplementationFile& pFile)
{
	assert(pFunction);

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(%s, %s) called\n",
		__func__, pFunction->GetName().c_str(), pFile.GetFileName().c_str());

	if (pFunction->DoWriteFunction(&pFile))
	{
		pFunction->Write(pFile);
		pFile << "\n";
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s finished\n", __func__);
}

/** \brief calculates the function identifier
 *  \param pFEOperation the front-end operation to get the identifier for
 *  \return the function identifier
 *
 * The function identifier is unique within an interface. Usually an
 * interface's scope is determined by its definition. But since the server
 * loop using the function identifier only differentiates number, we have to
 * be unique within the same interface number.
 *
 * It is sufficient to check base interfaces for their interface ID, because
 * derived interfaces will use this algorithm as well and thus regard our
 * generated function ID's.
 *
 * For all interfaces with the same ID as the operation's interface, we build
 * a grid of predefined function ID's. In the next step we iterate over the
 * functions and count them, skipping the predefined numbers. When we finally
 * reached the given function, we return the calculated function ID.
 *
 * But first of all, the Uuid attribute of a function determines it's ID. To
 * print warnings about same Uuids anyways, we test for this situation after
 * the FindPredefinedNumbers call.
 */
int CBEClass::GetOperationNumber(CFEOperation *pFEOperation)
{
	assert(pFEOperation);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::GetOperationNumber for %s\n",
		pFEOperation->GetName().c_str());
	CFEInterface *pFEInterface = pFEOperation->GetSpecificParent<CFEInterface>();
	int nInterfaceNumber = GetInterfaceNumber(pFEInterface);
	// search for base interfaces with same interface number and collect them
	vector<CFEInterface*> vSameInterfaces;
	FindInterfaceWithNumber(pFEInterface, nInterfaceNumber, &vSameInterfaces);
	// now search for predefined numbers
	map<unsigned int, string> functionIDs;
	vSameInterfaces.push_back(pFEInterface); // add this interface too
	FindPredefinedNumbers(&vSameInterfaces, &functionIDs);
	// find pFEInterface in vector
	vector<CFEInterface*>::iterator iterI = std::find(vSameInterfaces.begin(),
		vSameInterfaces.end(), pFEInterface);
	if (iterI != vSameInterfaces.end())
		vSameInterfaces.erase(iterI);
	// check for Uuid attribute and return its value
	if (HasUuid(pFEOperation))
		return GetUuid(pFEOperation);
	// now there is no Uuid, thus we have to calculate the function id
	// get the maximum function ID of the interfaces with the same number
	int nFunctionID = 0;
	iterI = vSameInterfaces.begin();
	while (iterI != vSameInterfaces.end())
	{
		CFEInterface *pCurrI = *iterI++;
		if (!pCurrI)
			continue;
		// we sum the max opcodes, because they have to be disjunct
		// if we would use max(current, functionID) the function ID would be
		// the same ...
		//
		// interface 1: 1, 2, 3, 4    \_ if max
		// interface 2: 1, 2, 3, 4, 5 /
		//
		// interface 1: 1, 2, 3, 4    \_ if sum
		// interface 2: 5, 6, 7, 8, 9 /
		//
		nFunctionID += GetMaxOpcodeNumber(pCurrI);
	}
	// now iterate over functions, incrementing a counter, check if new value
	// is predefined and if the checked function is this function, then return
	// the counter value
	vector<CFEOperation*>::iterator iterO;
	for (iterO = pFEInterface->m_Operations.begin();
		iterO != pFEInterface->m_Operations.end();
		iterO++)
	{
		nFunctionID++;
		while (IsPredefinedID(&functionIDs, nFunctionID)) nFunctionID++;
		if ((*iterO) == pFEOperation)
			return nFunctionID;
	}
	// something went wrong -> if we are here, the operations parent interface
	// does not have this operation as a child
	assert(false);
	return 0;
}
/** \brief checks if a function ID is predefined
 *  \param pFunctionIDs the predefined IDs
 *  \param nNumber the number to test
 *  \return true if number is predefined
 */
bool CBEClass::IsPredefinedID(map<unsigned int, std::string> *pFunctionIDs, int nNumber)
{
	return pFunctionIDs->find(nNumber) != pFunctionIDs->end();
}

/** \brief calculates the maximum function ID
 *  \param pFEInterface the interface to test
 *  \return the maximum function ID for this interface
 *
 * Theoretically the max opcode count is the number of its operations. This
 * can be calculated calling CFEInterface::GetOperationCount(). This estimate
 * can be wrong if the interface contains functions with uuid's, which are
 * bigger than the operation count.
 */
int CBEClass::GetMaxOpcodeNumber(CFEInterface *pFEInterface)
{
	int nMax = pFEInterface->GetOperationCount(false);
	// now check operations
	vector<CFEOperation*>::iterator iter;
	for (iter = pFEInterface->m_Operations.begin();
		iter != pFEInterface->m_Operations.end();
		iter++)
	{
		// if this function has uuid, we check if uuid is bigger than count
		if (!HasUuid(*iter))
			continue;
		// now check
		int nUuid = GetUuid(*iter);
		if (nUuid > nMax)
			nMax = nUuid;
	}
	return nMax;
}

/** \brief returns the functions Uuid
 *  \param pFEOperation the operation to get the UUID from
 *  \return the Uuid or -1 if none exists
 */
int CBEClass::GetUuid(CFEOperation *pFEOperation)
{
	CFEAttribute *pUuidAttr = pFEOperation->m_Attributes.Find(ATTR_UUID);
	if (pUuidAttr && dynamic_cast<CFEIntAttribute*>(pUuidAttr))
		return static_cast<CFEIntAttribute*>(pUuidAttr)->GetIntValue();
	// also check for uuid-range
	pUuidAttr = pFEOperation->m_Attributes.Find(ATTR_UUID_RANGE);
	if (pUuidAttr && dynamic_cast<CFERangeAttribute*>(pUuidAttr))
		return static_cast<CFERangeAttribute*>(pUuidAttr)->GetLowerValue();
	return -1;
}

/** \brief checks if this functio has a Uuid
 *  \param pFEOperation the operation to check
 *  \return true if this operation has a Uuid
 */
bool CBEClass::HasUuid(CFEOperation *pFEOperation)
{
	CFEAttribute *pUuidAttr = pFEOperation->m_Attributes.Find(ATTR_UUID);
	if (pUuidAttr && dynamic_cast<CFEIntAttribute*>(pUuidAttr))
		return true;
	// also check for uuid-range
	pUuidAttr = pFEOperation->m_Attributes.Find(ATTR_UUID_RANGE);
	if (pUuidAttr && dynamic_cast<CFERangeAttribute*>(pUuidAttr))
		return true;
	return false;
}

/** \brief calculates the number used as base opcode number
 *  \param pFEInterface the interface to check.
 *  \return the interface number
 *
 * AN interface number is first of all it's uuid. If it is not available
 * the base interfaces are counted and this interface's number is the
 * highest number of it's base interfaces + 1.
 */
int CBEClass::GetInterfaceNumber(CFEInterface *pFEInterface)
{
	assert(pFEInterface);

	CFEAttribute *pUuidAttr = pFEInterface->m_Attributes.Find(ATTR_UUID);
	if (pUuidAttr)
	{
		if (dynamic_cast<CFEIntAttribute*>(pUuidAttr))
		{
			return ((CFEIntAttribute*)pUuidAttr)->GetIntValue();
		}
	}

	int nNumber = 1;
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEInterface->m_BaseInterfaces.begin();
		iterI != pFEInterface->m_BaseInterfaces.end();
		iterI++)
	{
		int nBaseNumber = GetInterfaceNumber(*iterI);
		if (nBaseNumber >= nNumber)
			nNumber = nBaseNumber+1;
	}
	return nNumber;
}

/** \brief searches all interfaces with the same interface number
 *  \param pFEInterface its base interfaces are searched
 *  \param nNumber the number to compare with
 *  \param pCollection the vector to add the same interfaces to
 *
 * We only search the base interfaces, because if we would search derived
 * interfaces as well, we might produce different client bindings if a base
 * interface is suddenly derived from. Instead, we generate errors in the
 * derived interface, that function id's might overlap.
 */
int
CBEClass::FindInterfaceWithNumber(CFEInterface *pFEInterface,
	int nNumber,
	vector<CFEInterface*> *pCollection)
{
	assert(pCollection);
	int nCount = 0;
	// search base interfaces
	vector<CFEInterface*>::iterator iter;
	for (iter = pFEInterface->m_BaseInterfaces.begin();
		iter != pFEInterface->m_BaseInterfaces.end();
		iter++)
	{
		if (GetInterfaceNumber(*iter) == nNumber)
		{
			// check if we already got this interface (use pointer)
			vector<CFEInterface*>::const_iterator iterI = find(pCollection->begin(), pCollection->end(),
				*iter);
			if (iterI == pCollection->end()) // no match found
				pCollection->push_back(*iter);
			nCount++;
		}
		nCount += FindInterfaceWithNumber(*iter, nNumber, pCollection);
	}
	return nCount;
}

/** \brief find predefined function IDs
 *  \param pCollection the vector containing the interfaces to test
 *  \param pNumbers the array containing the numbers
 *  \return number of predefined function IDs
 *
 * To find predefined function id'swe have to iterate over the interface's
 * function, check if they have a UUID attribute and get it's number. If it
 * has a number we extend the pNumber array and add this number at the correct
 * position (the array is ordered). If the number exists already, we print a
 * warning containing both function's names and the function ID.
 */
int CBEClass::FindPredefinedNumbers(vector<CFEInterface*> *pCollection,
	map<unsigned int, std::string> *pNumbers)
{
	assert (pCollection);
	assert (pNumbers);
	int nCount = 0;
	// iterate over interfaces with same interface number
	vector<CFEInterface*>::iterator iterI = pCollection->begin();
	CFEInterface *pFEInterface;
	while (iterI != pCollection->end())
	{
		pFEInterface = *iterI++;
		if (!pFEInterface)
			continue;
		// iterate over current interface's operations
		vector<CFEOperation*>::iterator iterO;
		for (iterO = pFEInterface->m_Operations.begin();
			iterO != pFEInterface->m_Operations.end();
			iterO++)
		{
			// check if operation has Uuid attribute
			if (HasUuid(*iterO))
			{
				int nOpNumber = GetUuid(*iterO);
				// check if this number is already defined somewhere
				if (pNumbers->find(nOpNumber) != pNumbers->end())
				{
					if (CCompiler::IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
					{
						CMessages::GccWarning(*iterO,
							"Function \"%s\" has same Uuid (%d) as function \"%s\"\n",
							(*iterO)->GetName().c_str(), nOpNumber,
							(*pNumbers)[nOpNumber].c_str());
						break;
					}
					else
					{
						CMessages::GccError(*iterO,
							"Function \"%s\" has same Uuid (%d) as function \"%s\"\n",
							(*iterO)->GetName().c_str(), nOpNumber,
							(*pNumbers)[nOpNumber].c_str());
						exit(1);
					}
				}
				// no element with this number is present

				// check if this number might collide with base interface numbers
				CheckOpcodeCollision(pFEInterface, nOpNumber, pCollection,
					*iterO);
				// add uuid attribute to list
				pNumbers->insert(std::make_pair(nOpNumber,
						(*iterO)->GetName()));
				// incremenet count
				nCount++;
			}
		}
	}
	return nCount;
}

/** \brief will check the opcode collisions of the base interfaces
 *  \param pFEInterface the interface to check for
 *
 * iterates the base interfaces and compares any two interfaces
 */
void
CBEClass::CheckOpcodeCollision(CFEInterface *pFEInterface)
{
	vector<CFEInterface*>::iterator i1;
	for (i1 = pFEInterface->m_BaseInterfaces.begin();
		i1 != pFEInterface->m_BaseInterfaces.end();
		i1++)
	{
		// only check successors
		vector<CFEInterface*>::iterator i2;
		for (i2 = i1 + 1;
			i2 != pFEInterface->m_BaseInterfaces.end();
			i2++)
		{
			CheckOpcodeCollision(*i1, *i2);
		}
	}
}

/** \brief check if two interfaces share functions with the same IDs
 *  \param pFirst first interface
 *  \param pSecond second interface
 */
void
CBEClass::CheckOpcodeCollision(CFEInterface *pFirst,
	CFEInterface *pSecond)
{
	// check base interfaces of first interface (will eventually also include
	// second interfaces)
	CheckOpcodeCollision(pFirst);
	// check if same interface ID
	if (GetInterfaceNumber(pFirst) != GetInterfaceNumber(pSecond))
		return;
	// iterate operations of first interface
	vector<CFEOperation*>::iterator iOp1;
	for (iOp1 = pFirst->m_Operations.begin();
		iOp1 != pFirst->m_Operations.end();
		iOp1++)
	{
		// and compare to all operations of second interface
		vector<CFEOperation*>::iterator iOp2;
		for (iOp2 = pSecond->m_Operations.begin();
			iOp2 != pSecond->m_Operations.end();
			iOp2++)
		{
			int nOpNumber;
			if ((nOpNumber = GetOperationNumber(*iOp1)) ==
				GetOperationNumber(*iOp2))
			{
				if (CCompiler::IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
				{
					CMessages::GccWarning(*iOp2,
						"Function \"%s\" in interface \"%s\" has same Uuid (%d) "
						"as function \"%s\" in interface \"%s\"\n",
						(*iOp2)->GetName().c_str(), pSecond->GetName().c_str(),
						nOpNumber,
						(*iOp1)->GetName().c_str(), pFirst->GetName().c_str());
					break;
				}
				else
				{
					CMessages::GccError(*iOp2,
						"Function \"%s\" in interface \"%s\" has same Uuid (%d) "
						"as function \"%s\" in interface \"%s\"\n",
						(*iOp2)->GetName().c_str(), pSecond->GetName().c_str(),
						nOpNumber,
						(*iOp1)->GetName().c_str(), pFirst->GetName().c_str());
					exit(1);
				}
			}
		}
	}
}

/** \brief checks if the opcode could be used by base interfaces
 *  \param pFEInterface the currently investigated interface
 *  \param nOpNumber the opcode to check for
 *  \param pCollection the collection of predefined function ids
 *  \param pFEOperation a reference to the currently tested function
 */
int
CBEClass::CheckOpcodeCollision(CFEInterface *pFEInterface,
	int nOpNumber,
	vector<CFEInterface*> *pCollection,
	CFEOperation *pFEOperation)
{
	int nBaseNumber = 0;
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEInterface->m_BaseInterfaces.begin();
		iterI != pFEInterface->m_BaseInterfaces.end();
		iterI++)
	{
		// test current base interface only if numbers are identical
		if (GetInterfaceNumber(pFEInterface) !=
			GetInterfaceNumber(*iterI))
			continue;
		// first check the base interface's base interfaces
		nBaseNumber += CheckOpcodeCollision(*iterI, nOpNumber, pCollection,
			pFEOperation);
		// now check interface's range
		nBaseNumber += GetMaxOpcodeNumber(*iterI);
		if ((nOpNumber > 0) && (nOpNumber <= nBaseNumber))
		{
			if (CCompiler::IsWarningSet(PROGRAM_WARNING_IGNORE_DUPLICATE_FID))
			{
				CMessages::GccWarning(pFEOperation,
					"Function \"%s\" has Uuid (%d) which is used by compiler for base interface \"%s\"\n",
					pFEOperation->GetName().c_str(), nOpNumber,
					(*iterI)->GetName().c_str());
				break;
			}
			else
			{
				CMessages::GccError(pFEOperation,
					"Function \"%s\" has Uuid (%d) which is used by compiler for interface \"%s\"\n",
					pFEOperation->GetName().c_str(), nOpNumber,
					(*iterI)->GetName().c_str());
				exit(1);
			}
		}
	}
	// return checked range
	return nBaseNumber;
}

/** \brief test if this class belongs to the file
 *  \param pFile the file to test
 *  \return true if the given file is a target file for the class
 *
 * A file is a target file for the class if its the target file for at least
 * one function.
 */
bool CBEClass::IsTargetFile(CBEFile* pFile)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::IsTargetFile(%s) called\n",
		pFile->GetFileName().c_str());

	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if ((*iter)->IsTargetFile(pFile))
			return true;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClass::IsTargetFile returns false\n");
	return false;
}

/** \brief tries to create a new back-end representation of a tagged type declaration
 *  \param pFEType the respective front-end type
 *  \return true if successful
 */
void
CBEClass::CreateBackEndTaggedDecl(CFEConstructedType *pFEType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(constr type) called\n",
		__func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEType *pType = pCF->GetNewType(pFEType->GetType());
	m_TypeDeclarations.Add(pType);
	pType->SetParent(this);
	pType->CreateBackEnd(pFEType);
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEClass::%s(constr type) returns\n",
		__func__);
}

/** \brief writes a tagged type declaration
 *  \param pType the type to write
 *  \param pFile the file to write to
 */
void CBEClass::WriteTaggedType(CBEType *pType,
	CBEHeaderFile& pFile)
{
	assert(pType);
	if (!pType->IsTargetFile(&pFile))
		return;
	// get tag
	string sTag;
	if (dynamic_cast<CBEStructType*>(pType))
		sTag = ((CBEStructType*)pType)->GetTag();
	if (dynamic_cast<CBEUnionType*>(pType))
		sTag = ((CBEUnionType*)pType)->GetTag();
	CBENameFactory *pNF = CBENameFactory::Instance();
	sTag = pNF->GetTypeDefine(sTag);
	pFile << "#ifndef " << sTag << "\n";
	pFile << "#define " << sTag << "\n";
	pType->Write(pFile);
	pFile << ";\n";
	pFile << "#endif /* !" << sTag << " */\n";
	pFile << "\n";
}

/** \brief searches for a function with the given type
 *  \param sTypeName the name of the type to look for
 *  \param pFile the file to write to (its used to test if a function shall be written)
 *  \return true if a parameter of that type is found
 *
 * Search functions for a parameter with that type.
 */
bool CBEClass::HasFunctionWithUserType(std::string sTypeName, CBEFile* pFile)
{
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if ((*iter)->DoWriteFunction(pFile) &&
			(*iter)->FindParameterType(sTypeName))
			return true;
	}
	return false;
}

/** \brief check if functions have parameters with given attributes
 *  \param nAttribute1 the first attribute
 *  \param nAttribute2 the second attribte
 *  \return true if one such function could be found
 */
bool CBEClass::HasParameterWithAttributes(ATTR_TYPE nAttribute1, ATTR_TYPE nAttribute2)
{
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if ((*iter)->HasParameterWithAttributes(nAttribute1, nAttribute2))
			return true;
	}
	// check base classes
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_BaseClasses.begin();
		iterC != m_BaseClasses.end();
		iterC++)
	{
		if ((*iterC)->HasParameterWithAttributes(nAttribute1, nAttribute2))
			return true;
	}
	return false;
}

/** \brief try to find functions with the given attribute
 *  \param nAttribute the attribute to find
 *  \return true if such a function could be found
 */
bool CBEClass::HasFunctionWithAttribute(ATTR_TYPE nAttribute)
{
	// check own functions
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if ((*iter)->m_Attributes.Find(nAttribute))
			return true;
	}
	// check base classes
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_BaseClasses.begin();
		iterC != m_BaseClasses.end();
		iterC++)
	{
		if ((*iterC)->HasFunctionWithAttribute(nAttribute))
			return true;
	}
	// nothing found
	return false;
}

/** \brief try to find functions with parameters that should be allocated by the user
 *  \return true if such function exists
 */
bool CBEClass::HasMallocParameters()
{
	// check own functions
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if ((*iter)->HasMallocParameters())
			return true;
	}
	// check base classes
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_BaseClasses.begin();
		iterC != m_BaseClasses.end();
		iterC++)
	{
		if ((*iterC)->HasMallocParameters())
			return true;
	}
	// nothing found
	return false;
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEClass::CreateOrderedElementList()
{
	// clear vector
	m_vOrderedElements.clear();
	// typedef
	vector<CBEFunction*>::iterator iterF;
	for (iterF = m_Functions.begin();
		iterF != m_Functions.end();
		iterF++)
	{
		InsertOrderedElement(*iterF);
	}
	// typedef and exceptions
	vector<CBETypedef*>::iterator iterT;
	for (iterT = m_Typedefs.begin();
		iterT != m_Typedefs.end();
		iterT++)
	{
		InsertOrderedElement(*iterT);
	}
	// tagged types
	vector<CBEType*>::iterator iterTa;
	for (iterTa = m_TypeDeclarations.begin();
		iterTa != m_TypeDeclarations.end();
		iterTa++)
	{
		InsertOrderedElement(*iterTa);
	}
	// consts
	vector<CBEConstant*>::iterator iterC;
	for (iterC = m_Constants.begin();
		iterC != m_Constants.end();
		iterC++)
	{
		InsertOrderedElement(*iterC);
	}
}

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBEClass::InsertOrderedElement(CObject *pObj)
{
	// search for element with larger number
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if ((*iter)->m_sourceLoc > pObj->m_sourceLoc)
		{
			// insert before that element
			m_vOrderedElements.insert(iter, pObj);
			return;
		}
	}
	// new object is bigger that all existing
	m_vOrderedElements.push_back(pObj);
}

/** \brief write helper functions, if any
 *  \param pFile the file to write to
 *
 * Writes platform specific helper functions.
 */
void CBEClass::WriteHelperFunctions(CBEHeaderFile& pFile)
{
	WriteDefaultFunction(pFile);
}

/** \brief write the default function
 *  \param pFile the file to write to
 *
 * In case a default function was defined for the server loop we write its
 * declaration into the header file. The implementation has to be user
 * provided.
 */
void CBEClass::WriteDefaultFunction(CBEHeaderFile& pFile)
{
	// check for function prototypes
	if (!pFile.IsOfFileType(FILETYPE_COMPONENT))
		return;

	CBEAttribute *pAttr = m_Attributes.Find(ATTR_DEFAULT_FUNCTION);
	if (!pAttr)
		return;
	string sDefaultFunction = pAttr->GetString();
	if (sDefaultFunction.empty())
		return;

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	string sMsgBuffer = pMsgBuffer->m_Declarators.First()->GetName();
	// int \<name\>(\<corba object\>, \<msg buffer type\>*,
	//              \<corba environment\>*)
	WriteExternCStart(pFile);
	pFile << "\tint " << sDefaultFunction << " (CORBA_Object, " <<
		sMsgBuffer << "*, CORBA_Server_Environment*);\n";
	WriteExternCEnd(pFile);
}

/** \brief write helper functions, if any
 *  \param pFile the file to write to
 *
 * Writes platform specific helper functions.
 */
void CBEClass::WriteHelperFunctions(CBEImplementationFile& /*pFile*/)
{ }

/** \brief retrieve reference to the class' server loop function
 *  \return reference to the class' server loop function
 */
CBESrvLoopFunction* CBEClass::GetSrvLoopFunction()
{
	vector<CBEFunction*>::iterator iter;
	for (iter = m_Functions.begin();
		iter != m_Functions.end();
		iter++)
	{
		if (dynamic_cast<CBESrvLoopFunction*>(*iter))
			return (CBESrvLoopFunction*)(*iter);
	}
	return 0;
}

/** \brief writes the start of extern "C" statement
 *  \param pFile the file to write to
 */
void
CBEClass::WriteExternCStart(CBEFile& pFile)
{
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return;

	pFile << "#ifdef __cplusplus\n" <<
		"extern \"C\" {\n" <<
		"#endif\n\n";
}

/** \brief writes the end of extern "C" statement
 *  \param pFile the file to write to
 */
void
CBEClass::WriteExternCEnd(CBEFile& pFile)
{
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return;

	pFile << "#ifdef __cplusplus\n" <<
		"}\n" <<
		"#endif\n\n";
}

/** \brief creates the CORBA_Object variable (and member)
 *  \return true if successful
 */
void
CBEClass::CreateObject()
{
	// trace remove old object
	if (m_pCorbaObject)
	{
		delete m_pCorbaObject;
		m_pCorbaObject = 0;
	}

	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	string exc = string(__func__);

	string sTypeName("CORBA_Object");
	string sName = pNF->GetCorbaObjectVariable();
	m_pCorbaObject = pCF->GetNewTypedDeclarator();
	m_pCorbaObject->SetParent(this);
	m_pCorbaObject->CreateBackEnd(sTypeName, sName, 0);
	// CORBA_Object is always in
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->CreateBackEnd(ATTR_IN);
	m_pCorbaObject->m_Attributes.Add(pAttr);
}

/** \brief creates the CORBA_Environment variable (and parameter)
 *  \return true if successful
 */
void
CBEClass::CreateEnvironment()
{
	// clean up
	if (m_pCorbaEnv)
	{
		delete m_pCorbaEnv;
		m_pCorbaEnv = 0;
	}

	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	// if function is at server side, this is a CORBA_Server_Environment
	string sTypeName = "CORBA_Server_Environment";
	string sName = pNF->GetCorbaEnvironmentVariable();
	m_pCorbaEnv = pCF->GetNewTypedDeclarator();
	m_pCorbaEnv->SetParent(this);
	m_pCorbaEnv->CreateBackEnd(sTypeName, sName, 1);
}

