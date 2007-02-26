/**
 *    \file    dice/src/CPreProcess.cpp
 *    \brief   contains the implementation of the class CPreProcess
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "CPreProcess.h"
#include "CParser.h"
#include "Compiler.h"
#include <errno.h> // needed for errno
#include <unistd.h> // needed for pipe
#include <sys/wait.h> // needed for waitpid
#include <sys/types.h> // needed for waitpid
#include <sys/stat.h> // needed for open
#include <fcntl.h> // needed for open
#include "fe/FEFile.h"

//@{
/** globale pre-processor variables and function */
extern FILE *incin,*incout;
extern int inclex();
//@}

//@{
/** some external config variables */
extern const char* dice_configure_gcc;
extern const char* dice_compile_gcc;
//@}

/** the name of the input file */
extern string sInFileName;
/** the name of the include path */
extern string sInPathName;
/** back up name of the top level input file - we need this when scanning included files */
extern string sTopLevelInFileName;

/** a reference to the currently parsed file */

/** the current line number (relative to the current file) */
extern int gLineNumber;

CPreProcess *CPreProcess::m_pPreProcessor = 0;

CPreProcess::CPreProcess()
{
    m_sCPPArgs = 0;
    m_nCPPArgCount = 0;
    char *sCPP = getenv("CXX");
    if (!sCPP)
        sCPP = getenv("CC");
    if (sCPP && (strlen(sCPP) > 0))
      m_sCPPProgram = strdup(sCPP);
    else if (TestCPP("gcc"))
      m_sCPPProgram = strdup("gcc");
    else if (dice_configure_gcc)
      m_sCPPProgram = strdup(dice_configure_gcc);
    else if (dice_compile_gcc)
      m_sCPPProgram = strdup(dice_compile_gcc);
    else
      m_sCPPProgram = strdup("gcc");
    char* sC = CheckCPPforArguments(m_sCPPProgram);
    if (m_sCPPProgram)
        free(m_sCPPProgram);
    m_sCPPProgram = sC;
    // 1. argument: "-E"
    AddCPPArgument("-E");
    // to allow distinction between GCC's CPP and DICE's CPP invocation
    AddCPPArgument("-DDICE");
    //AddCPPArgument("-P");
    for (int i=0; i<MAX_INCLUDE_PATHS; i++)
        m_sIncludePaths[i] = "";
    m_nCurrentIncludePath = -1;
    m_pBookmarkHead = m_pBookmarkTail = 0;
    for (int i=0; i<PROGRAM_OPTION_GROUPS; i++)
        m_nOptions[i] = 0;
}

/** destroys the preprocess object */
CPreProcess::~CPreProcess()
{
    for (int i=0; i<m_nCPPArgCount; i++)
    {
        if (m_sCPPArgs[i])
            free(m_sCPPArgs[i]);
        m_sCPPArgs[i] = 0;
    }
    if (m_sCPPProgram)
        free(m_sCPPProgram);
    inc_bookmark_t *bookmark;
    while ((bookmark = PopIncludeBookmark()) != 0)
    {
        if (bookmark->m_pFromFile)
            delete bookmark->m_pFromFile;
        if (bookmark->m_pFilename)
            delete bookmark->m_pFilename;
        delete bookmark;
    }
    m_pPreProcessor = 0;
}

/** \brief returns a reference to the preprocessor
 *  \return a reference to the preprocessor
 *
 * We want the preprocessor to be a singleton. Therefore we use this
 * function to obtain a reference to the ONE preprocessor (constructor
 * is private). If the preprocessor is not create yet, we do so
 */
CPreProcess *CPreProcess::GetPreProcessor()
{
    if (!m_pPreProcessor)
        m_pPreProcessor = new CPreProcess();
    return m_pPreProcessor;
}

/** \brief allows access to the gcc arguments
 *  \return a reference to the arguments
 */
char ** CPreProcess::GetCPPArguments()
{
    return m_sCPPArgs;
}

/** \brief adds a include path
 *  \param sPath the path to add
 *  \return the index of the new path, or -1 if max limit reached
 */
