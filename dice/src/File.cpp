/**
 *	\file	dice/src/File.cpp
 *	\brief	contains the implementation of the class CFile
 *
 *	\date	07/05/2001
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

#include "File.h"
#include <stdarg.h>

IMPLEMENT_DYNAMIC(CFile) CFile::CFile()
{
    m_fCurrent = 0;
    m_nIndent = 0;
    IMPLEMENT_DYNAMIC_BASE(CFile, CObject);
}

CFile::CFile(CFile & src)
{
    m_fCurrent = src.m_fCurrent;
    m_sFileName = src.m_sFileName;
    m_nIndent = src.m_nIndent;
    IMPLEMENT_DYNAMIC_BASE(CFile, CObject);
}

/** because the object does not really contain anything, the destructor has
 *	nothing to clean up */
CFile::~CFile()
{
}

/**
 *	\brief opens a file with a specific type
 *	\param sFileName the file name
 *	\param nStatus whether file is open for Read or Write
 *	\return true if open was siccessful, false otherwise
 */
bool CFile::Open(String sFileName, int nStatus)
{
    if (m_fCurrent)
      {
	  return false;
      }
    if (sFileName.IsEmpty())
      {
	  if (nStatus == Read)
	    {
		m_fCurrent = stdin;
	    }
	  if (nStatus == Write)
	    {
		m_fCurrent = stdout;
	    }
      }
    else
      {
	  if (nStatus == Read)
	    {
		m_fCurrent = fopen((const char *) sFileName, "r");
	    }
	  if (nStatus == Write)
	    {
		m_fCurrent = fopen((const char *) sFileName, "w+");
	    }
      }
    if (!m_fCurrent)
      {
	  return false;
      }
    m_sFileName = sFileName;
    m_nIndent = 0;
    m_nStatus = nStatus;
    return true;
}

/**
 *	\brief opens a file with a specific type
 *	\param nStatus whether the file is open for Read or Write
 *	\return true if open was successful, false otherwise
 *
 * This implementation calls the other Open function using the member m_sFileName as filename.
 * This can be used to set the filename beforehand and then open a file later.
 */
bool CFile::Open(int nStatus)
{
    return Open(m_sFileName, nStatus);
}

/**
 *	\brief closes a file
 *	\return true if close was successful, false otherwise
 */
bool CFile::Close()
{
    if (fclose(m_fCurrent))
	return false;
    m_fCurrent = 0;
    m_sFileName.Empty();
    return true;
}

/**
 *	\brief prints a line into the currently open file
 *	\param fmt the format string for the argument list
 *
 * This function starts to write at the current position in the file.
 */
void CFile::Print(char *fmt, ...)
{
    if (!m_fCurrent)
	return;
    va_list args;
    va_start(args, fmt);
    vfprintf(m_fCurrent, fmt, args);
    va_end(args);
}

/**
 *	\brief prints a line into the currently open file
 *	\param fmt the format string for the argument list
 *
 * This function starts to write the number of identify characters into the file
 * and then starts to print the line.
 */
void CFile::PrintIndent(char *fmt, ...)
{
    if (!m_fCurrent)
	return;
    // print indent
    for (int i = 0; i < m_nIndent; i++)
	fprintf(m_fCurrent, " ");
    // print
    va_list args;
    va_start(args, fmt);
    vfprintf(m_fCurrent, fmt, args);
    va_end(args);
}

/**
 *	\brief increases the ident for this file
 *	\param by the number of characters, the ident should be increased.
 *
 * The standard value to increase the ident is specified in the constant STD_INDENT.
 * If the ident reaches the values specified in MAX_IDENT it ignores the ident increase.
 */
void CFile::IncIndent(int by)
{
    if ((m_nIndent + by) <= MAX_INDENT)
      {
	  m_nIndent += by;
	  m_nLastIndent = by;
      }
}

/**
 *	\brief decreases the ident
 *	\param by the number of character, by which the ident should be decreased
 *
 * The standard value for the decrement operation is STD_IDENT. If the ident reaches zero
 * (0) the operation ignores the decrement.
 * If by is -1 the indent is decremented by the value of the last increment.
 */
void CFile::DecIndent(int by)
{
    if (by == -1)
      {
	  by = m_nLastIndent;
	  m_nLastIndent = 0;
      }

    if (m_nIndent > by)
	m_nIndent -= by;
    else
	m_nIndent = 0;
}

/**
 *	\brief return the name of the file
 *	\return the name of the currently open file, 0 if no file is open
 */
String CFile::GetFileName()
{
    return m_sFileName;
}

/** test if the file is reading
 *	\return true if reading
 */
bool CFile::IsLoading()
{
    return (m_nStatus == Read);
}

/** test if the file is writing
 *	\return true if writing
 */
bool CFile::IsStoring()
{
    return (m_nStatus == Write);
}

/**	\brief test whether or not file is open
 *	\return true if file is open
 *
 * This function test the current file handle member (m_fCurrent). If it is set to 0 the functionr eturns false.
 */
bool CFile::IsOpen()
{
    return (m_fCurrent != 0);
}
