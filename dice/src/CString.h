/**
 *	\file	dice/src/CString.h
 *	\brief	contains the declaration of the class String
 *
 *	\date	04/12/2002
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
#ifndef __DICE_STRING_H__
#define __DICE_STRING_H__

#include "defines.h"

/**	\class String
 *	\ingroup misc
 *	\brief class to encapsulate the string operations
 */
class String
{
// Constructor
  public:
	/** \brief the constructor for a string
	 *	It constructs an empty string
	 */
    String();
	/**	\brief a constructor for a string
	 *	\param src the source string
	 *	It copies the string from the source to the member and sets the size respectively.
	 */
    String(const char *src);
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
     String(const String & src);
	/**	\brief single character constructor
	 *	\param ch the single character
	 */
     String(char ch);

     virtual ~String();

// Operations
  public:
    void Empty();
    bool IsEmpty();
    int GetLength();
    char CharAt(int index) const;
    void SetAt(int index, char ch);

    // operators
    char operator [] (int index) const;
    // cast
    operator           const char *() const;
    // assignment
    const String & operator =(const String & src);
    const String & operator =(char ch);
    const String & operator =(const char *str);
    // concatenation
    virtual void Concat(const String & add);
    virtual void Concat(const char *add);
    virtual void Concat(char ch);
	virtual void Concat(int nValue);
	virtual void Concat(float fValue);
	virtual void Concat(double dValue);
	virtual void Concat(long double dValue);
    // have to be friend so we can use char and char* as first arguments
    friend String operator +(const String & s1, const String & s2);
    friend String operator +(const String & s1, char ch);
    friend String operator +(const String & s1, const char *str);
    friend String operator +(char ch, const String & s2);
    friend String operator +(const char *str, const String & s2);
    // assign + concat
    const String & operator +=(const String & src);
    const String & operator +=(char ch);
    const String & operator +=(const char *str);
	const String & operator +=(int nValue);
    // compare
    int Compare(const char *str) const;
    int CompareNoCase(const char *str) const;
    friend bool operator ==(const String & s1, const String & s2);
    friend bool operator ==(const String & s1, const char *s2);
    friend bool operator ==(const char *s1, const String & s2);
    friend bool operator !=(const String & s1, const String & s2);
    friend bool operator !=(const String & s1, const char *s2);
    friend bool operator !=(const char *s1, const String & s2);
    friend bool operator <(const String & s1, const String & s2);
    friend bool operator <(const String & s1, const char *s2);
    friend bool operator <(const char *s1, const String & s2);
    friend bool operator >(const String & s1, const String & s2);
    friend bool operator >(const String & s1, const char *s2);
    friend bool operator >(const char *s1, const String & s2);
    friend bool operator <=(const String & s1, const String & s2);
    friend bool operator <=(const String & s1, const char *s2);
    friend bool operator <=(const char *s1, const String & s2);
    friend bool operator >=(const String & s1, const String & s2);
    friend bool operator >=(const String & s1, const char *s2);
    friend bool operator >=(const char *s1, const String & s2);
    // extraction
    String Left(int count) const;
    String Right(int count) const;
    String Mid(int start) const;
    String Mid(int start, int count) const;
    String SpanIncluding(const char *include) const;
    String SpanExcluding(const char *exclude) const;
    // searching
    int Find(char ch) const;
    int Find(const char *str) const;
    int Find(char ch, int start) const;
    int Find(const char *str, int start) const;
    int ReverseFind(char ch) const;
    int ReverseFind(const char *str) const;
    int ReverseFind(char ch, int start) const;
    int ReverseFind(const char *str, int start) const;
    int FindOneOf(const char *str) const;
    int FindOneOf(const char *str, int start) const;
    // conversions
    void MakeUpper();
    void MakeLower();
    void MakeReverse();
    int Replace(char ch1, char ch2, int start = 0);
    int Replace(const char *str1, const char *s2, int start = 0);
    int ReplaceIncluding(const char *str1, char ch2);
    int ReplaceExcluding(const char *str1, char ch2);
    int Remove(char ch);
    int Insert(int index, char ch);
    int Insert(int index, const char *str);
    int Delete(int index, int count = 1);
    void TrimLeft();
    void TrimLeft(char ch);
    void TrimLeft(const char *str);
    void TrimRight();
    void TrimRight(char ch);
    void TrimRight(const char *str);
	int ToInt();
	float ToFloat();
	double ToDouble();
	long double ToLongDouble();

// Attributes
  protected:
	/**	\var int m_nSize
	 *	\brief the size of the string
	 */
    int m_nSize;
	/**	\var char *m_sString
	 *	\brief the real string
	 */
    char *m_sString;
};

#endif				// __DICE_STRING_H__
