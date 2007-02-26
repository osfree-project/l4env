/**
 *    \file    dice/src/be/BEStructType.h
 *    \brief   contains the declaration of the class CBEStructType
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
#ifndef __DICE_BESTRUCTTYPE_H__
#define __DICE_BESTRUCTTYPE_H__

#include "be/BEType.h"
#include <vector>
using namespace std;

class CBEContext;
class CBETypedDeclarator;
class CFEArrayType;
class CDeclaratorStackLocation;

/**    \class CBEStructType
 *    \ingroup backend
 *    \brief the back-end struct type
 */
class CBEStructType : public CBEType
{
// Constructor
public:
    /**    \brief constructor
     */
    CBEStructType();
    virtual ~CBEStructType();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBEStructType(CBEStructType &src);

    virtual void WriteGetMemberSize(CBEFile *pFile, CBETypedDeclarator *pMember, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual bool CreateBackEndSequence(CFEArrayType *pFEType, CBEContext *pContext);

public:
    virtual void WriteZeroInit(CBEFile *pFile, CBEContext *pContext);
    virtual int GetSize();
    virtual int GetStringLength();
    virtual CObject* Clone();
    virtual void RemoveMember(CBETypedDeclarator *pMember);
    virtual void Write(CBEFile *pFile, CBEContext *pContext);
    virtual CBETypedDeclarator* GetNextMember(vector<CBETypedDeclarator*>::iterator &iter);
    virtual vector<CBETypedDeclarator*>::iterator GetFirstMember();
    virtual void AddMember(CBETypedDeclarator *pMember);
    virtual bool CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext);
    virtual bool IsConstructedType();
    virtual int GetMemberCount();
    virtual bool HasTag(string sTag);
    virtual void WriteCast(CBEFile * pFile, bool bPointer, CBEContext * pContext);
    virtual string GetTag();
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteGetSize(CBEFile * pFile, vector<CDeclaratorStackLocation*> *pStack, CBEContext * pContext);
    virtual int GetFixedSize();
    virtual bool IsSimpleType();
    virtual CBETypedDeclarator* FindMember(string sName);
    virtual CBETypedDeclarator* FindMemberAttribute(int nAttributeType);
    virtual CBETypedDeclarator* FindMemberIsAttribute(int nAttributeType, string sAttributeParameter);

protected:
    /** \var vector<CBETypedDeclarator*> m_vMembers
     *  \brief contains the members of this struct
     */
    vector<CBETypedDeclarator*> m_vMembers;
    /** \var string m_sTag
     *  \brief the tag if the source is a tagged struct
     */
    string m_sTag;
    /** \var bool m_bForwardDeclaration
     *  \brief true if this is a forward declaration
     */
    bool m_bForwardDeclaration;
};

#endif // !__DICE_BESTRUCTTYPE_H__
