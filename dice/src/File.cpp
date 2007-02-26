/**
 *    \file    dice/src/File.cpp
 *    \brief   contains the implementation of the class CFile
 *
 *    \date    07/05/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#include <string.h>

CFile::CFile()
{
    m_fCurrent = 0;
    m_nIndent = 0;
}

CFile::CFile(CFile & src)
{
    m_fCurrent = src.m_fCurrent;
    m_sFileName = src.m_sFileName;
    m_nIndent = src.m_nIndent;
}

/** because the object does not really contain anything, the destructor has
 * nothing to clean up 
 */
CFile::~CFile()
{
}

/**
 *    \brief opens a file with a specific type
 *    \param sFileName the file name
 *    \param nStatus whether file is open for Read or Write
 *    \return true if open was siccessful, false otherwise
 */
bool CFile::Open(string sFileName, int nStatus)
{
    if (m_fCurrent)
    {
  	return false;
    }
    if (sFileName.empty())
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
	    m_fCurrent = fopen(sFileName.c_str(), "r");
	}
	if (nStatus == Write)
	{
	    m_fCurrent = fopen(sFileName.c_str(), "w+");
	}
    }
    if (!m_fCurrent)
    {
     	return false;
    }
    if (m_sFileName.empty())
   	m_sFileName = sFileName;
    m_nIndent = 0;
    m_nStatus = nStatus;
    return true;
}

/**
 *    \brief opens a file with a specific type
 *    \param nStatus whether the file is open for Read or Write
 *    \return true if open was successful, false otherwise
 *
 * This implementation calls the other Open function using the member
 * m_sFileName as filename.  This can be used to set the filename beforehand
 * and then open a file later.
 */
bool CFile::Open(int nStatus)
{
    return Open(m_sFileName, nStatus);
}

/**
 *    \brief closes a file
 *    \return true if close was successful, false otherwise
 */
bool CFile::Close()
{
    if (fclose(m_fCurrent))
	return false;
    m_fCurrent = 0;
    return true;
}

/**
 *    \brief prints a line into the currently open file
 *    \param fmt the format string for the argument list
 *
 * This function starts to write at the current position in the file.
 */
void CFile::Print(const char *fmt, ...)
{
    if (!m_fCurrent)
	return;
    va_list args;
    va_start(args, fmt);
    VPrint(fmt, args);
    va_end(args);
}

/**
 *  \brief prints the line into the currently open file
 *  \param fmt the format string for the argument list
 *  \param args the argument list
 *
 * If the first character in the line is a '\t' we eat it and send the rest to
 * VPrintIndent.
 *
 * If the string contains a '\n' we print everything up to and including the
 * '\n', and restart VPrint with the rest.
 */
void CFile::VPrint(const char *fmt, va_list args)
{
    if (!m_fCurrent)
	return;
    // check if there is a string
    if (!fmt)
	return;
    // check for '\t' at beginning
    if (fmt[0] == '\t')
    {
	VPrintIndent(&fmt[1], args);
	return;
    }
    // check for '\n'
    char *p = strchr(fmt, '\n');
    if (p && *(++p))
    {
	int len = p-fmt;
	char *first = (char*)malloc(len+1);
	strncpy(first, fmt, len);
	first[len] = 0;
	// print
	vfprintf(m_fCurrent, first, args);
	// print rest
	VPrint(p, args);
	
	return;
    }
    // print
    vfprintf(m_fCurrent, fmt, args);
}

/**
 *  \brief prints the line into the currently open file
 *  \param str the string to write
 *
 * If the first character in the line is a '\t' we eat it and send the rest to
 * VPrintIndent.
 *
 * If the string contains a '\n' we print everything up to and including the
 * '\n', and restart VPrint with the rest.
 */
void CFile::Prints(const char *str)
{
    if (!m_fCurrent)
	return;
    // check if there is a string
    if (!str)
	return;
    // check for '\t' at beginning
    if (str[0] == '\t')
    {
	PrintIndent("%s", &str[1]);
	return;
    }
    // check for '\n'
    char *p = strchr(str, '\n');
    if (p && *(++p))
    {
	int len = p-str;
	char *first = (char*)malloc(len+1);
	strncpy(first, str, len);
	first[len] = 0;
	// print
	fprintf(m_fCurrent, first);
	// print rest
	Prints(p);
	
	return;
    }
    // print
    fprintf(m_fCurrent, str);
}

/**
 *    \brief prints a line into the currently open file
 *    \param fmt the format string for the argument list
 *
 * This function starts to write the number of identify characters into the
 * file and then starts to print the line.
 */
void CFile::PrintIndent(const char *fmt, ...)
{
    if (!m_fCurrent)
	return;
    // print
    va_list args;
    va_start(args, fmt);
    VPrintIndent(fmt, args);
    va_end(args);
}

/**
 *  \brief prints a line into the currently open file
 *  \param fmt the format string for the line to print
 *  \param args the arguments to the format string
 *
 * If there is a '\n' in the string and something behind it, then we print
 * everything up to and including the first '\n' and restart VPrint with the
 * rest.
 */
void CFile::VPrintIndent(const char *fmt, va_list args)
{
    if (!m_fCurrent)
	return;
    // print indent
    for (int i = 0; i < m_nIndent; i++)
	fprintf(m_fCurrent, " ");
    // check if there is a string
    if (!fmt)
	return;
    // check for '\n'
    char *p = strchr(fmt, '\n');
    if (p && *(++p))
    {
	int len = p-fmt;
	char *first = (char*)malloc(len+1);
	strncpy(first, fmt, len);
	first[len] = 0;
	// print
	vfprintf(m_fCurrent, first, args);
	// print rest
	VPrint(p, args);
	
	return;
    }
    // print
    vfprintf(m_fCurrent, fmt, args);
}

/**
 *    \brief increases the ident for this file
 *    \param by the number of characters, the ident should be increased.
 *
 * The standard value to increase the ident is specified in the constant
 * STD_INDENT.  If the ident reaches the values specified in MAX_IDENT it
 * ignores the ident increase.
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
 *    \brief decreases the ident
 *    \param by the number of character, by which the ident should be decreased
 *
 * The standard value for the decrement operation is STD_IDENT. If the ident
 * reaches zero (0) the operation ignores the decrement.  If by is -1 the
 * indent is decremented by the value of the last increment.
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
 *    \brief return the name of the file
 *    \return the name of the currently open file, 0 if no file is open
 */
string CFile::GetFileName()
{
    return m_sFileName;
}

/** test if the file is reading
 *    \return true if reading
 */
bool CFile::IsLoading()
{
    return (m_nStatus == Read);
}

/** test if the file is writing
 *    \return true if writing
 */
bool CFile::IsStoring()
{
    return (m_nStatus == Write);
}

/**    \brief test whether or not file is open
 *    \return true if file is open
 *
 * This function test the current file handle member (m_fCurrent). If it is
 * set to 0 the functionr eturns false.
 */
bool CFile::IsOpen()
{
    return (m_fCurrent != 0);
}

/** \brief flushes the content of the file stream to disk
 */
void CFile::Flush()
{
    if (m_fCurrent)
	fflush(m_fCurrent);
}

