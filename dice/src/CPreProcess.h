/**
 *  \file    dice/src/CPreProcess.h
 *  \brief   contains the declaration of the class CPreProcess
 *
 *  \date    Mon Jul 28 2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include <vector>
#include "ProgramOptions.h" // needed for ProgramOptionType
#include "IncludeStatement.h"
#include "template.h"

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
    void AddCPPArgument(string sNewArgument); // used by CCompiler
    void AddCPPArgument(const char* sNewArgument); // used by CCompiler
    bool SetCPP(const char* sCPP); // used by CCompiler
    FILE* PreProcess(string sFilename, bool bDefault);
    string GetIncludePath(string sFilename);
    string FindPathToFile(string sFilename, unsigned int nLineNb); // used by scanner
    void AddIncludePath(string sPath);
    bool AddInclude(string sFile, string sFromFile, unsigned int nLineNb,
	bool bImport, bool bStandard); // used by scanner
    string GetOriginalIncludeForFile(string sFilename,
	unsigned int nLineNb); // used by scanner
    bool IsStandardInclude(string sFilename, unsigned int nLineNb);
    FILE* OpenFile(string sName, bool bDefault,
	bool bIgnoreErrors = false);

    string RemoveSlashes(string & s);

    static CPreProcess* GetPreProcessor(); // used by CCompiler, CParser

    vector<CIncludeStatement>::iterator GetFirstIncludeInFile(); // used by CFEFile
    CIncludeStatement* GetNextIncludeInFile(string sFilename,
	vector<CIncludeStatement>::iterator & iter);

protected:
    void CPPErrorHandling();
    int ExecCPP(FILE *fInput, FILE* fOutput);
    bool TestCPP(char* sCPP);
    char* CheckCPPforArguments(const char* sCPP);
    bool CheckName(string sPathToFile);
    int FindLineNbOfInclude(string sFilename, string sFromFile);

protected:
    /** \var CPreProcess *m_pPreProcessor
     *  \brief a reference to the preprocessor
     */
    static CPreProcess *m_pPreProcessor;
    /** \var int m_nCPPArgCount
     *  \brief the number of cpp arguments
     */
    int m_nCPPArgCount;
    /** \var char** m_sCPPArgs
     *  \brief the arguments for the cpp preprocessor
     */
    char **m_sCPPArgs;
    /** \var char* m_sCPPProgram
     *  \brief the program name of CPP
     */
    char* m_sCPPProgram;
    /** \var vector<string> m_vIncludePaths
     *  \brief contains the include paths
     */
    vector<string> m_vIncludePaths;
    /** \var int m_nCurrentIncludePath
     *  \brief indicates the currently used include path
     */
    int m_nCurrentIncludePath;
    /** \var vector<CIncludeStatement> m_vBookmarks
     *  \brief the list of all include bookmarks
     */
    vector<CIncludeStatement> m_vBookmarks;
    /** \var vector<CIncludeStatement> m_vOpenBookmarks
     *  \brief contains the files opened until now
     */
    vector<CIncludeStatement> m_vOpenBookmarks;
};

#endif /* !CPREPROCESS_H */