int CPreProcess::AddIncludePath(string sPath)
{
    // search for last
    int nCurrent = 0;
    while (!m_sIncludePaths[nCurrent].empty()) nCurrent++;
    if (nCurrent < MAX_INCLUDE_PATHS)
    {
        // add trailing slash if not present
        if (sPath[sPath.length()-1] != '/')
            sPath += "/";
        m_sIncludePaths[nCurrent] = sPath;
        return nCurrent;
    }
    return -1;
}

/** \brief adds a include path
 *  \param sNewPath the path to add
 *  \return the index of the new path, or -1 if max limit reached
 */
int CPreProcess::AddIncludePath(const char* sNewPath)
{
    return AddIncludePath(string(sNewPath));
}

/** \brief adds another C pre-processor argument
 *  \param sNewArgument the new argument
 */
void CPreProcess::AddCPPArgument(string sNewArgument)
{
    if (sNewArgument.empty())
        return;
    AddCPPArgument(sNewArgument.c_str());
}

/** \brief adds another C pre-processor argument
 *  \param sNewArgument the new argument
 */
void CPreProcess::AddCPPArgument(const char* sNewArgument)
{
    if (!sNewArgument)
        return;
    if (!m_sCPPArgs)
        m_nCPPArgCount = 2;
    else
        m_nCPPArgCount++;
    m_sCPPArgs = (char **) realloc(m_sCPPArgs, m_nCPPArgCount * sizeof(char *));
    assert(m_sCPPArgs);
    m_sCPPArgs[m_nCPPArgCount - 2] = strdup(sNewArgument);
    m_sCPPArgs[m_nCPPArgCount - 1] = 0;
}

/** \brief explicetly sets the CPP
 *  \param sCPP the name of the new CPP
 *
 * Since this is always called after the constructor, we savely
 * override the configured CPP. This can be used to specify a
 * different CPP at runtime.
 */
bool CPreProcess::SetCPP(const char* sCPP)
{
    if (!sCPP)
        return false;
    char *sC = CheckCPPforArguments(sCPP);
    if (!TestCPP(sC))
        return false;
    if (m_sCPPProgram)
        free(m_sCPPProgram);
    m_sCPPProgram = sC;
    return true;
}

/** \brief does the error recognition and handling if CPP failed
 *
 * This function is called in the child process and only if CPP could not be
 * started.  The child process will return with value 3 if something went
 * wrong.
 */
void CPreProcess::CPPErrorHandling()
{
    char *s = strerror(errno);
    if (s)
        CCompiler::Error("execvp(\"%s\", cpp_args) returned: %s\n", m_sCPPProgram, s);
    else
        CCompiler::Error("execvp(\"%s\", cpp_args) returned an unknown error\n", m_sCPPProgram);
}

/** \brief runs the C pre-processor
 *  \param fInput the input file
 *  \param fOutput the output file
 *  \return an error code if something went wrong, zero (0) otherwise
 */
int CPreProcess::ExecCPP(FILE *fInput, FILE* fOutput)
{
    if (!m_sCPPProgram ||
        (strlen(m_sCPPProgram) == 0))
    {
        CCompiler::Error("No preprocessor (gcc or cpp) set.\n");
        return 3;
    }

    int pipes[2];
    if (pipe(pipes) == -1)
        return 1;

    int pid = fork();
    int status;
    if (pid == -1)
        return 2;

    // in child process call cpp
    if (pid == 0)
    {
        close(pipes[0]);
        // adjust stdin
        fclose(stdin);
        dup(fileno(fInput));
        // adjust stdout
        fclose(stdout);
        dup(fileno(fOutput));
        // child -> run cpp
        // last argument: the file (stdin)
        // function sets last argument always to 0
        AddCPPArgument("-");
        // reallocate cpp args to add the program itself as first argument
        char **sArgs = (char **) malloc((m_nCPPArgCount + 1) * sizeof(char *));
        sArgs[0] = m_sCPPProgram;
        for (int i=0; i<m_nCPPArgCount; i++)
            sArgs[i+1] = m_sCPPArgs[i];
        execvp(m_sCPPProgram, sArgs);
        CPPErrorHandling();
        return 3;
    }
    // parent -> wait for cpp
    waitpid(pid, &status, 0);

    return status;
}

