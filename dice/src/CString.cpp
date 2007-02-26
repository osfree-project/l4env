/**
 *	\file	dice/src/String.cpp
 *	\brief	contains the implementation of the class String
 *
 *	\date	04/12/2002
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

#include "CString.h"
#include "defines.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>

String::String()
{
    m_sString = 0;
    m_nSize = 0;
}

String::String(const String & src)
{
    if (src.m_sString)
    {
        m_sString = strdup(src.m_sString);
        m_nSize = src.m_nSize;
    }
    else
    {
        m_nSize = 0;
        m_sString = 0;
    }
}

/** cleans up the String */
String::~String()
{
    if (m_sString)
        free(m_sString);
    m_sString = 0;
    m_nSize = 0;
}

String::String(const char *src)
{
    if (src)
    {
		m_sString = strdup(src);
        m_nSize = strlen(m_sString);
    }
    else
    {
        m_sString = 0;
        m_nSize = 0;
    }
}

String::String(char ch)
{
    m_sString = (char *) malloc(2);
    m_sString[0] = ch;
    m_sString[1] = 0;
    m_nSize = 1;
}

/**	\brief calculates the string size
*	\return the value of m_nSize
*/
int String::GetLength()
{
    return m_nSize;
}

/**	\brief tests whether or not the string is empty
*	\return true if the string is empty, false if not
*
* A string is defined as empty if it has the length zero.
*/
bool String::IsEmpty()
{
    return (m_nSize == 0);
}

/**	\brief empties a string
*
* This function forces a string to be empty. It frees used memory (of internal string data)
* and sets the size to zero.
*/
void String::Empty()
{
    if (m_sString)
        free(m_sString);
    m_sString = 0;
    m_nSize = 0;
}

/**	\brief adds another string the existing string
*	\param add the string to add
*
* This concatenates the existings string with the given string parameter.
* The string parameters is added at the end of the string.
*/
void String::Concat(const String & add)
{
    if (add.m_nSize == 0)
	return;
    m_sString = (char *) realloc(m_sString, m_nSize + add.m_nSize + 1);	// terminating 0
    strcpy(m_sString + m_nSize, add.m_sString);
    m_nSize += add.m_nSize;
}

/**	\brief adds another string to the existing one
*	\param add the string to add
*/
void String::Concat(const char *add)
{
    if (!add)
        return;

    int nNew = strlen(add);
    m_sString = (char *) realloc(m_sString, m_nSize + nNew + 1);	// terminating 0
    strcpy(m_sString + m_nSize, add);
    m_nSize += nNew;
}

/**	\brief adds another string to the existing one
*	\param ch the character to add
*/
void String::Concat(char ch)
{
    m_sString = (char *) realloc(m_sString, m_nSize + 2);	// terminating 0
    m_sString[m_nSize++] = ch;
    m_sString[m_nSize] = 0;
}

/**	\brief retrieves a character from the string at a given position
*	\param index the position to look at
*	\return the character at this position or 0 if out of bounds
*
* If the index is out of bounds (less than zero and greater than GetLength()-1) than
* zero is returned. I have chosen zero, because most code tests on zero.
*/
char String::CharAt(int index) const
{
    if ((index < 0) || (index >= m_nSize))
	return 0;
    return m_sString[index];
}

/**	\brief retrieves a character from the string at agiven position
*	\param index the position to look at
*	\return the character at this position or 0 if out of bounds
*
* This implementation returns the value of CharAt(index).
*/
char String::operator [] (int index) const
{
    return CharAt(index);
}
/**	\brief sets a new character at a given position
*	\param index the position of the new character
*	\param ch the new character
*
* This functions sets the character at the given index in the string. The index is zero-based
* (means: starting with zero). If the index is out of bounds (less than zero or larger than
* GetLength()-1) the function will not insert the character.
*/
void String::SetAt(int index, char ch)
{
    if ((index < 0) || (index >= m_nSize))
        return;
    m_sString[index] = ch;
}

/**	\brief cast operator
*	\return a reference to the string member
*/
String::operator  const char *() const
{
    return m_sString;
}
/**	\brief assignment operator
*	\param src the source string
*/
const String & String::operator =(const String & src)
{
    Empty();
    m_nSize = src.m_nSize;
    if (src.m_sString)
        m_sString = strdup(src.m_sString);
    else
        m_sString = 0;
    return *this;
}

