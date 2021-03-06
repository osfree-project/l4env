/**
 *    \file    dice/src/be/BEObject.h
 *  \brief   contains the declaration of the class CBEObject
 *
 *    \date    02/13/2001
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
#ifndef __DICE_FE_BEOBJECT_H__
#define __DICE_FE_BEOBJECT_H__

#include "Object.h"
#include <string>
#include "FunctionType.h"

class CBERoot;
class CBETarget;
class CBEFunction;
class CBEStructType;
class CBEUnionType;
class CBEFile;
class CFEBase;
class CBEHeaderFile;
class CBEImplementationFile;
class CBETypedef;
class CBEConstant;
class CBENameSpace;
class CBEClass;
class CBEType;
class CBEEnumType;

/** \class CBEObject
 *    \ingroup backend
 *  \brief The back-end base class
 *
 * This class is the base class for all classes of the back-end. It
 * implements several features, each back-end class might use.
 */
class CBEObject : public CObject
{
public:
	/** constructs a back-end base object
	 *  \param pParent the parent object of this one */
	CBEObject(CObject* pParent = 0);
	virtual ~CBEObject();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CBEObject(CBEObject* src);

	// Operations
public:
	virtual CBEObject* Clone();
	virtual bool IsTargetFile(CBEFile* pFile);

	virtual CBETypedef* FindTypedef(std::string sTypeName, CBETypedef *pPrev = 0);
	virtual CBEConstant* FindConstant(std::string sConstantName);
	virtual CBENameSpace* FindNameSpace(std::string sNameSpaceName);
	virtual CBEClass* FindClass(std::string sClassName);
	virtual CBEType* FindTaggedType(unsigned int nType, std::string sTag);
	virtual CBEFunction* FindFunction(std::string sFunctionName, FUNCTION_TYPE nFunctionType);
	virtual CBEEnumType* FindEnum(std::string sEnumerator);

protected:
	virtual void SetTargetFileName(CFEBase *pFEObject);
	virtual void CreateBackEnd(CFEBase* pFEObject);

	/** \class DoCall
	 *  \brief functor to perform the same call for all elements of a collection
	 */
	template<class C, class A>
	class DoCall
	{
		/** \var C* c
		 *  \brief the class of the collection's members
		 */
		C* c;
		/** \var std::mem_fun1_t<void, C, A*> f
		 *  \brief function to invoke for elements of a collection
		 */
		std::mem_fun1_t<void, C, A*> f;
	public:
		/** \brief constructor
		 *  \param cc the class of the member
		 *  \param ff the function to call
		 */
		explicit DoCall(C* cc, void (C::*ff)(A* arg)) :
			c(cc), f(ff) { }
		/** \brief the operator to invoke
		 *  \param arg the argument to pass to the function
		 */
		void operator() (A* arg) { f(c, arg); }
	};

protected:
	/** \var std::string m_sTargetHeader
	 *  \brief contains the calculated target file name  for the header file
	 */
	std::string m_sTargetHeader;
	/** \var std::string m_sTargetImplementation
	 *  \brief contains the calculated target file name  for the implementation file
	 */
	std::string m_sTargetImplementation;
};

#endif // !__DICE_FE_BEOBJECT_H__