/** \brief pre-process the file
 *  \param sFilename the filename of the file to pre-process
 *  \param bDefault true if the file to pre-process is a default file
 *  \param bVerbose true if the preprocessor should print verboe output
 *  \return true if successful
 *
 * First we scan for include/import, and then we run CPP.
 */
FILE* CPreProcess::PreProcess(string sFilename, bool bDefault, bool bVerbose)
{
    FILE *fInput = OpenFile(sFilename, bDefault, bVerbose);
    if (!fInput)
        return 0; // open file already printed error message

    sInFileName = sFilename;
    if (sInFileName.empty())
        sInFileName = "<stdin>";
    // we add the current path to the file-name so it conformes to the stored file-name
    // generated by Gcc
    if (m_nCurrentIncludePath >= 0)
        sTopLevelInFileName = m_sIncludePaths[m_nCurrentIncludePath] + sFilename;
    else
        sTopLevelInFileName = sFilename;

    if (bVerbose)
        printf("Start preprocessing input file (\"%s\") ...\n", sTopLevelInFileName.c_str());
    // turn debugging on
    if (bVerbose)
        nGlobalDebug++;
    // search for import and include statements
    FILE *fOutput;
    string s, sBase;
    if (IsOptionSet(PROGRAM_KEEP_TMP_FILES))
    {
        sBase = sFilename.substr(sFilename.rfind('/')-1);
        s = "temp1-" +  sBase;
        fOutput = fopen(s.c_str(), "w+");
    }
    else
        fOutput = tmpfile();
    if (!fOutput)
    {
        fprintf(stderr, "could not create temporary file\n");
        if (fInput != 0)
            fclose(fInput);
        return 0;
    }

    // check if parser has to contribute something
    CParser *pParser = CParser::GetCurrentParser();
    pParser->PrepareEnvironment(sFilename, fInput, fOutput);
    // search fInput for import statements
    incin = fInput;
    incout = fOutput;
    rewind(incin);
    // set start of file statement
    fprintf(incout, "#line 1 \"%s\"\n", sInFileName.c_str());
    // set line number count to start
    gLineNumber = 1;
    inclex();

    // turn debugging off
    if (bVerbose)
        nGlobalDebug--;
    // close input of preprocess
    if (!sFilename.empty())    // _not_ stdin
        fclose(fInput);

    // now all includes and imports are scanned

    // get new temp file
    if (IsOptionSet(PROGRAM_KEEP_TMP_FILES))
    {
        s = "temp2-" + sBase;
        fInput = fopen(s.c_str(), "w+");
    }
    else
        fInput = tmpfile();
    if (!fInput)
    {
        fprintf(stderr, "could not create temporary file\n");
        if (fOutput != 0)
            fclose(fOutput);
        return 0;
    }
    // set input file handle to beginning of file
    rewind(fOutput);

    // turn verboseness of CPP on
    if (bVerbose)
    {
        AddCPPArgument("-v");
        AddCPPArgument("-H");
    }

    // run cpp preprocessor
    int iRet;
    if ((iRet = ExecCPP(fOutput, fInput)) > 0)
    {
        fprintf(stderr, "could not preprocess input file \"%s\" (returned %d).\n", sInFileName.c_str(), iRet);
        return 0;
    }
    // close input file of cpp scanner
    fclose(fOutput);

    if (bVerbose)
        printf("... finished preprocessing input file.\n");

    // set input to beginning of file again
    fseek(fInput, 0, SEEK_SET);

    if (!fInput)
        CCompiler::Error("No input file.\n");

    return fInput;
}

