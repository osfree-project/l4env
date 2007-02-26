/**
 *	\file	dice/src/Vector.cpp
 *	\brief	contains the implementation of the class Vector and VectorElement
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

#include "Vector.h"
#include "defines.h"

/////////////////////////////////////////////////////////////////////
// Vector Elements

IMPLEMENT_DYNAMIC(VectorElement)

VectorElement::VectorElement()
{
    m_pNext = m_pPrev = 0;
    m_pElement = 0;
    IMPLEMENT_DYNAMIC_BASE(VectorElement, CObject);
}

/** cleans up the vector element */
VectorElement::~VectorElement()
{
    m_pNext = m_pPrev = 0;
    m_pElement = 0;
}

/** \brief returns a reference to the next object
 *  \return a reference to the next object
 */
VectorElement *VectorElement::GetNext()
{
    return m_pNext;
}

/** \brief returns a reference to the previous object
 *  \return a reference to the previous object
 */
VectorElement *VectorElement::GetPrev()
{
    return m_pPrev;
}

/** \brief returns the element (or reference to it)
 *  \return the element (or reference to it)
 */
CObject *VectorElement::GetElement()
{
    return m_pElement;
}

//////////////////////////////////////////////////////////////////////
// Vector itself

Vector::Vector(CRuntimeClass * pType)
{
    m_pType = pType;
    m_pHead = m_pTail = 0;
}

Vector::Vector(CRuntimeClass * pType, int nNumElements, CObject * pElement)
{
    m_pType = pType;
    m_pHead = m_pTail = 0;
    for (int i = 0; i < nNumElements; i++)
        Add(pElement);
}

Vector::Vector(Vector & src)
{
    m_pHead = m_pTail = 0;
    m_pType = src.m_pType;
    VectorElement *pIter = src.GetFirst();
    while (pIter)
    {
        if (pIter->GetElement())
            Add(pIter->GetElement()->Clone());
        pIter = pIter->GetNext();
    }
}

/** cleans up the vector and its elements */
Vector::~Vector()
{
    m_pType = 0;
    while (GetFirst())
        RemoveFirst();
    m_pHead = m_pTail = 0;
}

/** \brief add an object to the vector
 *  \param pElement the new member of the vector
 *
 * This function creates a new VectorElement, sets its element pointer and appends it
 * to the end of the chain.
 *
 * Do not add empty elements (pElement == 0)
 */
void Vector::Add(CObject * pElement)
{
    if (!pElement)
    {
        assert(false);
        return;
    }
    assert(m_pType);
    assert(pElement->IsKindOf(m_pType));
    VectorElement *pNewEl = new VectorElement();
    pNewEl->m_pElement = pElement;
    pNewEl->m_pPrev = m_pTail;
    if (m_pTail)
        m_pTail->m_pNext = pNewEl;
    if (!m_pHead)
        m_pHead = pNewEl;
    m_pTail = pNewEl;
}

/** \brief add a whole vector to this vector
 *  \param pOldVector the old vector
 *
 * Every element of the old vector is seperately added to this vector using Vector::Add.
 */
void Vector::Add(Vector * pOldVector)
{
    if (!pOldVector)
        return;
    if (!pOldVector->m_pType->IsDerivedFrom(m_pType) ||
        (pOldVector->m_pType->m_sName != m_pType->m_sName))
        return;
    VectorElement *pCurrent;
    for (pCurrent = pOldVector->GetFirst(); pCurrent; pCurrent = pCurrent->GetNext())
    {
        if (pCurrent->GetElement())
            Add(pCurrent->GetElement()->Clone());
    }
}

/** \brief add a element at the head of the chain
 *  \param pElement the new member of the vector
 *
 * The new element is added to the head of the chain. Otherwise the functionality is the
 * sam as with Vector::Add.
 */
