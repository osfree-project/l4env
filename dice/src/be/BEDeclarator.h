/**
 *  \file    dice/src/be/BEDeclarator.h
 *  \brief   contains the declaration of the class CBEDeclarator
 *
 *  \date    01/15/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "template.h"
#include <vector>
#include <string>

class CFEDeclarator;
class CFEIdentifier;
class CFEArrayDeclarator;
class CFEEnumDeclarator;
class CFEExpression;

class CBEExpression;
class CBEFile;

class CBEDeclarator;

/** defines the maximum number if array bounds a declarator
 * might have and thus how many indices may be available for
 * a declarators.
 */
const int MAX_INDEX_NUMBER = 10;

class CDeclaratorStackLocation;

typedef std::vector<CDeclaratorStackLocation> CDeclStack;

/** \class CDeclaratorStackLocation
 *  \brief class to represent a stack location for the declarator stack
 *
 * This is a helper class for writing access to members of constructed types.
 * It is used to build a stack of declarators traversing down the hierarchy of
 * the constructed type.
 * So a struct variable 'a' with a member 'b', which is also a struct having a
 * member 'c' will generate a stack of CDeclaratorStackLocations for 'a', 'b',
 * and 'c'.
 *
 * The declarator stack location also contains information useful for arrays
 * (even multidimensional ones). This is the purpose of the index members.
 */
class CDeclaratorStackLocation
{
public:
	/** \brief constructor of this class
	 *  \param pDecl the declarator
	 */
	CDeclaratorStackLocation(CBEDeclarator *pDecl = 0)
	{
		pDeclarator = pDecl;
		for (int i=0; i<MAX_INDEX_NUMBER; i++)
		{
			nIndex[i] = -1;
			sIndex[i].erase(sIndex[i].begin(), sIndex[i].end());
		}
	};
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CDeclaratorStackLocation(CDeclaratorStackLocation* src)
	{
		for (int i=0; i<MAX_INDEX_NUMBER; i++)
		{
			nIndex[i] = src->nIndex[i];
			sIndex[i] = src->sIndex[i];
		}
		pDeclarator = src->pDeclarator;
	};
	/** \var CBEDeclarator *pDeclarator
	 *  \brief a reference to the declarator at this location
	 */
	CBEDeclarator *pDeclarator;
	/** \var int nIndex[MAX_INDEX_NUMBER]
	 *  \brief the current index of this declarator in an array \
	 *         (or -1 if no array)
	 */
	int nIndex[MAX_INDEX_NUMBER];
	/** \var std::string sIndex
	 *  \brief the name of the index variable if there is no fixed index
	 */
	std::string sIndex[MAX_INDEX_NUMBER];
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
		sIndex[nArrayDim].erase(sIndex[nArrayDim].begin(),
			sIndex[nArrayDim].end());
	}
	/** \brief sets the index variable of the stack location
	 *  \param sIdx the new stack location variable
	 *  \param nArrayDim the index where to add the element
	 */
	void SetIndex(std::string sIdx, int nArrayDim = 0)
	{
		if (sIdx.empty())
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

	static void WriteToString(std::string &sResult,
		CDeclStack* pStack, bool bUsePointer);
	static void Write(CBEFile& pFile,
		CDeclStack* pStack, bool bUsePointer);
};

/** \def DUMP_STACK(iter, stack, str)
 *  \brief dumps the declarator stack
 */
#define DUMP_STACK(iter, stack, str) \
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s stack is:\n", str); \
    CDeclStack::iterator iter = stack->begin(); \
    for (; iter != stack->end(); iter++) \
        CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s\n", \
	    iter->pDeclarator->GetName().c_str()); \
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "--end--\n");

/** \class CBEDeclarator
 *  \ingroup backend
 *  \brief the back-end declarator
 */
class CBEDeclarator : public CBEObject
{
	// Constructor
public:
	/** \brief constructor
	 */
	CBEDeclarator();
	virtual ~CBEDeclarator();

protected:
	/** \brief copy constructor
	 *  \param src the source to copy from
	 */
	CBEDeclarator(CBEDeclarator* src);

public:
	virtual CObject* Clone();