/** \brief tries to open a file (and use the search paths)
 *  \param sName the name of the file
 *  \param bDefault true if the file is a deafult file ('<' file '>')
 *  \param bVerbose true if the preprocessor should produce verbose output
 *  \param bIgnoreErrors true if this function should not print any errors, but return a 0 handle
 *  \return a file descriptior if file found
 */
FILE* CPreProcess::OpenFile(string sName, bool bDefault, bool bVerbose, bool bIgnoreErrors)
{
    string sCurPath;
    if (m_nCurrentIncludePath < 0)
        sCurPath = "";
    else
        sCurPath = m_sIncludePaths[m_nCurrentIncludePath];
    /* try to open included file */
    if (bVerbose)
    {
        if (sCurPath.empty())
            printf("try to open include \"%s\"\n",sName.c_str());
        else
            printf("try to open include \"%s\" in path \"%s\"\n",sName.c_str(),sCurPath.c_str());
    }
    FILE *fReturn = 0;
    if (sCurPath.empty() || bDefault)
    {
        // check if this is a file
        if (CheckName(sName))
            fReturn = fopen(sName.c_str(), "r");
        else
            fReturn = NULL;
    }
    else
    {
        string s = sCurPath + sName;
        if (CheckName(s))
            fReturn = fopen(s.c_str(), "r");
        else
            fReturn = NULL;
    }
    if (fReturn && bVerbose)
        printf("success\n");

    if (!fReturn)
    {
        /* not in current directory -> search paths */
        int i = 0;
        while (!fReturn && (i < MAX_INCLUDE_PATHS))
        {
            if (!(m_sIncludePaths[i].empty()))
            {
                if (bVerbose)
                    printf("Search file in path \"%s\".\n", m_sIncludePaths[i].c_str());
                string s = m_sIncludePaths[i];
                if (s[s.length()] != '/')
                    s += "/";
                s += sName;
                if (CheckName(s))
                {
                    m_nCurrentIncludePath = i;
                    if (bVerbose)
                        printf("try to open with path %s\n",s.c_str());
                    // it's there
                    fReturn = fopen(s.c_str(), "r");
                }
            }
            i++;
        }
    }
    else
        m_nCurrentIncludePath = -1;

    if (bVerbose && (m_nCurrentIncludePath >= 0) && fReturn)
        printf("Found file \"%s\" in path \"%s\".\n", sName.c_str(), m_sIncludePaths[m_nCurrentIncludePath].c_str());

    if (!fReturn && !bIgnoreErrors)
    {
        if (bVerbose)
            printf("Couldn't find file\n");
        CFEFile *pCurFile = CParser::GetCurrentFile();
        if (!pCurFile)
        {
            fprintf(stderr, "dice: %s: No such file or directory.\n", sName.c_str());
            return 0;
        }
        /* if not open by now, couldn't find file */
        if (pCurFile->GetParent())
        {
            // run to top
            CFEFile *pFile = pCurFile->GetSpecificParent<CFEFile>(1); // returns this, if it is the file itself
            vector<CFEFile*> vStack;
            while (pFile)
            {
                vStack.insert(vStack.begin(), pFile);
                pFile = pFile->GetSpecificParent<CFEFile>(1); // returns this, if it is the file itself
            }
            // down in line
            if (vStack.size() > 1)
                fprintf(stderr, "In file included ");
            vector<CFEFile*>::iterator iter = vStack.begin();
            while (iter != vStack.end())
            {
                CFEFile *pFEFile = *iter;
                // get line number
                int nLine = 1;
                string sFileName = pFEFile->GetFullFileName();
                if (sFileName.empty())
                    sFileName = sTopLevelInFileName;
                if (pFEFile->GetParent() != 0)
                    nLine = FindLineNbOfInclude(sFileName, ((CFEFile*)(pFEFile->GetParent()))->GetFullFileName());
                else
                    nLine = FindLineNbOfInclude(sFileName, pFEFile->GetFullFileName());
                fprintf(stderr, "from %s:%d", sFileName.c_str(), nLine);
                if (iter+1 != vStack.end())
                    fprintf(stderr, ",\n                 ");
                else
                    fprintf(stderr, ":\n");
                iter++;
            }
        }
        string sFileName = pCurFile->GetFullFileName();
        if (sFileName.empty())
            sFileName = sTopLevelInFileName;
        int nLine = FindLineNbOfInclude(sName, sFileName);
        fprintf(stderr, "%s:%d: %s: No such file or directory.\n", sFileName.c_str(), nLine, sName.c_str());
    }
    if (!fReturn)
        return 0;

    if (m_nCurrentIncludePath >= 0)
    {
        if (!m_sIncludePaths[m_nCurrentIncludePath].empty())
        {
            if (m_sIncludePaths[m_nCurrentIncludePath][m_sIncludePaths[m_nCurrentIncludePath].length()-1] != '/')
                m_sIncludePaths[m_nCurrentIncludePath] += "/";
        }
    }

    /* switch to buffer */
    return (fReturn != 0)? fReturn : stdin;
}

