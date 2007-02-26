/**
 *    \file    dice/src/File.h
 *    \brief   contains the declaration of the class CFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FILE_H__
#define __DICE_FILE_H__

#include "Object.h"
#include <string>
using namespace std;

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#define MAX_INDENT    80 /**< the maximum possible indent */
#define STD_INDENT    04 /**< the standard indentation value */

/** \class CFile
 *  \ingroup base
 *  \brief base class for all file classes
 */
class CFile : public CObject
{
// Constructor
  public:
    /** \enum FileStatus
     *  \brief describes the type of the file
     */
    enum FileStatus
    {
        Read = 1,    /**< file is open for read */
        Write = 2    /**< file is open for write */
    };

    /** the constructor for this class */
    CFile();
    virtual ~ CFile();

  protected:
    /** the copy constructor
     *    \param src the source to copy from
     */
    CFile(CFile & src);

// Operations
  public:
     virtual bool IsOpen();
    bool IsStoring();
    bool IsLoading();
    virtual string GetFileName();
    void DecIndent(int by = STD_INDENT);
    void IncIndent(int by = STD_INDENT);
    int GetIndent()
    { return m_nIndent; }
    virtual void PrintIndent(const char *fmt, ...);
    virtual void Print(const char *fmt, ...);
    virtual void Prints(const char* str);
    virtual bool Close();
    virtual bool Open(string sFileName, int nStatus);
    virtual void Flush();

  protected:
     virtual bool Open(int nStatus);
     virtual void VPrint(const char* fmt, va_list args);
     virtual void VPrintIndent(const char* fmt, va_list args);

  protected:
    /**    \var string m_sFileName
     *    \brief the file's name
     */
    string m_sFileName;
    /**    \var int m_nIndent
     *    \brief the current valid indent, when printing to the file
     */
    int m_nIndent;
    /**    \var FILE* m_fCurrent
     *    \brief the file handle
     */
    FILE *m_fCurrent;
    /** \var m_nStatus
     *    \brief write or read file
     */
    int m_nStatus;
    /**    \var m_nLastIndent
     *    \brief remembers last increment
     */
    int m_nLastIndent;
};

/**    \brief writes a string to the file
 *    \param f the file to write to
 *     \param str the string to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const char * const str)
{
    if (str && (str[0] == '\t'))
        f.PrintIndent(&str[1]);
    else
        f.Prints(str);
    return f;
}

/**    \brief writes a string to the file
 *    \param f the file to write to
 *     \param str the string to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, string str)
{
    f.Print("%s", str.c_str());
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const int i)
{
    f.Print("%d", i);
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const unsigned int i)
{
    f.Print("%u", i);
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const long i)
{
    f.Print("%ld", i);
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const unsigned long i)
{
    f.Print("%lu", i);
    return f;
}

#if SIZEOF_LONG_LONG > 0

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const long long i)
{
    f.Print("%lld", i);
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const unsigned long long i)
{
    f.Print("%llu", i);
    return f;
}

#endif

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const short i)
{
    f.Print("%hd", i);
    return f;
}

/**    \brief writes an integer to the file
 *    \param f the file to write to
 *     \param i the integer to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const unsigned short i)
{
    f.Print("%hu", i);
    return f;
}

/**    \brief writes an double to the file
 *    \param f the file to write to
 *     \param i the double to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const double i)
{
    f.Print("%f", i);
    return f;
}

/**    \brief writes an double to the file
 *    \param f the file to write to
 *     \param i the double to write
 *    \return the File again
 */
inline CFile& operator << (CFile& f, const long double i)
{
    f.Print("%Lf", i);
    return f;
}

#endif                // __DICE_FILE_H__
