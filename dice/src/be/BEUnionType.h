/**
 *  \file    dice/src/be/BEUnionType.h
 *  \brief   contains the declaration of the class CBEUnionType
 *
 *  \date    01/15/2002
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
#ifndef __DICE_BEUNIONTYPE_H__
#define __DICE_BEUNIONTYPE_H__

#include "be/BEType.h"
#include "template.h"
#include <vector>

class CBEContext;
class CBEUnionCase;
class CFETypeSpec;
class CBETypedDeclarator;
class CBEDeclarator;

/** \class CBEUnionType
 *  \ingroup backend
 *  \brief the back-end union type
 */
class CBEUnionType : public CBEType
{
	// Constructor
public:
	/** \brief constructor
	 */
	CBEUnionType();
	~CBEUnionType();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CBEUnionType(CBEUnionType* src);

	virtual int GetFixedSize();
	virtual void WriteGetMaxSize(CBEFile& pFile,
		const vector<CBEUnionCase*> *pMembers,
		vector<CBEUnionCase*>::iterator iter,
		CDeclStack* pStack,
		CBEFunction *pUsingFunc);
	virtual void WriteGetMemberSize(CBEFile& pFile,
		CBEUnionCase *pMember,
		CDeclStack* pStack,
		CBEFunction *pUsingFunc);

public:
	virtual void Write(CBEFile& pFile);
	virtual CBEUnionType* Clone();
	virtual CBETypedDeclarator* FindMember(
		CDeclStack* pStack,
		CDeclStack::iterator iCurr);

	virtual void CreateBackEnd(CFETypeSpec * pFEType);
	virtual void CreateBackEnd(std::string sTag);

	virtual int GetSize();
	virtual int GetMaxSize();
	virtual int GetUnionCaseCount();
	virtual void WriteCastToStr(std::string& str, bool bPointer);
	virtual void WriteZeroInit(CBEFile& pFile);
	virtual bool DoWriteZeroInit();
	virtual void WriteGetSize(CBEFile& pFile,
		CDeclStack* pStack, CBEFunction *pUsingFunc);
	virtual void WriteDeclaration(CBEFile& pFile);

	/** \brief return the tag
	 *  \return the tag
	 */
	std::string GetTag()
	{ return m_sTag; }
	/** \brief tests if this union has the given tag
	 *  \param sTag the tag to test for
	 *  \return true if the given tag is the same as the member tag
	 */
	bool HasTag(std::string sTag)
	{ return (m_sTag == sTag); }
	/** \brief test if this is a simple type
	 *  \return false
	 */
	virtual bool IsSimpleType()
	{ return false; }
	/** \brief checks if this is a constructed type
	 *  \return true, because a union is usually regarded a constructed type
	 */
	virtual bool IsConstructedType()
	{ return true; }

protected:
	/** \var std::string m_sTag
	 *  \brief the name of the tag if any
	 */
	std::string m_sTag;

public:
	/** \var CSearchableCollection<CBEUnionCase, std::string> m_UnionCases
	 *  \brief contains the union's cases
	 */
	CSearchableCollection<CBEUnionCase, std::string> m_UnionCases;
};

#endif                // !__DICE_BEUNIONTYPE_H__