/**	\brief assignment operator
*	\param ch the first character of the new string
*/
const String & String::operator =(char ch)
{
    Empty();
    m_nSize = 1;
    m_sString = (char *) malloc(2);
    m_sString[0] = ch;
    m_sString[1] = 0;
    return *this;
}

/**	\brief assignment operator
*	\param str the source string used to initialize
*/
const String & String::operator =(const char *str)
{
    Empty();
    if (str)
    {
        m_nSize = strlen(str);
        m_sString = strdup(str);
    }
    return *this;
}

/**	\brief concatenates two strings
*	\param s1 the first string
*	\param s2 the second string
*	\return a temporary string containing the concatenation
*/
String operator +(const String & s1, const String & s2)
{
    String result(s1);
    result.Concat(s2);
    return result;
}

/**	\brief concatenates two strings
*	\param s1 the first string
*	\param ch a single character
*	\return a temporary string containing the concatenation
*/
String operator +(const String & s1, char ch)
{
    String result(s1);
    result.Concat(ch);
    return result;
}

/**	\brief concatenates two strings
*	\param s1 the first string
*	\param str the second string
*	\return a temporary string containing the concatenation
*/
String operator +(const String & s1, const char *str)
{
    String result(s1);
    result.Concat(str);
    return result;
}

/**	\brief concatenates two strings
*	\param ch a single character
*	\param s2 the second string
*	\return a temporary string containing the concatenation
*/
String operator +(char ch, const String & s2)
{
    String result(ch);
    result.Concat(s2);
    return result;
}

/**	\brief concatenates two strings
*	\param str the first string
*	\param s2 the second string
*	\return a temporary string containing the concatenation
*/
String operator +(const char *str, const String & s2)
{
    String result(str);
    result.Concat(s2);
    return result;
}

/**	\brief adds one string to the own string
*	\param src the string to add
*
* This implementation only calls Concat
*/
const String & String::operator +=(const String & src)
{
    Concat(src);
    return *this;
}

/**	\brief adds one string to the own string
*	\param ch the character to add
*
* This implementation only calls Concat
*/
const String & String::operator +=(char ch)
{
    Concat(ch);
    return *this;
}

/**	\brief adds one string to the own string
*	\param str the string to add
*
* This implementation only calls Concat
*/
const String & String::operator +=(const char *str)
{
    Concat(str);
    return *this;
}

/**	\brief compares antother string with this one
*	\param str the string to compare to
*	\return 0 if equal, see details for other return values
*
* This implementation uses the strcmp functions to compare two strings.
* We can use this function to compare String classes as well, because they
* are automatically casted to const char*.
*
* The comparison is lexically, the strings are compared character by
* character. If an unequal character is dicovered, the functions checks, which one
* has the smaller ASCII value. If the left hand string is the one containing the
* smaller ASCII value -1 is returned. If the right hand string is the one 1 is
* returned. If both string are equal up to the last character 0 is returned.
*/
int String::Compare(const char *str) const
{
    if ((!str) && (!m_sString))
        return 0;
    if (!str)
        return 1;
    if (!m_sString)
        return -1;
    return strcmp(m_sString, str);
}

