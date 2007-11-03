/**
 *  \file    dice/src/be/BEStructType.h
 *  \brief   contains the declaration of the class CBEStructType
 *
 *  \date    01/15/2002
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
#ifndef __DICE_BESTRUCTTYPE_H__
#define __DICE_BESTRUCTTYPE_H__

#include "BEType.h"
#include "BEStructMembers.h"
#include "Attribute-Type.h"

class CBETypedDeclarator;
class CFEArrayType;

/** \class CBEStructType
 *  \ingroup backend
 *  \brief the back-end struct type
 */
class CBEStructType : public CBEType
{
	// Constructor
public:
	/** \brief constructor
	 */
	CBEStructType();
	virtual ~CBEStructType();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CBEStructType(CBEStructType* src);

	virtual void WriteGetMemberSize(CBEFile& pFile,
		CBETypedDeclarator *pMember,
		CDeclStack* pStack,
		CBEFunction *pUsingFunc);
	virtual void CreateBackEndSequence(CFEArrayType *pFEType);

public:
	virtual void WriteZeroInit(CBEFile& pFile);
	virtual int GetSize();
	virtual int GetMaxSize();
	virtual int GetStringLength();
	virtual CBEStructType* Clone();
	virtual void Write(CBEFile& pFile);
	virtual bool IsConstructedType();
	virtual int GetMemberCount();
	virtual bool HasTag(std::string sTag);
	virtual void WriteCastToStr(std::string &str, bool bPointer);
	virtual std::string GetTag();
	virtual void WriteDeclaration(CBEFile& pFile);
	virtual void WriteGetSize(CBEFile& pFile,
		CDeclStack* pStack, CBEFunction *pUsingFunc);
	virtual int GetFixedSize();
	virtual bool IsSimpleType();
	virtual CBETypedDeclarator* FindMember(
		CDeclStack* pStack, CDeclStack::iterator iCurr);
	virtual CBETypedDeclarator* FindMemberAttribute(ATTR_TYPE nAttributeType);
	virtual CBETypedDeclarator* FindMemberIsAttribute(ATTR_TYPE nAttributeType,
		std::string sAttributeParameter);
	virtual void CreateBackEnd(CFETypeSpec *pFEType);
	virtual void CreateBackEnd(std::string sTag, CFEBase *pRefObj);

protected:
	/** \var std::string m_sTag
	 *  \brief the tag if the source is a tagged struct
	 */
	std::string m_sTag;
	/** \var bool m_bForwardDeclaration
	 *  \brief true if this is a forward declaration
	 */
	bool m_bForwardDeclaration;

public:
	/** \var CStructMembers m_Members
	 *  \brief contains the members of this struct
	 */
	CStructMembers m_Members;
};

#endif // !__DICE_BESTRUCTTYPE_H__
