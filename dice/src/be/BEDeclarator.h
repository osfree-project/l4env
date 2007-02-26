/**
 *	\file	dice/src/be/BEDeclarator.h
 *	\brief	contains the declaration of the class CBEDeclarator
 *
 *	\date	01/15/2002
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
#ifndef __DICE_BEDECLARATOR_H__
#define __DICE_BEDECLARATOR_H__

#include "be/BEObject.h"
#include "Vector.h"

class CFEDeclarator;
class CFEArrayDeclarator;
class CFEEnumDeclarator;
class CFEExpression;

class CBEContext;
class CBEExpression;
class CBEFile;

class CBEDeclarator;

/** defines the maximum number if array bounds a declarator
 * might have and thus how many indices may be available for
 * a declarators.
 */
#define MAX_INDEX_NUMBER    10

/** \class CDeclaratorStackLocation
 *  \brief class to represent a stack location for the declarator stack
 *
 * We use an extra class here, because we want to use Vector for the Stack and it
 * only accepts classes derived from CObject.
 */
class CDeclaratorStackLocation : public CObject
{
DECLARE_DYNAMIC(CDeclaratorStackLocation);
public:
    /** \brief constructor of this class
     *  \param pDecl the declarator
     */
    CDeclaratorStackLocation(CBEDeclarator *pDecl)
    {
        IMPLEMENT_DYNAMIC_BASE(CDeclaratorStackLocation, CObject);
        pDeclarator = pDecl;
		for (int i=0; i<MAX_INDEX_NUMBER; i++)
		{
            nIndex[i] = -1;
			sIndex[i].Empty();
		}
        bCheckFunctionForReference = false;
    };
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CDeclaratorStackLocation(CDeclaratorStackLocation &src)
	: pDeclarator(src.pDeclarator)
	{
	    IMPLEMENT_DYNAMIC_BASE(CDeclaratorStackLocation, CObject);
// 		pDeclarator = (CBEDeclarator*)(src.pDeclarator->Clone());
		for (int i=0; i<MAX_INDEX_NUMBER; i++)
		{
		    nIndex[i] = src.nIndex[i];
			sIndex[i] = src.sIndex[i];
		}
		bCheckFunctionForReference = src.bCheckFunctionForReference;
	};
    /** \var CBEDeclarator *pDeclarator
     *  \brief a reference to the declarator at this location
     */
    CBEDeclarator *pDeclarator;
    /** \var int nIndex[MAX_INDEX_NUMBER]
     *  \brief the index of this declarator in an array (or -1 if no array)
     */
    int nIndex[MAX_INDEX_NUMBER];
    /** \var String sIndex
     *  \brief the name of the index variable if there is no fixed index
     */
    String sIndex[MAX_INDEX_NUMBER];
    /** \var bool bCheckFunctionForReference
     *  \brief true if WriteDeclaratorStack has to check pFunction->HasAdditionalReference
     */
    bool bCheckFunctionForReference;
    /** \brief sets the index of the stack location
     *  \param nIdx the new index
	 *  \param nArrayDim which of the array dimensions is meant
     *
     * The array index is -1 if this stack location is new, its 0 or above
     * if this stack location is an array, or -2 if the array index is a sIndex.
     */
    void SetIndex(int nIdx, int nArrayDim = 0)
    {
        if (nIdx < -3)
            return;
		if ((nArrayDim < 0) || (nArrayDim >= MAX_INDEX_NUMBER))
		    return;
        nIndex[nArrayDim] = nIdx;
        sIndex[nArrayDim].Empty();
    }
    /** \brief sets the index variable of the stack location
     *  \param sIdx the new stack location variable
     */
    void SetIndex(String sIdx, int nArrayDim = 0)
    {
        if (sIdx.IsEmpty())
            return;
		if ((nArrayDim < 0) || (nArrayDim >= MAX_INDEX_NUMBER))
		    return;
        nIndex[nArrayDim] = -2;
        sIndex[nArrayDim] = sIdx;
    }
    /** \brief checks if this stack location has an index
	 *  \param nArrayDim the array dimension to check
     *  \return true if nIndex != -1
     */
    bool HasIndex(int nArrayDim = 0)
    {
		if ((nArrayDim < 0) || (nArrayDim >= MAX_INDEX_NUMBER))
		    return false;
        return nIndex[nArrayDim] != -1;
    }
    /** \brief turns on the test for the additional reference
     */
    void SetRefCheck()
    {
        bCheckFunctionForReference = true;
    }
	/** \brief get the number of used indices
	 *  \return the number of used indices
	 *
	 * A index is used if nIndex[..] != -1
	 */
	int GetUsedIndexCount()
	{
	    int nRet = 0;
		while (HasIndex(nRet)) nRet++;
		return nRet;
	}
};

/** \class CDeclaratorStack
 *  \ingroup backend
 *  \brief wrapper class for a declarator stack
 */
class CDeclaratorStack : public CBEObject
{
DECLARE_DYNAMIC(CDeclaratorStack);

public:
    /** creates a new declarator stack */
    CDeclaratorStack() :  vStack(RUNTIME_CLASS(CDeclaratorStackLocation))
    {
        IMPLEMENT_DYNAMIC_BASE(CDeclaratorStack, CBEObject);
    }