/**	\brief compares two string case insensitive
*	\param str the string to compare to
*	\return 0 if strings are equal, see details for other return values
*
* This implementation uses stricmp to compare the given string with the internal
* one. The results are the same as stricmp would deliver.
*/
int String::CompareNoCase(const char *str) const
{
    if ((!str) && (!m_sString))
        return 0;
    if (!str)
        return 1;
    if (!m_sString)
        return -1;
    return strcasecmp(m_sString, str);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if equal
*/
bool operator ==(const String & s1, const String & s2)
{
    return (s1.Compare(s2) == 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if equal
*/
bool operator ==(const String & s1, const char *s2)
{
    return (s1.Compare(s2) == 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if equal
*/
bool operator ==(const char *s1, const String & s2)
{
    return (s2.Compare(s1) == 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if not equal
*/
bool operator !=(const String & s1, const String & s2)
{
    return (s1.Compare(s2));
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if not equal
*/
bool operator !=(const String & s1, const char *s2)
{
    return (s1.Compare(s2));
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if not equal
*/
bool operator !=(const char *s1, const String & s2)
{
    return (s2.Compare(s1));
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than s2
*/
bool operator <(const String & s1, const String & s2)
{
    return (s1.Compare(s2) < 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than s2
*/
bool operator <(const String & s1, const char *s2)
{
    return (s1.Compare(s2) < 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than s2
*/
bool operator <(const char *s1, const String & s2)
{
    return (s2.Compare(s1) >= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than s2
*/
bool operator >(const String & s1, const String & s2)
{
    return (s1.Compare(s2) > 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than s2
*/
bool operator >(const String & s1, const char *s2)
{
    return (s1.Compare(s2) > 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than s2
*/
bool operator >(const char *s1, const String & s2)
{
    return (s2.Compare(s1) <= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than or equal to s2
*/
bool operator <=(const String & s1, const String & s2)
{
    return (s1.Compare(s2) <= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than or equal to s2
*/
bool operator <=(const String & s1, const char *s2)
{
    return (s1.Compare(s2) <= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is less than or equal to s2
*/
bool operator <=(const char *s1, const String & s2)
{
    return (s2.Compare(s1) > 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than or equal to s2
*/
bool operator >=(const String & s1, const String & s2)
{
    return (s1.Compare(s2) >= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than or equal to s2
*/
bool operator >=(const String & s1, const char *s2)
{
    return (s1.Compare(s2) >= 0);
}

/**	\brief compare two string
*	\param s1 the first string
*	\param s2 the second string
*	\return true if s1 is greater than or equal to s2
*/
bool operator >=(const char *s1, const String & s2)
{
    return (s2.Compare(s1) < 0);
}

/**	\brief extracts the left part of a string
*	\param count the number of bytes to extract
*	\return the substring containing the left count characters
*/
String String::Left(int count) const
{
    String result;
    if (count < 0)
		return result;
    if (count > m_nSize)
		count = m_nSize;
    if (count == 0)
		return result;
    result.m_sString = (char *) malloc(count + 1);
    strncpy(result.m_sString, m_sString, count);
    result.m_sString[count] = 0;
    result.m_nSize = count;
    return result;
}

/**	\brief extracts the right part of a string
*	\param count the number of bytes to extract
*	\return the substring containing the right count characters
*/
String String::Right(int count) const
{
    String result;
    if (count < 0)
        return result;
    if (count > m_nSize)
        count = m_nSize;
    if (count == 0)
        return result;
    result.m_sString = (char *) malloc(count + 1);
    strcpy(result.m_sString, m_sString + (m_nSize - count));
    result.m_sString[count] = 0;
    result.m_nSize = count;
    return result;
}

/**	\brief extracts the mid part of a string
*	\param start the start of extraction
*	\return the substring containing the right characters starting with start
*/
String String::Mid(int start) const
{
    return Right(m_nSize - start);
}

/**	\brief extracts the mid part of a string
*	\param start the start of the extraction
*	\param count the number of bytes to extract (has to be greater than zero)
*	\return the substring containing the count characters starting at start
*/
String String::Mid(int start, int count) const
{
    String result;
    if (count < 0)
	return result;
    if (start < 0)
	start = 0;
    if (start > m_nSize)
	start = m_nSize;
    if (count > (m_nSize - start))
	count = m_nSize - start;
    if (count == 0)
	return result;
    result.m_sString = (char *) malloc(count + 1);
    strncpy(result.m_sString, m_sString + start, count);
    result.m_sString[count] = 0;
    result.m_nSize = count;
    return result;
}

/**	\brief creates a new string which does only include the characters specified
*	\param include contains the characters to use
*	\return a string which contains only the specified characters
*/
String String::SpanIncluding(const char *include) const
{
    ASSERT(false);
    return *this;
}

/**	\brief creates a new string which does not include the specified characters
*	\param exclude the characters to exclude from the new string
*	\return a new string which does not include the specified characters
*/
String String::SpanExcluding(const char *exclude) const
{
    ASSERT(false);
    return *this;
}

/**	\brief searches for a character in this string
*	\param ch the character to search for
*	\return the index where the character was found or -1 if not found.
*/
int String::Find(char ch) const
{
    return Find(ch, 0);
}

/**	\brief searches for a string in this string
*	\param str the string to search for
*	\return the index where the string was found or -1 if not found.
*/
int String::Find(const char *str) const
{
    return Find(str, 0);
}

/**	\brief searches for a character starting at a specific position
*	\param ch the character to search for
*	\param start the start index
*	\return the index where the character was found
*/
int String::Find(char ch, int start) const
{
    if (start < 0)
	start = 0;
    if (start >= m_nSize)
	return -1;
    for (int i = start; i < m_nSize; i++)
      {
	  if (m_sString[i] == ch)
	      return i;
      }
    return -1;
}

/**	\brief searches for a string starting at the specified position
*	\param str the string to search for
*	\param start the starting position
*	\return the index where the string was found
*/
int String::Find(const char *str, int start) const
{
    if (!str)
        return -1;
    if (start < 0)
        start = 0;
    int nStrLen = strlen(str);
    if (start >= (m_nSize - nStrLen))
        return -1;
    for (int i = start; i < (m_nSize - nStrLen); i++)
    {
        if (strncmp(&m_sString[i], str, nStrLen) == 0)
            return i;
    }
    return -1;
}

/**	\brief searches for a character starting at the end of the string
*	\param ch the character to search for
*	\return the index where the character was found
*/
int String::ReverseFind(char ch) const
{
    return ReverseFind(ch, m_nSize-1);
}

/**	\brief searches for a string starting at the end of the string
*	\param str the string to search for
*	\return the index where the string was found
*/
int String::ReverseFind(const char *str) const
{
    return ReverseFind(str, m_nSize-1);
}

/**	\brief searches for a character starting at the given position in reverse direction
*	\param ch the character to look for
*	\param start the start index of the search
*	\return the index where the character was found or -1 if not found
*
* The search direction is from back to front!
*/
int String::ReverseFind(char ch, int start) const
{
    if (start >= m_nSize)
        start = m_nSize - 1;
    if (start < 0)
        return -1;
    for (int i = start; i >= 0; i--)
    {
        if (m_sString[i] == ch)
            return i;
    }
    return -1;
}

/**	\brief searches for a string starting at the given position in reverse direction
*	\param str the string to search for
*	\param start the start of the search
*	\return the index where the string was found
*/
int String::ReverseFind(const char *str, int start) const
{
    if (!str)
        return -1;
    if (start >= m_nSize)
        start = m_nSize - 1;
    int nStrLen = strlen(str)-1;
    if (start < nStrLen)
        return -1;
    for (int i = start - nStrLen; i >= 0; i--)
    {
        if (strncmp(&m_sString[i], str, nStrLen) == 0)
            return i;
    }
    return -1;
}

/**	\brief searches for one of the characters in the given string
*	\param str the string containing the characters to search for
*	\return the index of the first matching character in the string or -1 if not found
*/
int String::FindOneOf(const char *str) const
{
    if (!str)
	return -1;
    String includes(str);
    for (int i = 0; i < includes.GetLength(); i++)
      {
	  int pos = Find(includes.CharAt(i));
	  if (pos >= 0)
	      return pos;
      }
    return -1;
}

/**	\brief searches for one of the characters in the given string
*	\param str the string containing the characters to search for
*	\param start the position where to start the search
*	\return the index of the first matching character in the string or -1 if not found
*/
int String::FindOneOf(const char *str, int start) const
{
    if (!str)
	return -1;
    String includes(str);
    for (int i = 0; i < includes.GetLength(); i++)
      {
	  int pos = Find(includes.CharAt(i), start);
	  if (pos >= 0)
	      return pos;
      }
    return -1;
}

/**	\brief makes a string upper case
 *
 * This function uses toupper() to convert every single character of a string.
 */
void String::MakeUpper()
{
    for (int i = 0; i < m_nSize; i++)
	m_sString[i] = toupper(m_sString[i]);
}

/**	\brief make a string lower case
 *
 * This implementation uses tolower() to convert every single character.
 */
void String::MakeLower()
{
    for (int i = 0; i < m_nSize; i++)
	m_sString[i] = tolower(m_sString[i]);
}

/** \brief makes a reverse string
 *
 * This implementation reverses the string, which means that the last character will be the first.
 */
void String::MakeReverse()
{
    if (!m_sString)
        return;
    char *sBackup = strdup(m_sString);
    for (int i = 0; i < m_nSize; i++)
        m_sString[i] = sBackup[m_nSize-i-1];
    free(sBackup);
}

/** \brief replaces one character with another
 *
 * All characters ch1 are replaced with ch2. The first
 */
int String::Replace(char ch1, char ch2)
{
    if (ch1 == ch2)
        return -1;
    if (!m_sString)
        return -1;
    int nFirstPos = -1;
    for (int i=0; i < m_nSize; i++)
    {
        if (m_sString[i] == ch1)
        {
            m_sString[i] = ch2;
            if (nFirstPos < 0)
                nFirstPos = i;
        }
    }
    return nFirstPos;
}

/** \brief replaces one string with another
 *  \param str1 the replace to search for
 *  \param s2 the string to replace with
 *  \return the first position to replace
 */
int String::Replace(const char *str1, const char *s2)
{
    ASSERT(false);
    return -1;
}

/** \brief search for characters defined by the first string and replace them with the second parameter
 *  \param str1 the string containing the characters to replace
 *  \param ch2 the character to be used instead
 *  \return the position where the first character was replaced
 */
int String::ReplaceIncluding(const char *str1, char ch2)
{
    ASSERT(false);
    return -1;
}

/**	\brief replace all characters, which do not belong to one of the str1 characters with ch2
 *	\param str1 the string containing the character to leave as they are
 *	\param ch2 the character to use instead (if not matching)
 *	\return replace the index of the first replaced character or-1 if none replaced
 */
int String::ReplaceExcluding(const char *str1, char ch2)
{
    int nFirstPos = -1;
    String sMatch(str1);
    for (int i = 0; i < m_nSize; i++)
      {
	  if (sMatch.Find(m_sString[i]) < 0)
	    {
		if (nFirstPos < 0)
		    nFirstPos = i;
		m_sString[i] = ch2;
	    }
      }
    return nFirstPos;
}

/** \brief removes a specific character from the string
 *  \param ch the character to remove
 *  \return the first position at which the character was removed
 *
 * Due to the removal of the character the string might shrink in size.
 */
int String::Remove(char ch)
{
    ASSERT(false);
    return -1;
}

/** \brief inserts a character at the specified position
 *  \param index the position at which the character should be inserted
 *  \param ch the character to be inserted
 *  \return the position at which the character was inserted or -1 if an error occurred.
 */
int String::Insert(int index, char ch)
{
    ASSERT(false);
    return -1;
}

/** \brief inserts a string into this string
 *  \param index the index to insert the string at
 *  \param str the string to insert
 *  \return the position where the string was inserted or -1 if error
 */
int String::Insert(int index, const char *str)
{
    ASSERT(false);
    return -1;
}

/** \brief deletes a number of characters starting from a given position
 *  \param index the position to start the deletion
 *  \param count the number of characters to remove
 *  \return the number of characters which have really been removed
 */
int String::Delete(int index, int count)
{
    ASSERT(false);
    return 0;
}

/** \brief removes white spaces from the left of the string
 */
void String::TrimLeft()
{
    ASSERT(false);
}

/** \brief remove a specified character from the left of the string
 *  \param ch the character to remove
 *
 * First we search for the first non-matching character. If it is the first,
 * we return. If its not, we copy the string starting at this position to a
 * temporary variable, free the old string and set the m_sString pointer to the
 * temporary string.
 */
void String::TrimLeft(char ch)
{
    if (!m_sString)
        return;
    if (ch == 0)
        return;
    char *sPos = m_sString;
    while (*sPos == ch) sPos++;
    // test if we skipped some of the ch characters
    if (sPos == m_sString)
        return;
    // check if the whole string was made of ch
    if (*sPos == 0)
    {
        Empty();
        return;
    }
    char *sTemp = strdup(sPos);
    free (m_sString);
    m_sString = sTemp;
    m_nSize = strlen(m_sString);
}

/** \brief removes a substring from the left of the string
 *  \param str the string to remove
 */
void String::TrimLeft(const char *str)
{
    ASSERT(false);
}

/** \brief removes white spaces from the right of the string
 */
void String::TrimRight()
{
    ASSERT(false);
}

/** \brief removes the specified character from the right side of the string
 *  \param ch the character to remove
 *
 * First we seek backwards through the string to find a character which is not
 * equal to the given ch. Then we set the position after that character to zero
 * and copy m_sString to a temporary string. We free the old m_sString, and set
 * m_sString to the location of the temporary string.
 */
void String::TrimRight(char ch)
{
    if (!m_sString)
        return;
    if (ch == 0)
        return;
    char *sPos = m_sString+m_nSize-1;
    while ((*sPos == ch) && (sPos != m_sString)) sPos--;
    // test if there were any characters at all
    if (sPos == (m_sString+m_nSize-1))
        return;
    // test if the whole string was made of ch
    if (sPos == m_sString)
    {
        Empty();
        return;
    }
    // set pos to zero, so we can copy
    *(sPos+1) = 0;
    char *sTemp = strdup(m_sString);
    free (m_sString);
    m_sString = sTemp;
    m_nSize = strlen(m_sString);
}

/** \brief remove the given substring from the right of the string
 *  \param str the string to be removed
 */
void String::TrimRight(const char *str)
{
    ASSERT(false);
}