/** \brief try to execute cpp
 *  \param sCPP the cpp to test
 *  \return true if cpp could be executed
 *
 * If we can execute cpp, it is found in the PATH.
 */
bool CPreProcess::TestCPP(const char* sCPP)
{
    if (!sCPP)
        return false;

    int pid = fork();
    int status;
    if (pid == -1)
        return false;

    // in child process call cpp
    if (pid == 0)
    {
        char *sArgs[3];
        sArgs[0] = (char*)sCPP;
        sArgs[1] = "--version";
        sArgs[2] = 0;
        // close stdout
        int fd = open("/dev/null", O_APPEND);
        fclose(stdout);
        dup2(fd, 1 /* stdout */);
        // child -> run cpp
        // last argument: the file (stdin)
        // function sets last argument always to 0
        execvp(sCPP, sArgs);
        return errno;
    }
    // parent -> wait for cpp
    waitpid(pid, &status, 0);

    return (status == 0);
}

/** \brief checks if the file-name really points to a file
 *  \param sPathToFile the full name of the file
 *  \return true if it is a correct name
 */
bool CPreProcess::CheckName(string sPathToFile)
{
    struct stat st;
    if (stat(sPathToFile.c_str(), &st))
        return false;
    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
        return false;
    return true;
}

/** \brief retrieve the current include path
 *  \return the name of the current include path
 */
string CPreProcess::GetCurrentIncludePath()
{
    string sPath;
    if (m_nCurrentIncludePath >= 0) // set by PreProcess (or rather OpenFile)
        sPath = m_sIncludePaths[m_nCurrentIncludePath];
    return sPath;
}

/** \brief tries to find the suitable path for a given file name
 *  \param sFilename the given filename
 *  \param nLineNb
 *  \return the path to the file
 *
 * The function searches the stored include statements to find one, where the stored filename
 * is part of the given filename and the linenumbers match. It also checks if the resulting string
 * is one of the given include paths.
 */
string CPreProcess::FindPathToFile(string sFilename, int nLineNb)
{
    if (sFilename.empty())
        return string();

    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->m_pNext)
    {
        // if no file-name, move on
        if (!(pCurrent->m_pFilename))
            continue;
        // test line numbers
        if (pCurrent->m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind(*(pCurrent->m_pFilename))) < 0)
            continue;
        // file name found, now extract path
        string sPath = sFilename.substr(0, nPos);
        // now search if path is in include paths
        for (int i=0; !m_sIncludePaths[i].empty(); i++)
        {
            if (m_sIncludePaths[i] == sPath)
                return sPath;
        }
    }

    // now we try to find the include path at the beginning of the string
    for (int i=0; !m_sIncludePaths[i].empty(); i++)
    {
        if (sFilename.substr(0, m_sIncludePaths[i].length()) == m_sIncludePaths[i])
            return m_sIncludePaths[i];
    }

    return string();
}

/** \brief tries to find the original include statement for the given file name
 *  \param sFilename the name to search for
 *  \param nLineNb the line number where the include statemenet appeared on
 *  \return the text of the original include statement.
 */