	virtual void WriteName(CBEFile& pFile);
	virtual void WriteNameToStr(std::string &str);
	virtual void WriteIndirectInitialization(CBEFile& pFile,
		bool bUsePointer);
	virtual void WriteIndirectInitializationMemory(CBEFile& pFile,
		bool bUsePointer);
	virtual void WriteIndirect(CBEFile& pFile, bool bUsePointer,
		bool bHasPointerType);
	virtual void WriteDeclaration(CBEFile& pFile);
	virtual void CreateBackEnd(std::string sName, int nStars);
	virtual void CreateBackEnd(CFEIdentifier * pFEIdentifier);

	virtual int GetSize();

	virtual int GetArrayDimensionCount();
	virtual bool IsArray();
	virtual int GetRemainingNumberOfArrayBounds(
		vector<CBEExpression*>::iterator iter);
	virtual void AddArrayBound(CBEExpression * pBound);
	virtual void RemoveArrayBound(CBEExpression *pBound);

	/** \brief only returns a reference to the internal name
	 *  \return the name of the declarator (without stars and such)
	 */
	std::string GetName()
	{ return m_sName; }
	/** \brief set the name of the declarator
	 *  \param sName the new name
	 */
	void SetName(std::string sName)
	{ m_sName = sName; }
	/** \brief matches the given nameto the internally stored name
	 *  \param sName the name to match against
	 *  \return true if names match
	 */
	bool Match(std::string sName)
	{ return m_sName == sName; }

	/** \brief returns the number of bitfields used by this declarator
	 *  \return the value of the member m_nBitfields
	 */
	int GetBitfields()
	{ return m_nBitfields; }

	/** \brief modifies the number of stars
	 *  \param nBy the number to add to m_nStars (if it is negative it dec)
	 *  \return the new number of stars
	 */
	void IncStars(int nBy)
	{ m_nStars += nBy; }
	/** \brief set the number of stars to a fixed value
	 *  \param nStars the new number of stars
	 *  \return the old number
	 */
	void SetStars(int nStars)
	{ m_nStars = nStars; }
	/** \brief returns the number of stars
	 *  \return the value of m_nStars
	 */
	int GetStars()
	{ return m_nStars; }
	/** \brief accessor for initial value
	 *  \return reference to initial value
	 */
	CBEExpression* GetInitialValue()
	{ return m_pInitialValue; }

	virtual int GetMaxSize();
	virtual void WriteCleanup(CBEFile& pFile, bool bUsePointer,
		bool bDeferred);

protected:
	virtual int GetEmptyArrayDims();
	virtual void CreateBackEndDecl(CFEDeclarator * pFEDeclarator);
	virtual void CreateBackEndArray(CFEArrayDeclarator * pFEArrayDeclarator);
	virtual void CreateBackEndEnum(CFEEnumDeclarator * pFEEnumDeclarator);
	virtual CBEExpression *GetArrayDimension(CFEExpression * pLower,
		CFEExpression * pUpper);
	virtual void WriteArray(CBEFile& pFile);
	virtual void WriteArrayIndirect(CBEFile& pFile);
	virtual void WriteEnum(CBEFile& pFile);

protected:
	/** \var std::string m_sName
	 *  \brief the name of the declarator
	 */
	std::string m_sName;
	/** \var int m_nStars
	 *  \brief the number of asterisks (indirection) of this declarator
	 */
	int m_nStars;
	/** \var int m_nBitfields
	 *  \brief the bitfield number (especially used with structs)
	 */
	int m_nBitfields;
	/** \var int m_nType
	 *  \brief defines the value in the union
	 */
	int m_nType;
	/** \var int m_nOldType
	 *  \brief contains a backup of the type value if a temporary array is \
	 *         established
	 */
	int m_nOldType;
	/** \var CBEExpression *m_pInitialValue
	 *  \brief contains a reference to the initail value of the enum declarator
	 */
	CBEExpression *m_pInitialValue;

public:
	/** \var CCollection<CBEExpression> m_Bounds
	 *  \brief contains the array bounds
	 */
	CCollection<CBEExpression> m_Bounds;
};

#endif   // !__DICE_BEDECLARATOR_H__

