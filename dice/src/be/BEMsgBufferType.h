/**
 *	\file	dice/src/be/BEMsgBufferType.h
 *	\brief	contains the declaration of the class CBEMsgBufferType
 *
 *	\date	02/13/2002
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
#ifndef __DICE_BEMSGBUFFERTYPE_H__
#define __DICE_BEMSGBUFFERTYPE_H__

#include "be/BETypedef.h"
#include "be/BEClass.h"
#include "Vector.h"

class CBEContext;

class CFEInterface;
class CFETypeSpec;
class CFEDeclarator;

/**	\class CBEMsgBufferType
 *	\ingroup backend
 *	\brief the back-end struct type
 */
class CBEMsgBufferType : public CBETypedef  
{
DECLARE_DYNAMIC(CBEMsgBufferType);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEMsgBufferType();
	virtual ~CBEMsgBufferType();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEMsgBufferType(CBEMsgBufferType &src);

public:
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext * pContext);
    virtual void WriteDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteDefinition(CBEFile *pFile, bool bTypedef, CBEContext *pContext);
    virtual void WriteInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual bool IsVariableSized(int nDirection = 0);
    virtual CObject * Clone();
    virtual bool CreateBackEnd(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext);
    virtual void ZeroCounts(int nDirection = 0);
    virtual int GetFixedCount(int nDirection = 0);
    virtual int GetVariableCount(int nDirection = 0);
    virtual int GetStringCount(int nDirection = 0);
    virtual void WriteMemberAccess(CBEFile *pFile, int nMemberType, CBEContext *pContext);
    virtual void InitCounts(CBEFunction *pFunction, CBEContext *pContext);
    virtual void InitCounts(CBEClass *pClass, CBEContext *pContext);

protected:
    virtual CFETypeSpec* GetMsgBufferType(CFEInterface *pFEInterface, CFEDeclarator* &pFEDeclarator, CBEContext *pContext);
    virtual CFETypeSpec* GetMsgBufferType(CFEOperation *pFEOperation, CFEDeclarator* &pFEDeclarator, CBEContext *pContext);
    virtual void InitCounts(CBEMsgBufferType *pMsgBuffer, CBEContext *pContext);
    virtual void WriteInitializationVarSizedParameters(CBEFile *pFile, CBEContext *pContext);

protected:
    /** \var int m_nFixedCount[2]
     *  \brief number of parameters with fixed size \
     *  (use nDirection-1 to access elements)
     */
    int m_nFixedCount[2];
    /** \var int m_nVariableCount[2];
     *  \brief number of variable sized parameters
     */
    int m_nVariableCount[2];
    /** \var int m_nStringCount[2];
     *  \brief number of string parameters (are according to CORBA variable sized)
     */
    int m_nStringCount[2];
    /** \var CBEMsgBufferType *m_pAliasType
     *  \brief reference to original msg buffer if this is alias
     */
    CBEMsgBufferType *m_pAliasType;
    /** \var bool m_bCountAllVarsAsMax
     *  \brief indicates whether or not all variable sized params should be count as max
     */
    bool m_bCountAllVarsAsMax;
};

#endif // !__DICE_BEMSGBUFFERTYPE_H__
