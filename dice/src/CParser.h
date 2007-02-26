/**
 *	\file	dice/src/Parser.h
 *	\brief	contains the declaration of the class CParser
 *
 *	\date	Mon Jul 22 2002
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
#ifndef PARSER_H
#define PARSER_H

#include "CString.h"
#include <stdio.h>
#include <string.h> // needed for memset

class CFEFile;

//@{
/** helper functions to switch between input buffers */
void* SwitchToFileDce(FILE *fNewFile);
void RestoreBufferDce(void *buffer);

void* SwitchToFileCorba(FILE *fNewFile);
void RestoreBufferCorba(void *buffer);
//@}

/** \struct _buffer_t
 *  \brief helper structure for parser
 *
 * This structure contains the data needed for input buffers.
 */
struct _buffer_t
{
    /** \var void* buffer
     *  \brief is the pointer to the saved scan-buffer
     */
    void *buffer;
    /** \var String *pFilename
     *  \brief contains the filename of the file
     */
    String *pFilename;
    /** \var String *pPrevTopLevelFileName
     *  \brief contains the top level file-name of the previos level
     */
    String *pPrevTopLevelFileName;
    /** \var String *pPathname
     *  \brief contains the full path to the file
     */
    String *pPathname;
    /** \var int nLineNo
     *  \brief contains the line-number where the scan of the file was interrupted
     */
    int nLineNo;
    /** \var int c_inc
     *  \brief is 1 if it is a C header file, 0 if IDL header file
     */
    int c_inc;
    /** \var struct _buffer_t *pPrev
     *  \brief a pointer to the previous buffer in the "stack"
     */
    struct _buffer_t *pPrev;
    /** \var struct _buffer_t *pNext
     *  \brief a pointer to the next buffer in the "stack"
     *
     * We need a double linked list, because we do not only push and pop
     * from the stack, but also parse the stack back and forth
     */
    struct _buffer_t *pNext;
};

/** \var struct _buffer_t buffer_t
 *  \brief helper struct alias for parser
 */
typedef struct _buffer_t buffer_t;

/** \brief creates a new instance of buffer_t
 *  \return a reference to the new buffer
 */
inline buffer_t* new_buffer()
{
   buffer_t* tmp = (buffer_t*)malloc(sizeof(buffer_t));
   memset(tmp, 0, sizeof(buffer_t));
   return tmp;
}

/** \brief deletes the instance of the buffer
 *  \param tmp the buffer to delete
 */
inline void del_buffer(buffer_t *tmp)
{
   delete tmp->pFilename;
   delete tmp->pPathname;
   delete tmp->pPrevTopLevelFileName;
   free(tmp);
}

/** \struct _inc_bookmark_t
 *  \brief helper structure for the parser
 *
 * This structure contains information about include and
 * import statements. It is used later to associate line pre-
 * processor statements with the correct include/import statement.
 */
struct _inc_bookmark_t
{
    /** \var String *pFromFile
     *  \brief contains the filename in which the include/import statement appears
     */
    String *pFromFile;
    /** \var String *pFilename
     *  \brief contains the filename of the include/import statement
     */
    String *pFilename;
    /** \var int nLineNb
     *  \brief contains the line-number at which the statement appeared
     */
    int nLineNb;
    /** \var bool bImport
     *  \brief true if this is an import statement
     */
    bool bImport;
    /** \var bool bStandard
     *  \brief true if this is an standard include/import (the ones with < > )
     */
    bool bStandard;
    /** \var struct _inc_bookmark_t *pPrev
     *  \brief contains a pointer to the previous bookmark in the "stack"
     */
    struct _inc_bookmark_t *pPrev;
    /** \var struct _inc_bookmark_t *pNext
     *  \brief contains a pointer to the next bookmark in the "stack"
     */
    struct _inc_bookmark_t *pNext;
};

/** \var struct _inc_bookmark_t inc_bookmark_t
 *  \brief a helper structure for the parser
 */
typedef struct _inc_bookmark_t inc_bookmark_t;

/** \brief creates a new inc_bookmark structure
 *  \return a reference to a new bookmark structure
 */
inline inc_bookmark_t* new_include_bookmark()
{
    inc_bookmark_t *tmp = (inc_bookmark_t*)malloc(sizeof(inc_bookmark_t));
    memset(tmp, 0, sizeof(inc_bookmark_t));
    return tmp;
}