string CPreProcess::GetOriginalIncludeForFile(string sFilename, int nLineNb)
{
    if (sFilename.empty())
        return string();

    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->m_pNext)
    {
        // if no file-name, move on
        if (!(pCurrent->m_pFilename))
            continue;
        // test line numbers
        if (pCurrent->m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind(*(pCurrent->m_pFilename))) < 0)
            continue;
        // file name found, return it
        return *(pCurrent->m_pFilename);
    }
    return string();
}

/** \brief pushes a include bookmark to the top of the list
 *  \param pNew the new bookmark
 */
void CPreProcess::AddIncludeBookmark(inc_bookmark_t* pNew)
{
    pNew->m_pNext = 0;
    pNew->m_pPrev = m_pBookmarkTail;
    if (m_pBookmarkTail != 0)
        m_pBookmarkTail->m_pNext = pNew;
    m_pBookmarkTail = pNew;
    if (!m_pBookmarkHead)
        m_pBookmarkHead = pNew;
}

/** \brief removes a include bookmark from the top of the list
 *  \return the top element
 */
inc_bookmark_t* CPreProcess::PopIncludeBookmark()
{
    if (!m_pBookmarkHead)
        return 0;
    if (m_pBookmarkHead->m_pNext != 0)
        m_pBookmarkHead->m_pNext->m_pPrev = 0;
    inc_bookmark_t *tmp = m_pBookmarkHead;
    m_pBookmarkHead = m_pBookmarkHead->m_pNext;
    tmp->m_pNext = 0;
    return tmp;
}

/** \brief returns a reference to the current include bookmark
 *  \return a reference to the current include bookmark
 */
inc_bookmark_t* CPreProcess::FirstIncludeBookmark()
{
    return m_pBookmarkHead;
}

/** \brief checks if the given file is included as standard include
 *  \param sFilename the name of the file
 *  \param nLineNb the line number of the include statement
 *  \return true if standard include
 */
bool CPreProcess::IsStandardInclude(string sFilename, int nLineNb)
{
    if (sFilename.empty())
        return false;

    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->m_pNext)
    {
        // if no file-name, move on
        if (!(pCurrent->m_pFilename))
            continue;
        // test line numbers
        if (pCurrent->m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind(*(pCurrent->m_pFilename))) < 0)
            continue;
        // file name found, now extract standard include
        return pCurrent->m_bStandard;
    }
    return false;
}

/** \brief tries to find the line number where the given file was included
 *  \param sFilename the name of the included file
 *  \param sFromFile the name of the file, which included the other file
 *  \return the line-number of the include-statement
 */
int CPreProcess::FindLineNbOfInclude(string sFilename, string sFromFile)
{
    if (sFilename.empty())
        return 0;
    if (sFromFile.empty() && !sTopLevelInFileName.empty())
        sFromFile = sTopLevelInFileName;
    if (sFromFile.empty()) // if no from file, this may be top???
        return 1;

    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->m_pNext)
    {
        // if no file-name, move on
        if (!(pCurrent->m_pFilename))
            continue;
        // if no from file-name, move on
        if (!(pCurrent->m_pFromFile))
            continue;
        // test for file name
        if (sFilename.rfind(*(pCurrent->m_pFilename)) < 0)
            continue;
        // test for from file name
        if (sFromFile.rfind(*(pCurrent->m_pFromFile)) < 0)
            continue;
        // file name found, now return line number
        return pCurrent->m_nLineNb;
    }
    return 1;
}

/** \brief adds include bookmark to parser
 *  \param sFile the filename of the include statement
 *  \param sFromFile the file where the include statement is located
 *  \param nLineNb the line number of the include statement
 *  \param bImport true if the file is imported
 *  \param bStandard true if the include is a standard include ('<'file'>')
 *  \return true if include statement already exists
 */
