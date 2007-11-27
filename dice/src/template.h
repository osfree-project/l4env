/**
 *  \file    dice/src/template.h
 *  \brief   contains the declaration of some templates
 *
 *  \date    08/10/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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
#ifndef __DICE_TEMPLATE_H__
#define __DICE_TEMPLATE_H__

#include "defines.h"
#include "Object.h"
#include <vector>
using std::vector;
#include <algorithm>
#include <iostream>

/** \class CCollection
 *  \brief extends the vector class with some convenience functions
 */
template<class T>
class CCollection : public std::vector<T*>
{
	/** \var CObject *m_pParent
	 *  \brief reference to parent of members of this collection
	 */
	CObject *m_pParent;

	/** \brief hidden default constructor */
	CCollection()
	{ }

protected:
	/** \brief set the parent of a member
	 *  \param pNew the member
	 */
	void SetParent(CObject * pNew)
	{
		if (!m_pParent)
			return;
		if (!pNew)
			return;
		pNew->SetParent(m_pParent);
	}

	/** \class SetParentCall
	 *  \brief functor to set the parent of a collection
	 */
	class SetParentCall
	{
		/** \var CCollection *c
		 *  \brief the collection to modify
		 */
		CCollection *c;
	public:
		/** \brief constructor
		 *  \param cc the collection to modify
		 */
		SetParentCall(CCollection *cc) : c(cc)
		{ }

		/** \brief operator invoked by iteration function
		 *  \param mem the member of collection to set the parent
		 */
		void operator() (T* mem)
		{ c->SetParent(mem); }
	};

public:
	/** \brief constructs new collection object
	 *  \param src the source vector (can be NULL)
	 *  \param pParent the parent of the elements (can be NULL)
	 */
	CCollection(vector<T*> *src,
		CObject *pParent)
		: m_pParent(pParent)
	{
		Add(src);
		Adopt(m_pParent);
	}

	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CCollection(CCollection<T>& src)
		: vector<T*>()
	{
		m_pParent = src.m_pParent;
		typename CCollection<T>::const_iterator i;
		for (i = src.begin();
			i != src.end();
			i++)
		{
			push_back((*i)->Clone());
		}
		Adopt(m_pParent);
	}

	/** \brief destoys collection
	 *
	 * This implementation will not delete any members.
	 */
	virtual ~CCollection()
	{
		while (!vector<T*>::empty())
		{
			/* only delete members if we are parent */
			if (m_pParent &&
				vector<T*>::back()->IsParent(m_pParent))
				delete vector<T*>::back();
			vector<T*>::pop_back();
		}
	}

	/** \brief add a new member and set its parent
	 *  \param pNew reference to new member, if NULL ignored
	 */
	void Add(T* pNew)
	{
		if (!pNew)
			return;
		vector<T*>::push_back(pNew);
		SetParent(dynamic_cast<CObject*>(pNew));
	}

	/** \brief adds a vector of new members
	 *  \param src the source vector
	 *
	 * Swaps the source vector with the internal members. This way all
	 * previous members are lost. Does not set the parent property of the
	 * members.
	 */
	void Add(vector<T*> *src)
	{
		if (src)
			vector<T*>::swap(*src);
	}

	/** \brief return reference to firt element in vector or NULL if empty
	 *  \return reference to first element in vector
	 */
	T* First()
	{
		if (vector<T*>::empty())
			return (T*)0;
		return vector<T*>::front();
	}

	/** \brief "adopt" all elements of the vector
	 *  \param pParent the new parent
	 */
	void Adopt(CObject *pParent)
	{
		m_pParent = pParent;

		for_each(vector<T*>::begin(), vector<T*>::end(),
			SetParentCall(this));
	}

	/** \brief remove an element from the collection
	 *  \param pRem the element to remove
	 */
	void Remove(T* pRem)
	{
		if (!pRem)
			return;
		typename vector<T*>::iterator i = std::find(vector<T*>::begin(),
			vector<T*>::end(), pRem);
		if (i == vector<T*>::end())
			return;
		erase(i);
	}
};


/** \class CSearchableCollection
 *  \brief extends the Collection class by a searchable argument
 */
template<class T, typename KeyT>
class CSearchableCollection : public CCollection<T>
{
	/** hidden implicit constructor */
	CSearchableCollection()
	{ }

public:
	/** \brief constructs collection from source vector
	 *  \param src the source vector to use
	 *  \param pParent the parent of the elements
	 */
	CSearchableCollection(vector<T*> *src, CObject *pParent)
		: CCollection<T>(src, pParent)
	{ }

	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CSearchableCollection(CSearchableCollection<T, KeyT>& src)
		: CCollection<T>(src)
	{ }

	/** \brief tries to find a member in the collection given a key
	 *  \param key the key to find the member
	 *  \param prev the previously found member
	 *  \return reference to the found member or NULL if not found
	 */
	T* Find(KeyT key, T* prev = 0)
	{
		typename vector<T*>::iterator i = vector<T*>::begin();
		// if prev, find it in vector (we can savely add one to result,
		// because prev _has_ to be a member and is thus before end())
		if (prev)
			i = std::find(vector<T*>::begin(), vector<T*>::end(), prev) + 1;
		// now we have previous member before iterator (or begin if no previous)
		// and we can start the search
		i = std::find_if(i, vector<T*>::end(), std::bind2nd(std::ptr_fun(Match), key));
		if (i != vector<T*>::end())
			return *i;
		return (T*)0;
	}

private:
	/** \brief alias function to call object's match function
	 *  \param o the object to use
	 *  \param k the key to find
	 *  \return true if object's Match function with k returns true
	 */
	static bool Match(T* o, KeyT k)
	{
		return o->Match(k);
	}
};

#endif /* __DICE_TEMPLATE_H__ */