    /** \brief adds another declarator to the stack
     *  \param pDecl the declarator to add
     */
    void Push(CBEDeclarator *pDecl)
    {
        CDeclaratorStackLocation *pNew = new CDeclaratorStackLocation(pDecl);
        vStack.AddHead(pNew);
    }

    /** \brief adds another declarator to the stack
     *  \param pDecl the declarator to add
     */
    void Push(CDeclaratorStackLocation *pStackLoc)
    {
        CDeclaratorStackLocation *pNew = new CDeclaratorStackLocation(*pStackLoc);
        vStack.AddHead(pNew);
    }

	/** \brief removes the top-most stack location
     *  \return the top-most declarator
     */
    CBEDeclarator *Pop()
    {
        CDeclaratorStackLocation *pTop = (CDeclaratorStackLocation*)vStack.RemoveFirst();
        CBEDeclarator *pDecl = pTop->pDeclarator;
        delete pTop;
        return pDecl;
    }

	/** \brief retrieves a pointer to the top-most stack location
     *  \return a reference to the top-most stack location
     */
    CDeclaratorStackLocation *GetTop()
    {
        if (!vStack.GetFirst())
            return NULL;
        return (CDeclaratorStackLocation*)(vStack.GetFirst()->GetElement());
    }

	/** \brief retrieves the last stack location
	 *  \return a referebce to the bottom-most stack location
	 */
	CDeclaratorStackLocation *GetBottom()
	{
	    if (!vStack.GetLast())
		    return NULL;
	    return (CDeclaratorStackLocation*)(vStack.GetLast()->GetElement());
	}

	/** \brief returns the number of used stack locations
	 *  \return the size of the vector
	 */
    int GetSize()
    {
	    return vStack.GetSize();
	}

    void Write(CBEFile *pFile, bool bUsePointer, bool bFirstIsGlobal, CBEContext *pContext);

protected:
    /** \var Vector vStack
     *  \brief the stack containing the declarators
     */
    Vector vStack;
};


/**	\class CBEDeclarator
 *	\ingroup backend
 *	\brief the back-end declarator
 */
class CBEDeclarator : public CBEObject
{
DECLARE_DYNAMIC(CBEDeclarator);
// Constructor
  public:
	/**	\brief constructor
	 */
    CBEDeclarator();
    virtual ~ CBEDeclarator();

  protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
    CBEDeclarator(CBEDeclarator & src);

  public:
    virtual int GetArrayDimensionCount();
    virtual bool IsArray();
    virtual void WriteName(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteIndirectInitialization(CBEFile * pFile, bool bUsePointer, CBEContext * pContext);
    virtual void WriteIndirectInitializationMemory(CBEFile * pFile, bool bUsePointer, CBEContext * pContext);
    virtual void WriteIndirect(CBEFile * pFile, bool bUsePointer, bool bHasPointerType, CBEContext * pContext);
    virtual void WriteGlobalName(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext, bool bWriteArray = false);
    virtual void WriteDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual bool CreateBackEnd(String sName, int nStars, CBEContext * pContext);
    virtual bool CreateBackEnd(CFEDeclarator * pFEDeclarator, CBEContext * pContext);
    virtual int GetStars();
    virtual int GetSize();
    virtual CBEExpression *GetNextArrayBound(VectorElement * &pIter);
    virtual VectorElement *GetFirstArrayBound();
    virtual void AddArrayBound(CBEExpression * pBound);
    virtual String GetName();
    virtual int GetBitfields();
    virtual int IncStars(int nBy);
    virtual int GetMaxSize(CBEContext *pContext);
    virtual CObject * Clone();
    virtual void RemoveArrayBound(CBEExpression *pBound);
	virtual int GetRemainingNumberOfArrayBounds(VectorElement *pIter);
	virtual void WriteCleanup(CBEFile * pFile, bool bUsePointer, CBEContext * pContext);

protected:
    virtual int GetFakeStars();
    virtual bool CreateBackEndArray(CFEArrayDeclarator * pFEArrayDeclarator, CBEContext * pContext);
    virtual bool CreateBackEndEnum(CFEEnumDeclarator * pFEEnumDeclarator, CBEContext * pContext);
    virtual CBEExpression *GetArrayDimension(CFEExpression * pLower, CFEExpression * pUpper, CBEContext * pContext);
    virtual void WriteArray(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteArrayIndirect(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteEnum(CBEFile * pFile, CBEContext * pContext);

protected:
	/**	\var String m_sName
	 *	\brief the name of the declarator
	 */
    String m_sName;
	/**	\var int m_nStars
	 *	\brief the number of asterisks (indirection) of this declarator
	 */
    int m_nStars;
	/**	\var int m_nBitfields
	 *	\brief the bitfield number (especially used with structs)
	 */
    int m_nBitfields;
	/**	\var int m_nType
	 *	\brief defines the value in the union
	 */
    int m_nType;
    /** \var int m_nOldType
     *  \brief contains a backup of the type value if a temporary array is established
     */
    int m_nOldType;
	/**	\var CBEExpression *m_pInitialValue
	 *	\brief contains a reference to the initail value of the enum declarator
	 */
    CBEExpression *m_pInitialValue;
	/**	\var Vector *m_pBounds
	 *	\brief contains the array bounds
	 */
    Vector *m_pBounds;
};

#endif				//*/ !__DICE_BEDECLARATOR_H__