bool CPreProcess::AddInclude(string sFile, string sFromFile, int nLineNb, bool bImport, bool bStandard)
{
    // check if this include statement already exists
    inc_bookmark_t *pCurrent;
    for (pCurrent = FirstIncludeBookmark(); pCurrent != 0; pCurrent = pCurrent->m_pNext)
    {
        if (*(pCurrent->m_pFilename) != sFile)
            continue;
        if (*(pCurrent->m_pFromFile) != sFromFile)
            continue;
        if (pCurrent->m_nLineNb != nLineNb)
            continue;
        if (pCurrent->m_bImport != bImport)
            continue;
        if (pCurrent->m_bStandard != bStandard)
            continue;
        return true;
    }
    // now create new entry
    inc_bookmark_t* pNew = new_include_bookmark();
    pNew->m_pFilename = new string(sFile);
    pNew->m_pFromFile = new string(sFromFile);
    pNew->m_nLineNb = nLineNb;
    pNew->m_bImport = bImport;
    pNew->m_bStandard = bStandard;
    AddIncludeBookmark(pNew);
    // return if entry existed before
    return false;
}

/** \brief checks if the string for cpp contains arguments
 *  \param sCPP the string for cpp
 *  \return sCPP without arguments
 *
 * This function sets the first argument, which is supposed to be the
 * name of the program itself and if sCPP contains arguments, them as
 * well.
 *
 * We iterate over the string and check if there is a space in it.
 * If there is, we chop off the stuff after it, add it as argument
 * and start over again. The very first argument is then returned.
 */
char* CPreProcess::CheckCPPforArguments(const char* sCPP)
{
    string sC(sCPP);
    while (sC[0] == ' ')
        sC.erase(sC.begin());
    while (*(sC.end()-1) == ' ')
        sC.erase(sC.end()-1);
    int nPos;
    while ((nPos = sC.rfind(' ')) > 0)
    {
        string sRight = sC.substr(nPos+1);
        sC = sC.substr(0, nPos);
        while (*(sC.end()-1) == ' ')
            sC.erase(sC.end()-1);
        if (!sRight.empty())
            AddCPPArgument(sRight);
    }
    return strdup(sC.c_str());
}

/** \brief set options of the preprocessor
 *  \param nOptionAdd the options to add
 *  \param nOptionRemove the options to removes
 *
 * If you add and remove the same option, it is added.
 */
void CPreProcess::SetOption(unsigned int nOptionAdd, unsigned int nOptionRemove)
{
    m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nOptionRemove)] &=
        ~PROGRAM_OPTION_OPTION(nOptionRemove);
    m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nOptionAdd)] |=
        ~PROGRAM_OPTION_OPTION(nOptionAdd);
}

/** \brief test if an option is set
 *    \param nRawOption the option to test for
 *  \return true if the option is set
 */
bool CPreProcess::IsOptionSet(unsigned int nRawOption)
{
    return (m_nOptions[PROGRAM_OPTION_GROUP_INDEX(nRawOption)] &
        PROGRAM_OPTION_OPTION(nRawOption)) > 0;
}

/** \brief search for the first include statement in the given file
 *  \param sFilename the name of the file to search for
 *  \return a reference to the include bookmark
 */
inc_bookmark_t* CPreProcess::GetFirstIncludeInFile(string sFilename)
{
    if (!m_pBookmarkHead)
        return 0;
    if (*(m_pBookmarkHead->m_pFromFile) == sFilename)
        return m_pBookmarkHead;
    return GetNextIncludeInFile(sFilename, m_pBookmarkHead);
}

/** \brief search for the next include statement in the given file
 *  \param sFilename the name of the file containing the include statements
 *  \param pPrev the pointer to the bookmark of the previous include statement
 *  \return a reference to the next include statement or NULL if no more include statements
 */
inc_bookmark_t* CPreProcess::GetNextIncludeInFile(string sFilename, inc_bookmark_t* pPrev)
{
    // iterate over bookmarks
    inc_bookmark_t *pCur = pPrev;
    while (pCur)
    {
        pCur = pCur->m_pNext;
        if (pCur && (*(pCur->m_pFromFile) == sFilename))
            return pCur;
    }
    // nothing found
    return 0;
}
