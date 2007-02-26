/**
 *	\file	dice/src/Vector.h 
 *	\brief	contains the declaration of the class Vector and VectorElement
 *
 *	\date	01/31/2001
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
#ifndef __DICE_VECTOR_H__
#define __DICE_VECTOR_H__

#include "Object.h"

struct CRuntimeClass;

/**	\class VectorElement
 *	\brief represents an element of a vector
 */
class VectorElement:public CObject
{
DECLARE_DYNAMIC(VectorElement);
// Constructor
  public:
	/** creates a vector element */
    VectorElement();
    ~VectorElement();

// Operations
  public:
    virtual VectorElement * GetPrev();
    virtual VectorElement *GetNext();
    virtual CObject *GetElement();

// Attributes
  protected:
	/**	\var VectorElement *m_pNext
	 *	\brief points to the next object in the chain
	 */
     VectorElement * m_pNext;
	/**	\var VectorElement *m_pPrev
	 *	\brief points to the previous object in the chain
	 */
    VectorElement *m_pPrev;
	/** \var CObject *m_pElement
	 *	\brief points to the element of this vector position
	 */
    CObject *m_pElement;

	/**	allows access to protected memebers to class Vectore */
    friend class Vector;
};

/**	\class Vector
 *	\brief represents a collection of object
 *
 * This collection is type safe, meaning it takes only elements of a preset type or
 * its subclasses. These preset types have to be derived from the class CObject.
 */
class Vector
{
  public:
	/** simple constructor of Vector
	 *	\param pType the type of the elements of this vector
	 */
    Vector(CRuntimeClass * pType);
	/** constructs the vector with a number of elements
	 *	\param pType the type of the elements
	 *	\param nNumElements the number of elements
	 *	\param pElement the element prototype
	 */
    Vector(CRuntimeClass * pType, int nNumElements, CObject * pElement);
     virtual ~ Vector();

  protected:
	/** \brief  copy constructor
	 *	\param src the source to copy from
	 */
     Vector(Vector & src);

// Operations
public:
    virtual bool SetAt(int nIndex, CObject * pObject);
    virtual CObject *GetAt(int nIndex);
    void DeleteAll();
    virtual CObject *RemoveFirst();
    virtual CObject *RemoveAt(int nIndex);
    virtual void SetParentOfElements(CObject * pParent);
    virtual Vector *Clone();
    int GetSize();
    void Add(Vector * pOldVector);
    VectorElement *GetLast();
    VectorElement *GetFirst();
    void AddHead(CObject * pElement);
    bool Remove(CObject * pElement);
    void Add(CObject * pElement);
    virtual bool Exchange(CObject *pObject1, CObject *pObject2);
    virtual void AddUnique(CObject *pElement);

  protected:
	/**	\var VectorElement* m_pTail
	 *	\brief a reference to the end of the chain
	 */
     VectorElement * m_pTail;
	/**	\var VectorElement* m_pHead
	 *	\brief a reference to the beginning of the chain
	 */
    VectorElement *m_pHead;
	/**	\var CRuntimeClass* m_pType
	 *	\brief the CRuntimeClass used for type checking
	 */
    CRuntimeClass *m_pType;
};

#endif				// __DICE_VECTOR_H__
