/**
 *    \file    dice/src/be/BEType.h
 *  \brief   contains the declaration of the class CBEType
 *
 *    \date    01/15/2002
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
#ifndef __DICE_BETYPE_H__
#define __DICE_BETYPE_H__

#include "BEObject.h"
#include "BEDeclarator.h"
#include <vector>
using std::vector;

class CBEFile;
class CBETypedef;
class CFETypeSpec;
class CBEType;
class CBEExpression;
class CDeclaratorStackLocation;

/** \class CBEType
 *  \ingroup backend
 *  \brief the back-end type
 */
class CBEType : public CBEObject
{

// Constructor
public:
    /** \brief constructor
     */
    CBEType();
    virtual ~CBEType();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEType(CBEType &src);

public:
    virtual bool IsUnsigned();
    virtual int GetFEType();
    virtual void WriteZeroInit(CBEFile& pFile);
    virtual int GetStringLength();
    virtual CObject* Clone();
    virtual bool IsOfType(int nFEType);
    virtual int GetSize();
    virtual int GetMaxSize();
    virtual bool IsVoid();
    virtual void CreateBackEnd(bool bUnsigned, int nSize, int nFEType);
    virtual void CreateBackEnd(CFETypeSpec *pFEType);
    virtual CBETypedef* GetTypedef();
    virtual void Write(CBEFile& pFile);
    virtual void WriteToStr(string &str);
    virtual bool IsConstructedType();
    virtual bool HasTag(string sTag);
    virtual void WriteCast(CBEFile& pFile, bool bPointer);
    virtual bool IsPointerType();
    virtual bool IsArrayType();
    virtual void WriteDeclaration(CBEFile& pFile);
    virtual bool DoWriteZeroInit();
    virtual void WriteGetSize(CBEFile& pFile, CDeclStack* pStack,
	CBEFunction *pUsingFunc);
    virtual void WriteGetMaxSize(CBEFile& pFile, CDeclStack* pStack,
	CBEFunction *pUsingFunc);
    virtual bool IsSimpleType();
    virtual int GetArrayDimensionCount();
    virtual int GetIndirectionCount();
    virtual void WriteIndirect(CBEFile& pFile);

    virtual void AddToHeader(CBEHeaderFile* pHeader);

protected:
    virtual void WriteZeroInitArray(CBEFile& pFile, CBEType *pType, 
	CBEDeclarator *pAlias, vector<CBEExpression*>::iterator iter);

protected:
    /** \var bool m_bUnsigned
     *  \brief indicates if this type is unsigned
     */
    bool m_bUnsigned;
    /** \var int m_nSize
     *  \brief cached value of size
     */
    int m_nSize;
    /** \var int m_nMaxSize
     *  \brief cached value of maximum size
     */
    int m_nMaxSize;
    /** \var string m_sName
     *  \brief the fully extended name of the type
     *
     * A type's name might be extended by the library or interface name. These
     * extension are already in this variable.
     */
    string m_sName;
    /** \var in m_nFEType
     *  \brief only used for comparison
     */
    int m_nFEType;
};

#endif /* !__DICE_BETYPE_H__ */
