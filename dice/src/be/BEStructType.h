/**
 *	\file	dice/src/be/BEStructType.h
 *	\brief	contains the declaration of the class CBEStructType
 *
 *	\date	01/15/2002
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
#ifndef __DICE_BESTRUCTTYPE_H__
#define __DICE_BESTRUCTTYPE_H__

#include "be/BEType.h"
#include "Vector.h"

class CBEContext;
class CBETypedDeclarator;
class CDeclaratorStack;

/**	\class CBEStructType
 *	\ingroup backend
 *	\brief the back-end struct type
 */
class CBEStructType : public CBEType  
{
DECLARE_DYNAMIC(CBEStructType);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEStructType();
	virtual ~CBEStructType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEStructType(CBEStructType &src);

    virtual void WriteGetMemberSize(CBEFile *pFile, CBETypedDeclarator *pMember, CDeclaratorStack *pStack, CBEContext *pContext);

public:
    virtual void WriteZeroInit(CBEFile *pFile, CBEContext *pContext);
    virtual int GetSize();
    virtual int GetStringLength();
    virtual CObject* Clone();
    virtual void RemoveMember(CBETypedDeclarator *pMember);
    virtual void Write(CBEFile *pFile, CBEContext *pContext);
    virtual CBETypedDeclarator* GetNextMember(VectorElement* &pIter);
    virtual VectorElement* GetFirstMember();
    virtual void AddMember(CBETypedDeclarator *pMember);
    virtual bool CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext);
    virtual bool IsConstructedType();
	virtual int GetMemberCount();
	virtual bool HasTag(String sTag);
	virtual void WriteCast(CBEFile * pFile, bool bPointer, CBEContext * pContext);
    virtual String GetTag();
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteGetSize(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext);
    virtual int GetFixedSize();
    virtual bool IsSimpleType();

protected:
	/**	\var Vector m_vMembers
	 *	\brief contains the members of this struct
	 */
	Vector m_vMembers;
	/**	\var String m_sTag
	 *	\brief the tag if the source is a tagged struct
	 */
	String m_sTag;
};

#endif // !__DICE_BESTRUCTTYPE_H__
