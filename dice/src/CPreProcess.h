/**
 *    \file    dice/src/CPreProcess.h
 *    \brief   contains the declaration of the class CPreProcess
 *
 *    \date    Mon Jul 28 2003
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
#ifndef CPREPROCESS_H
#define CPREPROCESS_H

#include <string>
using namespace std;
#include "ProgramOptions.h" // needed for ProgramOptionType
#include <stdlib.h> // needed for malloc
#include <string.h> // needed for memset
#include "defines.h"

/** \struct _inc_bookmark_t
 *  \brief helper structure for the parser
 *
 * This structure contains information about include and
 * import statements. It is used later to associate line pre-
 * processor statements with the correct include/import statement.
 */
struct _inc_bookmark_t
{
    /** \var string *m_pFromFile
     *  \brief contains the filename in which the include/import statement appears
     */
    string *m_pFromFile;
    /** \var string *m_pFilename
     *  \brief contains the filename of the include/import statement
     */
    string *m_pFilename;
    /** \var int m_nLineNb
     *  \brief contains the line-number at which the statement appeared
     */
    int m_nLineNb;
    /** \var bool m_bImport
     *  \brief true if this is an import statement
     */
    bool m_bImport;
    /** \var bool m_bStandard
     *  \brief true if this is an standard include/import (the ones with < > )
     */
    bool m_bStandard;
    /** \var struct _inc_bookmark_t *m_pPrev
     *  \brief contains a pointer to the previous bookmark in the "stack"
     */
    struct _inc_bookmark_t *m_pPrev;
    /** \var struct _inc_bookmark_t *m_pNext
     *  \brief contains a pointer to the next bookmark in the "stack"
     */
    struct _inc_bookmark_t *m_pNext;
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
    if (tmp->m_pFilename != 0)
        delete tmp->m_pFilename;
    free(tmp);
}

/** \class CPreProcess
 *  \ingroup frontend
 *  \brief encapsulates the common pre-processor
 */
class CPreProcess
{
private:
    /** creates a new preprocessor object */
    CPreProcess();
public:
    ~CPreProcess();

    char ** GetCPPArguments(); // delete?
    int AddIncludePath(string sPath); // used by CCompiler
    int AddIncludePath(const char* sNewPath); // used by CCompiler
    void AddCPPArgument(string sNewArgument); // used by CCompiler
    void AddCPPArgument(const char* sNewArgument); // used by CCompiler
    bool SetCPP(const char* sCPP); // used by CCompiler
    FILE* PreProcess(string sFilename, bool bDefault, bool bVerbose);
    string GetCurrentIncludePath();
    string FindPathToFile(string sFilename, int nLineNb); // used by scanner
    bool AddInclude(string sFile, string sFromFile, int nLineNb, bool bImport, bool bStandard); // used by scanner
    string GetOriginalIncludeForFile(string sFilename, int nLineNb); // used by scanner
    bool IsStandardInclude(string sFilename, int nLineNb);
    FILE* OpenFile(string sName, bool bDefault, bool bVerbose, bool bIgnoreErrors = false);
    void SetOption(unsigned int nOptionAdd, unsigned int nOptionRemove = 0);

    static CPreProcess* GetPreProcessor(); // used by CCompiler, CParser

    inc_bookmark_t* GetFirstIncludeInFile(string sFilename); // used by CFEFile
    inc_bookmark_t* GetNextIncludeInFile(string sFilename, inc_bookmark_t* pPrev);

protected:
    void CPPErrorHandling();
    int ExecCPP(FILE *fInput, FILE* fOutput);
    bool TestCPP(const char* sCPP);
    char* CheckCPPforArguments(const char* sCPP);
    bool CheckName(string sPathToFile);
    inc_bookmark_t* FirstIncludeBookmark();
    inc_bookmark_t* PopIncludeBookmark();
    void AddIncludeBookmark(inc_bookmark_t* pNew);
    int FindLineNbOfInclude(string sFilename, string sFromFile);

    bool IsOptionSet(unsigned int nRawOption);

protected:
    /** \var CPreProcess *m_pPreProcessor
     *  \brief a reference to the preprocessor
     */
    static CPreProcess *m_pPreProcessor;
    /**    \var int m_nCPPArgCount
     *    \brief the number of cpp arguments
     */
    int m_nCPPArgCount;
    /**    \var char** m_sCPPArgs
     *    \brief the arguments for the cpp preprocessor
     */
    char **m_sCPPArgs;
    /** \var char* m_sCPPProgram
     *  \brief the program name of CPP
     */
    char* m_sCPPProgram;
    /** \var string m_sIncludePaths[MAX_INCLUDE_PATHS]
     *  \brief contains the include paths
     */
    string m_sIncludePaths[MAX_INCLUDE_PATHS];
    /** \var int m_nCurrentIncludePath
     *  \brief indicates the currently used include path
     */
    int m_nCurrentIncludePath;
    /** \var inc_bookmark_t *m_pBookmarkHead
     *  \brief references a list of include bookmarks
     */
    inc_bookmark_t *m_pBookmarkHead;
    /** \var inc_bookmark_t *m_pBookmarkTail
     *  \brief references a list of include bookmarks
     */
    inc_bookmark_t *m_pBookmarkTail;
    /**    \var unsigned int m_nOptions[PROGRAM_OPTION_GROUPS]
     *    \brief the options which are specified with the compiler call
     */
    unsigned int m_nOptions[PROGRAM_OPTION_GROUPS];
};

#endif /* !CPREPROCESS_H */
