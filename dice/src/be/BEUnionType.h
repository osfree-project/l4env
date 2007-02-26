/**
 *    \file    dice/src/be/BEUnionType.h
 *    \brief   contains the declaration of the class CBEUnionType
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
#ifndef __DICE_BEUNIONTYPE_H__
#define __DICE_BEUNIONTYPE_H__

#include "be/BEType.h"
#include <vector>
using namespace std;

class CBEContext;
class CBEUnionCase;
class CFETypeSpec;
class CBETypedDeclarator;
class CBEDeclarator;

/**    \class CBEUnionType
 *    \ingroup backend
 *    \brief the back-end union type
 */
class CBEUnionType:public CBEType
{
// Constructor
  public:
    /**    \brief constructor
     */
    CBEUnionType();
    virtual ~ CBEUnionType();

  protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBEUnionType(CBEUnionType & src);

    virtual int GetFixedSize();
    virtual void WriteGetMaxSize(CBEFile *pFile,
        const vector<CBEUnionCase*> *pMembers,
        vector<CBEUnionCase*>::iterator iter,
        vector<CDeclaratorStackLocation*> *pStack,
        CBEContext *pContext);
    virtual void WriteGetMemberSize(CBEFile *pFile,
        CBEUnionCase *pMember,
        vector<CDeclaratorStackLocation*> *pStack,
        CBEContext *pContext);

public:
    virtual CObject * Clone();
    virtual void RemoveUnionCase(CBEUnionCase * pCase);
    virtual void Write(CBEFile * pFile, CBEContext * pContext);
    virtual CBEUnionCase *GetNextUnionCase(vector<CBEUnionCase*>::iterator &iter);
    virtual vector<CBEUnionCase*>::iterator GetFirstUnionCase();
    virtual bool IsCStyleUnion();
    virtual string GetSwitchVariableName();
    virtual void AddUnionCase(CBEUnionCase * pUnionCase);
    virtual bool CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext);
    virtual bool CreateBackEnd(string sTag, CBEType *pSwitchType,
        string sSwitchName, bool bCUnion, string sUnionName,
        bool bCORBA, CBEContext *pContext);
    virtual int GetSize();
    virtual CBETypedDeclarator* GetSwitchVariable();
    virtual void WriteUnionName(CBEFile *pFile, CBEContext *pContext);
    virtual CBEDeclarator* GetUnionName();
    virtual int GetUnionCaseCount();
    virtual bool IsConstructedType();
    virtual void WriteCast(CBEFile * pFile, bool bPointer, CBEContext * pContext);
    virtual bool HasTag(string sTag);
    virtual string GetTag();
    virtual void WriteZeroInit(CBEFile * pFile, CBEContext * pContext);
    virtual bool DoWriteZeroInit();
    virtual void WriteGetSize(CBEFile * pFile, vector<CDeclaratorStackLocation*> *pStack, CBEContext * pContext);
    virtual bool IsSimpleType();

  protected:
    /**    \var string m_sTag
     *    \brief the name of the tag if any
     */
     string m_sTag;
    /** \var CBETypedDeclarator *m_pSwitchVariable
     *  \brief the switch variable
     */
    CBETypedDeclarator *m_pSwitchVariable;
    /**    \var CBEDeclarator *m_pUnionName
     *    \brief names the union
     */
    CBEDeclarator *m_pUnionName;
    /**    \var vector<CBEUnionCase*> m_vUnionCases
     *    \brief contains the union's cases
     */
    vector<CBEUnionCase*> m_vUnionCases;
    /** \var bool m_bCORBA
     *  \brief true if CORBA compliant union
     */
    bool m_bCORBA;
    /**  \var bool m_bCUnion
     *   \brief true if this union was declared as C style union (without a switch type and var)
     */
    bool m_bCUnion;
};

#endif                // !__DICE_BEUNIONTYPE_H__
