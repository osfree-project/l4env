/**
 *	\file	dice/src/be/BEMarshaller.h
 *	\brief	contains the declaration of the class CBEMarshaller
 *
 *	\date	05/08/2002
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
#ifndef __DICE_BEMARSHALLER_H__
#define __DICE_BEMARSHALLER_H__

#include <be/BEObject.h>
#include <Vector.h>

#include "be/BEDeclarator.h" // needed for declarator-stack

class CBEFile;
class CBEFunction;
class CBETypedDeclarator;
class CBEContext;
class CBEType;

/** \class CBEMarshaller
 *  \brief the class contains the marshalling code
 */
class CBEMarshaller : public CBEObject
{
DECLARE_DYNAMIC(CBEMarshaller);
public:
    /** \brief constructor of marshaller */
	CBEMarshaller();
	~CBEMarshaller();

    virtual int Marshal(CBEFile *pFile, CBEFunction *pFunction, int nStartOffset, bool& bUseConstOffset, CBEContext *pContext);
    virtual int Marshal(CBEFile *pFile, CBEFunction * pFunction, int nFEType, int nNumber, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual int Marshal(CBEFile *pFile, CBETypedDeclarator *pParameter, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext);
    virtual int MarshalValue(CBEFile *pFile, CBEFunction *pFunction, int nBytes, int nValue, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual int Unmarshal(CBEFile * pFile, CBEFunction * pFunction, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual int Unmarshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nNumber, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext);
    virtual int Unmarshal(CBEFile * pFile, CBETypedDeclarator * pParameter, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual bool MarshalToPosition(CBEFile *pFile, CBEFunction *pFunction, int nPosition, int nPosSize, int nDirection, bool bWrite, CBEContext *pContext);
    virtual bool MarshalToPosition(CBEFile *pFile, CBETypedDeclarator *pParameter,  int nPosition,  int nPosSize,  int nCurSize,  bool bWrite,  CBEContext * pContext);
	virtual bool TestPositionSize(CBEFunction *pFunction, int nPosSize, int nDirection, bool bAllowSmaller, bool bAllowLarger, int nNumber, CBEContext *pContext);

protected: // Protected attributes
  /** \var CBEFile *m_pFile
   *  \brief the file to write to
   */
  CBEFile *m_pFile;
  /** \var bool m_bMarshal
   *  \brief true if we are marshalling; false if unmarshalling
   */
  bool m_bMarshal;
  /** \var CBEType *m_pType
   *  \brief the type of the currently marshalled variable
   */
  CBEType *m_pType;
  /** \var Vector m_vDeclaratorStack
   *  \brief the declarator stack for the marshalled parameter
   */
  CDeclaratorStack m_vDeclaratorStack;
  /** \var CBETypedDeclarator *m_pParameter
   *  \brief a reference to the top parameter (used to find attributes)
   */
  CBETypedDeclarator* m_pParameter;
  /** \var CBEFunction *m_pFunction
   *  \brief a reference to the function
   */
  CBEFunction *m_pFunction;

protected: // Protected methods
    virtual int MarshalDeclarator(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext *pContext);
    virtual int MarshalUnion(CBEUnionType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext);
    virtual int MarshalStruct(CBEStructType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext);
    virtual int MarshalArray(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bLastParameter, CBEContext *pContext);
    virtual void WriteBuffer(CBEType *pType, int nStartOffset, bool& bUseConstOffset, bool bDereferencePosition, CBEContext *pContext);
    virtual int MarshalString(CBEType *pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext *pContext);
	virtual int MarshalConstArray(CBEType *pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, VectorElement *pIter, int nLevel, CBEContext *pContext);
    virtual int MarshalVariableArray(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalValue(int nBytes, int nValue, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, CBEContext * pContext);
	virtual int MarshalBitfieldStruct(CBEStructType * pType, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteBuffer(String sTypeName, int nStartOffset, bool & bUseConstOffset, bool bDereferencePosition, CBEContext * pContext);

    virtual bool MarshalDeclaratorToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext);
	virtual bool MarshalArrayToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext);
	virtual bool MarshalConstArrayToPosition(CBEType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, VectorElement *pIter, int nLevel, CBEContext *pContext);
	virtual bool MarshalStructToPosition(CBEStructType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext);
	virtual bool MarshalUnionToPosition(CBEUnionType *pType, int nStartSize, int nPosition, int nPosSize, bool bWrite, CBEContext *pContext);

	virtual bool TestPositionSize(CBETypedDeclarator* pParameter, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext);
	virtual bool TestDeclaratorPositionSize(CBEType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext);
	virtual bool TestArrayPositionSize(CBEType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext);
	virtual bool TestConstArrayPositionSize(CBEType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, VectorElement *pIter, int nLevel, CBEContext *pContext);
	virtual bool TestStructPositionSize(CBEStructType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext);
	virtual bool TestUnionPositionSize(CBEUnionType* pType, int nPosSize, bool bAllowSmaller, bool bAllowLarger, CBEContext *pContext);
};

#endif  // !__DICE_BEMARSHALLER_H__
