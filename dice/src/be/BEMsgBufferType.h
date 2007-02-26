/**
 *    \file    dice/src/be/BEMsgBufferType.h
 *    \brief   contains the declaration of the class CBEMsgBufferType
 *
 *    \date    02/13/2002
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
#ifndef __DICE_BEMSGBUFFERTYPE_H__
#define __DICE_BEMSGBUFFERTYPE_H__

#include "be/BETypedef.h"
#include "be/BEClass.h"
#include <vector>
using namespace std;

class CBEContext;

class CFEInterface;
class CFETypeSpec;
class CFEDeclarator;

/** \def TYPE_FIXED
 *  \brief imitates a new type
 *
 * This is used to support the fixed sized member of the
 * message buffer using WriteMemberAccess.
 */
#define TYPE_FIXED     (TYPE_MAX + 1)
/** \def TYPE_VARSIZED
 *  \brief imitates a new type
 *
 * This is used to support the variable sized member of the
 * message buffer using WriteMemberAccess.
 */
#define TYPE_VARSIZED  (TYPE_MAX + 2)

/**    \class CBEMsgBufferType
 *    \ingroup backend
 *    \brief the back-end struct type
 */
class CBEMsgBufferType : public CBETypedef
{

// Constructor
public:
    /**    \brief constructor
     */
    CBEMsgBufferType();
    virtual ~CBEMsgBufferType();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBEMsgBufferType(CBEMsgBufferType &src);

    /**    \brief helper type */
    struct TypeCount
    {
        unsigned int nType;
        unsigned int nCount;
    };

public:
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext * pContext);
    virtual bool IsVariableSized(int nDirection = 0);
    virtual CObject * Clone();
    virtual bool CreateBackEnd(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext);
    virtual void ZeroCounts(int nDirection = 0);
    virtual unsigned int GetCount(unsigned int nType, int nDirection = 0);
    virtual void InitCounts(CBEFunction *pFunction, CBEContext *pContext);
    virtual void InitCounts(CBEClass *pClass, CBEContext *pContext);
    virtual bool NeedCast(int nFEType, CBEContext *pContext);

    virtual void WriteDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteDefinition(CBEFile *pFile, bool bTypedef, CBEContext *pContext);
    virtual void WriteInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInitialization(CBEFile *pFile, unsigned int nType, int nDirection, CBEContext *pContext);
    virtual void WriteDump(CBEFile *pFile, string sResult, CBEContext *pContext);
    virtual void WriteMemberAccess(CBEFile *pFile, int nMemberType, int nDirection, CBEContext *pContext, string sOffset = string());
    virtual void WriteMemberAccess(CBEFile *pFile, CBETypedDeclarator *pParameter, int nDirection, CBEContext *pContext, string sOffset = string());
    virtual void WriteSetZero(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteSetZero(CBEFile *pFile, unsigned int nType, int nDirection, CBEContext *pContext);

    virtual bool CheckProperty(int nProperty, int nDirection, CBEContext *pContext);

protected:
    virtual CFETypeSpec* GetMsgBufferType(CFEInterface *pFEInterface, CFEDeclarator* &pFEDeclarator, CBEContext *pContext);
    virtual CFETypeSpec* GetMsgBufferType(CFEOperation *pFEOperation, CFEDeclarator* &pFEDeclarator, CBEContext *pContext);
    virtual void InitCounts(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext);
    virtual void WriteInitializationVarSizedParameters(CBEFile *pFile, CBEContext *pContext);
    virtual vector<struct CBEMsgBufferType::TypeCount>::iterator GetCountIter(unsigned int nType, int nDirection);

protected:
    /** \var vector<TypeCount> m_vCounts[2]
     *  \brief contains the counters of parameters of a specific type
     *
     * To specify fixed sized counts, use for nType TYPE_MAX + 1 (TYPE_FIXED)
     * and for variable sized parameters use TYPE_MAX + 2 (TYPE_VARSIZED).
     */
    vector<struct CBEMsgBufferType::TypeCount> m_vCounts[2];

    /** \var CBEMsgBufferType *m_pAliasType
     *  \brief reference to original msg buffer if this is alias
     */
    CBEMsgBufferType *m_pAliasType;
    /** \var bool m_bCountAllVarsAsMax
     *  \brief indicates whether or not all variable sized params should be count as max
     */
    bool m_bCountAllVarsAsMax;
    /**    \var CBEFunction *m_pFunction
     *    \brief if this message buffer belongs to a function, this variable is
     *    set to this function
     */
    CBEFunction *m_pFunction;
};

#endif // !__DICE_BEMSGBUFFERTYPE_H__
