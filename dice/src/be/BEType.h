/**
 *	\file	dice/src/be/BEType.h
 *	\brief	contains the declaration of the class CBEType
 *
 *	\date	01/15/2002
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
#ifndef __DICE_BETYPE_H__
#define __DICE_BETYPE_H__

#include "be/BEObject.h"

class CBEContext;
class CBEFile;
class CBETypedef;
class CFETypeSpec;
class CDeclaratorStack;
class CBEDeclarator;
class CBEType;
class VectorElement;

/**	\class CBEType
 *	\ingroup backend
 *	\brief the back-end type
 */
class CBEType : public CBEObject
{
DECLARE_DYNAMIC(CBEType);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEType();
	virtual ~CBEType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEType(CBEType &src);

public:
    virtual bool IsUnsigned();
    virtual int GetFEType();
    virtual void WriteZeroInit(CBEFile *pFile, CBEContext *pContext);
    virtual int GetStringLength();
    virtual CObject* Clone();
    virtual bool IsOfType(int nFEType);
    virtual int GetSize();
    virtual bool IsVoid();
    virtual bool CreateBackEnd(bool bUnsigned, int nSize, int nFEType, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext);
	virtual CBETypedef* GetTypedef();
    virtual void Write(CBEFile *pFile, CBEContext *pContext);
    virtual bool IsConstructedType();
	virtual bool HasTag(String sTag);
	virtual void WriteCast(CBEFile *pFile, bool bPointer, CBEContext *pContext);
    virtual bool IsPointerType();
	virtual bool IsArrayType();
    virtual void WriteDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual bool DoWriteZeroInit();
    virtual void WriteGetSize(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext);
    virtual bool IsSimpleType();
	virtual int GetArrayDimensionCount();
	virtual int GetIndirectionCount();
	virtual void WriteIndirect(CBEFile* pFile, CBEContext* pContext);

	virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);

protected:
	virtual void WriteZeroInitArray(CBEFile *pFile, CBEType *pType, CBEDeclarator *pAlias, VectorElement *pIter, CBEContext *pContext);

protected:
	/**	\var bool m_bUnsigned
	 *	\brief indicates if this type is unsigned
	 */
	bool m_bUnsigned;
	/**	\var unsigned int m_nSize
	 *	\brief specifies the size of the type in bytes
	 */
	int m_nSize;
	/**	\var String m_sName
	 *	\brief the fully extended name of the type
	 *
	 * A type's name might be extended by the library or interface name. These extension are already in this variable.
	 */
	String m_sName;
	/**	\var in m_nFEType
	 *	\brief only used for comparison
	 */
	int m_nFEType;
};

#endif /* !__DICE_BETYPE_H__ */
