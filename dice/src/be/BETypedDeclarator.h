/**
 *	\file	dice/src/be/BETypedDeclarator.h
 *	\brief	contains the declaration of the class CBETypedDeclarator
 *
 *	\date	01/18/2002
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
#ifndef __DICE_BETYPEDDECLARATOR_H__
#define __DICE_BETYPEDDECLARATOR_H__

#include "be/BEObject.h"
#include "Vector.h"

class CFETypedDeclarator;
class CBEContext;
class CBEType;
class CBEAttribute;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEDeclarator;
class CDeclaratorStack;

/** defines an invalid array index, used to differ valid from invalid array indices when marshalling */
#define INVALID_ARRAY_INDEX -1

/**	\class CBETypedDeclarator
 *	\ingroup backend
 *	\brief the back-end parameter
 */
class CBETypedDeclarator : public CBEObject
{
DECLARE_DYNAMIC(CBETypedDeclarator);
// Constructor
  public:
	/**	\brief constructor
	 */
    CBETypedDeclarator();
    virtual ~CBETypedDeclarator();

  protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
    CBETypedDeclarator(CBETypedDeclarator & src);

  public:
    virtual void WriteGetSize(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext);
    virtual void WriteZeroInitDeclaration(CBEFile * pFile, CBEContext * pContext);
	virtual void WriteInitDeclaration(CBEFile* pFile, String sInitString, CBEContext* pContext);
	virtual void WriteSetZero(CBEFile* pFile, CBEContext* pContext);
    virtual CBEType *ReplaceType(CBEType * pNewType);
	virtual void WriteCleanup(CBEFile* pFile, CBEContext* pContext);
    virtual void WriteIndirectInitialization(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIndirectInitializationMemory(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIndirect(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteGlobalTestVariable(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual CBEDeclarator* FindDeclarator(String sName);
    virtual bool CreateBackEnd(CBEType * pType, String sName, CBEContext * pContext);
    virtual bool CreateBackEnd(String sUserDefinedType, String sName, int nStars, CBEContext * pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator * pFEParameter, CBEContext * pContext);
    virtual CBEType *GetType();
    virtual void RemoveAttribute(CBEAttribute * pAttribute);
    virtual void RemoveDeclarator(CBEDeclarator * pDeclarator);
    virtual int GetSize();
    virtual int GetMaxSize(CBEContext *pContext);
    virtual bool IsVariableSized();
    virtual bool IsString();
    virtual CBEAttribute *FindAttribute(int nAttrType);
    virtual CBEDeclarator *GetNextDeclarator(VectorElement * &pIter);
    virtual VectorElement *GetFirstDeclarator();
    virtual void AddDeclarator(CBEDeclarator * pDeclarator);
    virtual CBEAttribute *GetNextAttribute(VectorElement * &pIter);
    virtual VectorElement *GetFirstAttribute();
    virtual void AddAttribute(CBEAttribute * pAttribute);
    virtual void WriteType(CBEFile * pFile, CBEContext * pContext, bool bUseConst = true);
    virtual int GetBitfieldSize();
    virtual bool IsDirection(int nDirection);
    virtual bool HasSizeAttr(int nAttr);
    virtual CObject * Clone();
    virtual bool HasReference();
    virtual CBEAttribute* FindIsAttribute(String sDeclName);
    virtual bool IsFixedSized();

  protected:
     virtual void WriteDeclarators(CBEFile * pFile, CBEContext * pContext);
     virtual void WriteAttributes(CBEFile * pFile, CBEContext * pContext);
     virtual void WriteGlobalDeclarators(CBEFile * pFile, CBEContext * pContext);

  protected:
	/**	\var CBEType *m_pType
	 *	\brief the type of the parameter
	 */
     CBEType * m_pType;
	/**	\var Vector m_vAttributes
	 *	\brief contains the type's attributes
	 */
    Vector m_vAttributes;
	/**	\var Vector m_vDeclarators
	 *	\brief the names of the parameter
	 */
    Vector m_vDeclarators;
};

#endif				//*/ !__DICE_BETYPEDDECLARATOR_H__
