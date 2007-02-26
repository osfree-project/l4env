/**
 *	\file	dice/src/be/BEUserDefinedType.h
 *	\brief	contains the declaration of the class CBEUserDefinedType
 *
 *	\date	02/13/2002
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
#ifndef __DICE_BEUSERDEFINEDTYPE_H__
#define __DICE_BEUSERDEFINEDTYPE_H__

#include "be/BEType.h"
#include "Vector.h"

class CBEContext;
class CBEUnionCase;
class CFETypeSpec;

/**	\class CBEUserDefinedType
 *	\ingroup backend
 *	\brief the back-end union type
 */
class CBEUserDefinedType : public CBEType
{
DECLARE_DYNAMIC(CBEUserDefinedType);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEUserDefinedType();
	virtual ~CBEUserDefinedType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEUserDefinedType(CBEUserDefinedType &src);

public:
    virtual void WriteZeroInit(CBEFile *pFile, CBEContext *pContext);
    virtual int GetSize();
    virtual String GetName();
    virtual CObject* Clone();
    virtual bool IsConstructedType();
    virtual bool CreateBackEnd(String sName, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext);
    virtual bool DoWriteZeroInit();
    virtual void WriteGetSize(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext);
    virtual bool IsSimpleType();
	virtual bool IsArrayType();
	virtual bool IsPointerType();
	virtual int GetArrayDimensionCount();
	virtual int GetIndirectionCount();
	virtual void WriteIndirect(CBEFile* pFile,  CBEContext* pContext);

protected:
	virtual int GetSizeOfType(String sTypeName);
	virtual CBEType* GetRealType();
	virtual CBEDeclarator* GetRealName();
};

#endif // !__DICE_BEUSERDEFINEDTYPE_H__
