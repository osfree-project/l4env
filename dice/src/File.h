/**
 *	\file	dice/src/File.h 
 *	\brief	contains the declaration of the class CFile
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FILE_H__
#define __DICE_FILE_H__

#include "Object.h"
#include "CString.h"

#define MAX_INDENT	80 /**< the maximum possible indent */
#define STD_INDENT	04 /**< the standard indentation value */

/**	\class CFile
 *	\brief base class for all file classes
 */
class CFile : public CObject
{
DECLARE_DYNAMIC(CFile);
// Constructor
  public:
	/**	\enum FileStatus
	 *	\brief describes the type of the file
	 */
    enum FileStatus
    {
	Read = 1,			 /**< file is open for read */
	Write = 2			 /**< file is open for write */
    };

	/** the constructor for this class */
     CFile();
     virtual ~ CFile();

  protected:
	/** the copy constructor
	 *	\param src the source to copy from
	 */
     CFile(CFile & src);

// Operations
  public:
     virtual bool IsOpen();
    bool IsStoring();
    bool IsLoading();
    virtual String GetFileName();
    virtual void DecIndent(int by = STD_INDENT);
    virtual void IncIndent(int by = STD_INDENT);
    virtual void PrintIndent(char *fmt, ...);
    virtual void Print(char *fmt, ...);
    virtual bool Close();
    virtual bool Open(String sFileName, int nStatus);

  protected:
     virtual bool Open(int nStatus);

  protected:
	/**	\var String m_sFileName
	 *	\brief the file's name
	 */
     String m_sFileName;
	/**	\var int m_nIndent
	 *	\brief the current valid indent, when printing to the file
	 */
    int m_nIndent;
	/**	\var FILE* m_fCurrent
	 *	\brief the file handle
	 */
    FILE *m_fCurrent;
	/** \var m_nStatus
	 *	\brief write or read file
	 */
    int m_nStatus;
	/**	\var m_nLastIndent
	 *	\brief remembers last increment
	 */
    int m_nLastIndent;
};

#endif				// __DICE_FILE_H__
