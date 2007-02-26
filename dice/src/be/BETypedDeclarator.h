/**
 *    \file    dice/src/be/BETypedDeclarator.h
 *    \brief   contains the declaration of the class CBETypedDeclarator
 *
 *    \date    01/18/2002
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
#ifndef __DICE_BETYPEDDECLARATOR_H__
#define __DICE_BETYPEDDECLARATOR_H__

#include "be/BEObject.h"
#include <vector>
using namespace std;

class CFETypedDeclarator;
class CBEContext;
class CBEType;
class CBEAttribute;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEDeclarator;
class CDeclaratorStackLocation;

/** defines an invalid array index, used to differ valid from invalid array indices when marshalling */
#define INVALID_ARRAY_INDEX -1

/**    \class CBETypedDeclarator
 *    \ingroup backend
 *    \brief the back-end parameter
 */
class CBETypedDeclarator : public CBEObject
{

// Constructor
  public:
    /**    \brief constructor
     */
    CBETypedDeclarator();
    virtual ~CBETypedDeclarator();

  protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBETypedDeclarator(CBETypedDeclarator & src);

  public:
    virtual void WriteGetSize(CBEFile * pFile, vector<CDeclaratorStackLocation*> *pStack, CBEContext * pContext);
    virtual void WriteZeroInitDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteInitDeclaration(CBEFile* pFile, string sInitString, CBEContext* pContext);
    virtual void WriteSetZero(CBEFile* pFile, CBEContext* pContext);
    virtual void WriteCleanup(CBEFile* pFile, CBEContext* pContext);
    virtual void WriteDeferredCleanup(CBEFile* pFile, CBEContext* pContext);
    virtual void WriteIndirectInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIndirectInitializationMemory(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIndirect(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteGlobalTestVariable(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteType(CBEFile * pFile, CBEContext * pContext, bool bUseConst = true);

    virtual bool CreateBackEnd(CBEType * pType, string sName, CBEContext * pContext);
    virtual bool CreateBackEnd(string sUserDefinedType, string sName, int nStars, CBEContext * pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator * pFEParameter, CBEContext * pContext);

    virtual int GetSize();
    virtual int GetSize(string sName);
    virtual CBEType *GetType();
    virtual int GetBitfieldSize();
    virtual int GetMaxSize(bool bGuessSize, CBEContext *pContext);

    virtual CBEType *ReplaceType(CBEType * pNewType);

    virtual bool IsVariableSized();
    virtual bool IsString();
    virtual bool IsDirection(int nDirection);
    virtual bool IsFixedSized();
    virtual bool HasSizeAttr(int nAttr);
    virtual bool HasReference();

    virtual CBEDeclarator* FindDeclarator(string sName);
    virtual CBEDeclarator *GetNextDeclarator(vector<CBEDeclarator*>::iterator &iter);
    virtual CBEDeclarator *GetDeclarator();
    virtual CBEDeclarator *GetCallDeclarator();
    virtual vector<CBEDeclarator*>::iterator GetFirstDeclarator();
    virtual bool IsLastDeclarator(vector<CBEDeclarator*>::iterator iter);
    virtual void RemoveDeclarator(CBEDeclarator * pDeclarator);
    virtual void AddDeclarator(CBEDeclarator * pDeclarator);

    virtual CBEAttribute* FindIsAttribute(string sDeclName);
    virtual CBEAttribute *FindAttribute(int nAttrType);
    virtual CBEAttribute *GetNextAttribute(vector<CBEAttribute*>::iterator &iter);
    virtual vector<CBEAttribute*>::iterator GetFirstAttribute();
    virtual void RemoveAttribute(CBEAttribute * pAttribute);
    virtual void AddAttribute(CBEAttribute * pAttribute);

    virtual CObject * Clone();

  protected:
     virtual void WriteDeclarators(CBEFile * pFile, CBEContext * pContext);
     virtual void WriteAttributes(CBEFile * pFile, CBEContext * pContext);
     virtual void WriteGlobalDeclarators(CBEFile * pFile, CBEContext * pContext);

     virtual int GetSizeOfDeclarator(CBEDeclarator *pDeclarator);
     virtual CBEType* GetTransmitType();

  protected:
    /**    \var CBEType *m_pType
     *    \brief the type of the parameter
     */
     CBEType * m_pType;
    /**    \var vector<CBEAttribute*> m_vAttributes
     *    \brief contains the type's attributes
     */
    vector<CBEAttribute*> m_vAttributes;
    /**    \var vector<CBEDeclarator*> m_vDeclarators
     *    \brief the names of the parameter
     */
    vector<CBEDeclarator*> m_vDeclarators;
};

#endif                //*/ !__DICE_BETYPEDDECLARATOR_H__
