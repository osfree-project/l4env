/**
 *    \file    dice/src/be/l4/L4BEMsgBufferType.h
 *    \brief   contains the declaration of the class CL4BEMsgBufferType
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
#ifndef __DICE_L4BEMSGBUFFERTYPE_H__
#define __DICE_L4BEMSGBUFFERTYPE_H__

#include "be/BEMsgBufferType.h"
#include <vector>
using namespace std;

/** \def TYPE_MSGDOPE_SIZE
 *  \brief imitates a new type
 *
 * This is used to support the size and send dopes of the
 * message buffer using WriteMemberAccess.
 */
#define TYPE_MSGDOPE_SIZE  (TYPE_MAX + 3)
/** \def TYPE_MSGDOPE_SEND
 *  \brief imitates a new type
 *  \see TYPE_MSGDOPE_SIZE
 */
#define TYPE_MSGDOPE_SEND  (TYPE_MAX + 4)

/** \def MSGBUF_PROP_SHORT_IPC
 *    \brief lets caller check if short IPC can be used
 */
#define MSGBUF_PROP_SHORT_IPC    1

class CBEContext;
class CFEInterface;

/**    \class CL4BEMsgBufferType
 *    \ingroup backend
 *    \brief the back-end struct type
 */
class CL4BEMsgBufferType : public CBEMsgBufferType
{

// Constructor
public:
    /**    \brief constructor
     */
    CL4BEMsgBufferType();
    virtual ~CL4BEMsgBufferType();

public: // Public methods
    virtual CObject * Clone();
    virtual void WriteInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInitialization(CBEFile *pFile, unsigned int nType, int nDirection, CBEContext *pContext);
    virtual void WriteMemberAccess(CBEFile * pFile, int nMemberType, int nDirection, CBEContext * pContext, string sOffset = string());
    virtual void WriteMemberAccess(CBEFile * pFile, CBETypedDeclarator *pParameter, int nDirection, CBEContext * pContext, string sOffset = string());
    virtual void InitCounts(CBEFunction * pFunction, CBEContext *pContext);
    virtual void InitCounts(CBEClass * pClass, CBEContext *pContext);
    virtual void WriteSetZero(CBEFile *pFile, unsigned int nType, int nDirection, CBEContext *pContext);
    virtual void WriteDump(CBEFile *pFile, string sResult, CBEContext *pContext);
    virtual void WriteDefinition(CBEFile* pFile,  bool bTypedef,  CBEContext* pContext);

    virtual bool CheckProperty(int nProperty, int nDirection, CBEContext* pContext);

protected: // Protected methods
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CL4BEMsgBufferType(CL4BEMsgBufferType &src);
    virtual CFETypeSpec* GetMsgBufferType(CFEInterface *pFEInterface, CFEDeclarator* &pFEDeclarator, CBEContext *pContext);
    virtual CFETypeSpec* GetMsgBufferType(CFEOperation *pFEOperation, CFEDeclarator* &pFEDeclarator, CBEContext * pContext);
    virtual void InitCounts(CBEMsgBufferType * pMsgBuffer, CBEContext * pContext);

    virtual void WriteReceiveIndirectStringInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteReceiveIndirectStringSetZero(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteReceiveFlexpageInitialization(CBEFile *pFile, int nDirection, CBEContext *pContext);
    virtual void WriteSendDopeInit(CBEFile *pFile, int nSendDirection, CBEContext *pContext);
    virtual void WriteSendDopeInit(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteSizeDopeInit(CBEFile *pFile, CBEContext *pContext);

    virtual void WriteSizeOfPayload(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteSizeOfBytes(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteSizeOfRefStrings(CBEFile *pFile, CBEContext *pContext);

    virtual bool IsShortIPC(int nDirection, CBEContext *pContext, int nWords);

protected:
    /** \var vector<int> m_vMaxima
     *  \brief contains the maximum values for the indirect strings
     */
    vector<int> m_vMaxima[2];
};

#endif // !__DICE_L4BEMSGBUFFERTYPE_H__