void Vector::AddHead(CObject * pElement)
{
    if (!pElement)
    {
        assert(false);
        return;
    }
    assert(pElement->IsKindOf(m_pType));
    VectorElement *pNewEl = new VectorElement();
    pNewEl->m_pElement = pElement;
    pNewEl->m_pNext = m_pHead;
    if (m_pHead)
        m_pHead->m_pPrev = pNewEl;
    if (!m_pTail)
        m_pTail = pNewEl;
    m_pHead = pNewEl;
}

/** \brief adds an element if it is not already in the vector
 *  \param pElement the element to add
 */
void Vector::AddUnique(CObject *pElement)
{
    if (!pElement)
        return;
    if (pElement)
        assert(pElement->IsKindOf(m_pType));
    VectorElement *pCurrent = GetFirst();
    while (pCurrent)
    {
        if (pCurrent->m_pElement == pElement)
            return; // do not add
        pCurrent = pCurrent->GetNext();
    }
    // no equal element found
    Add(pElement);
}

/** \brief remove an object
 *  \param pElement the object to remove
 *  \return true if the element was removed, false if not
 *
 * We have to search for the element explicitely, because if it is contained
 * in some other vector we cannot simple set the next and prev pointers, but have
 * to be sure it is ours. After the element has been removed from the chain the
 * VectorElement is deleted, but the object itself still exists. You have to take
 * care of it yourself.
 */
bool Vector::Remove(CObject * pElement)
{
    if (!pElement)
        return false;
    if (pElement)
        assert(pElement->IsKindOf(m_pType));
    VectorElement *pCurrent = GetFirst();
    while (pCurrent)
    {
        if (pCurrent->m_pElement == pElement)
        {
            if (pCurrent->m_pPrev)
                pCurrent->m_pPrev->m_pNext = pCurrent->m_pNext;
            if (pCurrent->m_pNext)
                pCurrent->m_pNext->m_pPrev = pCurrent->m_pPrev;
            if (m_pHead == pCurrent)
                m_pHead = pCurrent->m_pNext;
            if (m_pTail == pCurrent)
                m_pTail = pCurrent->m_pPrev;
            delete pCurrent;
            return true;
        }
        pCurrent = pCurrent->m_pNext;
    }
    return false;
}

/** \brief remove an object
 *  \param nIndex the index to remove the object from
 *  \return a reference to the object
 */
CObject *Vector::RemoveAt(int nIndex)
{
    VectorElement *pCurrent = GetFirst();
    CObject *pRet = 0;
    int nCurrent = 0;
    while (pCurrent)
    {
        if (nCurrent == nIndex)
        {
            if (pCurrent->m_pPrev)
                pCurrent->m_pPrev->m_pNext = pCurrent->m_pNext;
            if (pCurrent->m_pNext)
                pCurrent->m_pNext->m_pPrev = pCurrent->m_pPrev;
            if (m_pHead == pCurrent)
                m_pHead = pCurrent->m_pNext;
            if (m_pTail == pCurrent)
                m_pTail = pCurrent->m_pPrev;
            pRet = pCurrent->GetElement();
            delete pCurrent;
            return pRet;
        }
        pCurrent = pCurrent->GetNext();
        nCurrent++;
    }
    return 0;
}

/** \brief remove first object
 *  \return a reference to the object
 *
 * Used for fast deletion of when destroying the object. The first object is removed
 * from the vector and a reference to it is returned to the caller.
 */
CObject *Vector::RemoveFirst()
{
    VectorElement *pCurrent = GetFirst();
    if (!pCurrent)
        return 0;
    if (pCurrent->m_pNext)
        pCurrent->m_pNext->m_pPrev = 0;
    if (m_pTail == pCurrent)
        m_pTail = 0;
    m_pHead = pCurrent->m_pNext;
    CObject *pRet = pCurrent->GetElement();
    delete pCurrent;
    return pRet;
}

/** \brief returns the first element of the vector
 *  \return the first element of the vector
 */
VectorElement *Vector::GetFirst()
{
    return m_pHead;
}

/** \brief returns the last element of this vector
 *  \return the last element of this vector
 */
VectorElement *Vector::GetLast()
{
    return m_pTail;
}