/** \brief deletes a bookmark structure
 *  \param tmp the bookmark structure to remove
 */
inline void del_include_bookmark(inc_bookmark_t* tmp)
{
    if (tmp->pFilename != 0)
        delete tmp->pFilename;
    free(tmp);
}

/** \class CParser
 *  \ingroup frontend
 *  \brief encapsulates the parser calls
 */
class CParser
{
public:
	/** creates a new parser object */
	CParser();
	virtual ~CParser();

	bool Parse(String sFilename, int nIDL, CFEFile *pFEFile, bool bVerbose);

public: // Public methods
	virtual void AddInclude(String sFile, String sFromFile, int nLineNb, bool bImport, bool bStandard);
	static CParser* GetCurrentParser();
	static void SetCurrentParser(CParser *pParser);
	virtual char ** GetCPPArguments();
	virtual int AddIncludePath(String sPath);
	virtual void AddCPPArgument(String sNewArgument);
	virtual bool Import(String sFilename);
	virtual bool EndImport();
	virtual String FindPathToFile(String sFilename, int nLineNb);
    virtual void AddCPPArgument(const char* sNewArgument);
    virtual int AddIncludePath(const char* sNewPath);
    virtual String GetOriginalIncludeForFile(String sFilename, int nLineNb);

protected: // Protected methods
    virtual void CPPErrorHandling();
	virtual int ExecCPP(FILE *fInput, FILE* fOutput);
	virtual bool PreProcess(String sFilename, bool bDefault);
	virtual void Verbose(char *sMsg, ...);
	virtual FILE* OpenFile(String sName, bool bDefault);
	virtual void RestoreLexContext();
	virtual void SaveLexContext();
	virtual buffer_t* PopBuffer();
	virtual void PushBuffer(buffer_t *pNew);
	virtual int FindLineNbOfInclude(String sFilename, String sFromFile);
	virtual buffer_t* CurrentBuffer();
	virtual inc_bookmark_t* FirstIncludeBookmark();
	virtual inc_bookmark_t* PopIncludeBookmark();
	virtual void PushIncludeBookmark(inc_bookmark_t* pNew);
	virtual bool IsStandardInclude(String sFilename, int nLineNb);
    int CheckName(String sPathToFile);
    virtual bool TestCPP();

protected: // Protected attributes
	/** \var CParser *m_pParent
	 *  \brief a parent to the calling parser
	 */
	CParser *m_pParent;
    /** \var bool m_bVerbose
     *  \brief true if we should print verbose output
     */
    bool m_bVerbose;
    /** \var int m_nIDLType
     *  \brief defines the IDL parser (DCE or CORBA)
     */
    int m_nIDLType;
    /** \var FILE* m_fInput;
     *  \brief the input file
     */
    FILE *m_fInput;
	/**	\var int m_nCPPArgCount
	 *	\brief the number of cpp arguments
	 */
    int m_nCPPArgCount;
	/**	\var char** m_sCPPArgs
	 *	\brief the arguments for the cpp preprocessor
	 */
    char **m_sCPPArgs;
    /** \var char* m_sCPPProgram
     *  \brief the program name of CPP
     */
    char* m_sCPPProgram;
	/** \var CParser *m_pCurrentParser
	 *  \brief a reference to the current paser
	 */
	static CParser* m_pCurrentParser;
    /** \var String m_sIncludePaths[MAX_INCLUDE_PATHS]
     *  \brief contains the include paths
     */
    String m_sIncludePaths[MAX_INCLUDE_PATHS];
    /** \var int m_nCurrentIncludePath
     *  \brief indicates the currently used include path
     */
    int m_nCurrentIncludePath;
    /** \var buffer_t *m_pBufferHead
     *  \brief references a list of lexer states
     */
    buffer_t *m_pBufferHead;
    /** \var buffer_t *m_pBufferTail
     *  \brief references a list of lexer states
     */
    buffer_t *m_pBufferTail;
    /** \var inc_bookmark_t *m_pBookmarkHead
     *  \brief references a list of include bookmarks
     */
    inc_bookmark_t *m_pBookmarkHead;
    /** \var inc_bookmark_t *m_pBookmarkTail
     *  \brief references a list of include bookmarks
     */
    inc_bookmark_t *m_pBookmarkTail;
};

#endif