/** \brief returns the size of the vector
 *  \return the size of the vector
 *
 * This function simple runs along the chain and counts the elements.
 */
int Vector::GetSize()
{
    int count = 0;
    VectorElement *pCurrent;
    for (pCurrent = GetFirst(); pCurrent; pCurrent = pCurrent->GetNext())
    {
        count++;
    }
    return count;
}

/** \brief creates a copy of this vector
 *  \return a copy of this vector
 */
Vector *Vector::Clone()
{
    return new Vector(*this);
}

/** \brief sets the parent pointer of its element
 *  \param pParent the new parent
 */
void Vector::SetParentOfElements(CObject * pParent)
{
    VectorElement *pIter;
    for (pIter = GetFirst(); pIter; pIter = pIter->GetNext())
    {
        if (pIter->GetElement())
            pIter->GetElement()->SetParent(pParent);
    }
}

/**	\brief empties the vector completely
 *
 * This function does not only remove the elements from the vector, but also deletes the object behind them.
 */
void Vector::DeleteAll()
{
    CObject *pO = 0;
    while (GetFirst())
    {
        pO = RemoveFirst();
        if (pO)
        {
            delete pO;
            pO = 0;
        }
    }
}

/**	\brief retrieves an element at a given position
 *	\param nIndex the position
 *	\return a reference to the element at the respective position or 0 if out of bounds
 */
CObject *Vector::GetAt(int nIndex)
{
    if (nIndex < 0)
        return 0;
    int nPos = 0;
    VectorElement *pIter = GetFirst();
    while (pIter)
    {
        if (nPos == nIndex)
            return pIter->GetElement();
        pIter = pIter->GetNext();
        nPos++;
    }
    return 0;
}

/**	\brief sets the element at a given index
 *	\param nIndex the position to set the new element to
 *	\param pObject the new element
 *	\return true if replace was successful
 *
 * We do not replace with 0 value. We do not take care of the object, which was
 * at this position before. So you have to save it before calling SetAt.
 */
bool Vector::SetAt(int nIndex, CObject * pObject)
{
    if (!pObject)
        return false;
    if (!pObject->IsKindOf(m_pType))
        return false;
    if ((nIndex < 0) || (nIndex >= GetSize()))
        return false;
    int nPos = 0;
    VectorElement *pIter = GetFirst();
    while ((pIter) && (nPos != nIndex))
        pIter = pIter->GetNext();
    pIter->m_pElement = pObject;
    return true;
}

/** \brief exchanges two elements of the vector
 *  \param pObject1 the first object
 *  \param pObject2 the second object
 *  \return true if successful
 *
 * The ides is to exchange elements, but keep the iterators valid.
 * Assume pObject1 is designated by iterator1 and pObject2 by iterator2.
 * After the exchange pObject1 will be referenced by iterator2 and pObject2
 * will be referenced by iterator1. The disadvantage of this function is, that
 * the caller may be aware of this behaviour, but eventual parallel accesses
 * may not be aware of this.
 *
 * This function is highly threading-unsafe!!!
 */
bool Vector::Exchange(CObject *pObject1, CObject *pObject2)
{
    // first test for 0 pointers
    if ((!pObject1) || (!pObject2))
        return false;
    // check types
    if (!pObject1->IsKindOf(m_pType) || !pObject2->IsKindOf(m_pType))
        return false;
    // check equality
    if (pObject1 == pObject2)
        return false;
    // now try to find the iterators
    VectorElement *pIter1 = GetFirst();
    while (pIter1)
    {
        if (pIter1->m_pElement == pObject1)
            break;
        pIter1 = pIter1->m_pNext;
    }
    if (!pIter1)
        return false;
    VectorElement *pIter2 = GetFirst();
    while (pIter2)
    {
        if (pIter2->m_pElement == pObject2)
            break;
        pIter2 = pIter2->GetNext();
    }
    if (!pIter2)
        return false;
    // exchange
    pIter1->m_pElement = pObject2;
    pIter2->m_pElement = pObject1;
    return true;
}
