/**
 *  \file    dice/src/CPreProcess.cpp
 *  \brief   contains the implementation of the class CPreProcess
 *
 *  \date    Mon Jul 28 2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "Messages.h"
#include "Error.h"
#include <cerrno> // needed for errno
#include <unistd.h> // needed for pipe
#include <sys/wait.h> // needed for waitpid
#include <sys/types.h> // needed for waitpid
#include <sys/stat.h> // needed for open
#include <fcntl.h> // needed for open
#include "fe/FEFile.h"
#include <iostream>
#include <cassert>

//@{
/** globale pre-processor variables and function */
extern FILE *incin,*incout;
/** the lexer function to get the tokens */
extern int inclex();
//@}

/** the name of the input file */
extern string sInFileName;
/** back up name of the top level input file - we need this when scanning
 * included files */
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
    char *sgcc = strdup("gcc");
    if (!sCPP)
        sCPP = getenv("CC");
    if (sCPP && (strlen(sCPP) > 0))
      m_sCPPProgram = strdup(sCPP);
    else if (TestCPP(sgcc))
      m_sCPPProgram = sgcc;
    else
      m_sCPPProgram = sgcc;
    char* sC = CheckCPPforArguments(m_sCPPProgram);
    if (m_sCPPProgram)
        free(m_sCPPProgram);
    m_sCPPProgram = sC;
    // 1. argument: "-E"
    AddCPPArgument("-E");
    // to allow distinction between GCC's CPP and DICE's CPP invocation
    AddCPPArgument("-DDICE");
    //AddCPPArgument("-P");
    m_nCurrentIncludePath = -1;
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
    if (m_sCPPArgs)
	free(m_sCPPArgs);
    if (m_sCPPProgram)
        free(m_sCPPProgram);
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

/** \brief removes double slashes from the filenames
 *  \param s the string to remove slashes from
 *  \return the string with slashes removed
 */
string CPreProcess::RemoveSlashes(string & s)
{
    // do not use realpath, because we want to keep relative paths
    string::size_type pos;
    while ((pos = s.find("//")) != string::npos)
	s.erase(pos, 1);
    return s;
}

/** \brief adds a include path
 *  \param sPath the path to add
 */
void CPreProcess::AddIncludePath(string sPath)
{
    // add trailing slash if not present
    if (sPath[sPath.length()-1] != '/')
	sPath += "/";
    // normalize any double slashes (//)
    RemoveSlashes(sPath);
    m_vIncludePaths.push_back(sPath);
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
 * Since this is always called after the constructor, we savely override the
 * configured CPP. This can be used to specify a different CPP at runtime.
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
        CMessages::Error("execvp(\"%s\", cpp_args) returned: %s\n", 
	    m_sCPPProgram, s);
    else
        CMessages::Error("execvp(\"%s\", cpp_args) returned an unknown error\n",
	    m_sCPPProgram);
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
	throw new error::preprocess_error("No preprocessor set.\n");

    int pipes[2];
    if (pipe(pipes) == -1)
	throw new error::preprocess_error("Could not open pipe.\n");

    int pid = fork();
    int status;
    if (pid == -1)
	throw new error::preprocess_error("Could not fork preprocess.\n");

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
	throw new error::preprocess_error("Could not start preprocess program.\n");
    }
    // parent -> wait for cpp
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
}

/** \brief pre-process the file
 *  \param sFilename the filename of the file to pre-process
 *  \param bDefault true if the file to pre-process is a default file
 *  \return true if successful
 *
 * First we scan for include/import, and then we run CPP.
 */
FILE* CPreProcess::PreProcess(string sFilename, bool bDefault)
{
    FILE *fInput = OpenFile(sFilename, bDefault);
    if (!fInput)
        return 0; // open file already printed error message

    sInFileName = sFilename;
    if (sInFileName.empty())
        sInFileName = "<stdin>";
    // we add the current path to the file-name so it conformes to the stored
    // file-name generated by Gcc
    if (m_nCurrentIncludePath >= 0 &&
	m_nCurrentIncludePath < (int)m_vIncludePaths.size())
        sTopLevelInFileName = m_vIncludePaths[m_nCurrentIncludePath] +
	    sFilename;
    else
        sTopLevelInFileName = sFilename;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Start preprocessing input file (\"%s\") ...\n",
	sTopLevelInFileName.c_str());
    // search for import and include statements
    FILE *fOutput;
    string s, sBase;
    if (CCompiler::IsOptionSet(PROGRAM_KEEP_TMP_FILES))
    {
        sBase = sFilename.substr(sFilename.rfind('/')+1);
        s = "temp1-" +  sBase;
        fOutput = fopen(s.c_str(), "w+");
    }
    else
        fOutput = tmpfile();
    if (!fOutput)
    {
        std::cerr << "could not create temporary file\n";
        if (fInput != 0)
            fclose(fInput);
        return 0;
    }

    // check if parser has to contribute something
    // Parser has to set at least the line statement correctly
    CParser *pParser = CParser::GetCurrentParser();
    pParser->PrepareEnvironment(sFilename, fInput, fOutput);
    // search fInput for import statements
    incin = fInput;
    incout = fOutput;
    rewind(incin);
    // set line number count to start
    gLineNumber = 1;
    inclex();

    // close input of preprocess
    if (!sFilename.empty())    // _not_ stdin
        fclose(fInput);

    // now all includes and imports are scanned

    // get new temp file
    if (CCompiler::IsOptionSet(PROGRAM_KEEP_TMP_FILES))
    {
        s = "temp2-" + sBase;
        fInput = fopen(s.c_str(), "w+");
    }
    else
        fInput = tmpfile();
    if (!fInput)
    {
        std::cerr << "could not create temporary file\n";
        if (fOutput != 0)
            fclose(fOutput);
        return 0;
    }
    // set input file handle to beginning of file
    rewind(fOutput);

    // turn verboseness of CPP on
    if (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_PARSER))
    {
        AddCPPArgument("-v");
        AddCPPArgument("-H");
    }

    // run cpp preprocessor
    try
    {
	int ret;
	if ((ret = ExecCPP(fOutput, fInput)) > 0)
	{
	    std::cerr << "Preprocessing \"" << sInFileName << "\" returned " << 
		ret << ".\n";
	    return 0;
	}
    }
    catch (error::preprocess_error *e)
    {
	std::cerr << "Preprocessing error: " << e->what() << std::endl;
	delete e;

	return 0;
    }
    // close input file of cpp scanner
    fclose(fOutput);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "... finished preprocessing input file.\n");

    // set input to beginning of file again
    rewind(fInput);

    if (!fInput)
        CMessages::Error("No input file.\n");

    return fInput;
}

/** \brief tries to open a file (and use the search paths)
 *  \param sName the name of the file
 *  \param bDefault true if the file is a deafult file ('<' file '>')
 *  \param bIgnoreErrors true if this function should not print any errors, but return a 0 handle
 *  \return a file descriptior if file found
 */
FILE* 
CPreProcess::OpenFile(string sName, 
    bool bDefault, 
    bool bIgnoreErrors)
{
    RemoveSlashes(sName);
    
    string sCurPath;
    if (m_nCurrentIncludePath < 0 || bDefault)
        sCurPath = "";
    else
        sCurPath = m_vIncludePaths[m_nCurrentIncludePath];
    /* try to open included file */
    if (sCurPath.empty())
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open include \"%s\"\n", sName.c_str());
    else
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open include \"%s\" in path \"%s\"\n",
	    sName.c_str(), sCurPath.c_str());
    FILE *fReturn = 0;
    if (sCurPath.empty()) // also empty if default
    {
        // check if this is a file
        if (CheckName(sName))
            fReturn = fopen(sName.c_str(), "r");
        else
            fReturn = 0;
    }
    else
    {
        string s = sCurPath + sName;
        if (CheckName(s))
            fReturn = fopen(s.c_str(), "r");
        else
            fReturn = 0;
    }
    if (fReturn)
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Success\n");

    if (!fReturn)
    {
        /* not in current directory -> search paths */
	for (int i = 0; i < (int)m_vIncludePaths.size() && !fReturn; i++)
	{
	    string s = m_vIncludePaths[i];
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Search file in path \"%s\".\n", s.c_str());
	    s += sName;
	    if (CheckName(s))
	    {
		m_nCurrentIncludePath = i;
		sCurPath = m_vIncludePaths[i];
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open with path %s.\n", s.c_str());
		// it's there
		fReturn = fopen(s.c_str(), "r");
	    }
        }
    }

    if (fReturn)
    {
	if (sCurPath.empty())
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Found file \"%s\" in current path.\n",
		sName.c_str());
	else
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Found file \"%s\" in path \"%s\".\n",
		sName.c_str(), m_vIncludePaths[m_nCurrentIncludePath].c_str());
    }

    if (!fReturn && !bIgnoreErrors)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Couldn't find file\n");
        CFEFile *pCurFile = CParser::GetCurrentFile();
        if (!pCurFile)
        {
            std::cerr << "dice: " << sName << ": No such file or directory.\n";
            return 0;
        }
        /* if not open by now, couldn't find file */
        if (pCurFile->GetParent())
        {
            // run to top
            CFEFile *pFile = pCurFile->GetSpecificParent<CFEFile>(1); 
            vector<CFEFile*> vStack;
            while (pFile)
            {
                vStack.insert(vStack.begin(), pFile);
                pFile = pFile->GetSpecificParent<CFEFile>(1); 
            }
            // down in line
            if (vStack.size() > 1)
                std::cerr << "In file included ";
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
                std::cerr << "from " << sFileName << ":" << nLine;
                if (iter+1 != vStack.end())
                    std::cerr << ",\n                 ";
                else
                    std::cerr << ":\n";
                iter++;
            }
        }
        string sFileName = pCurFile->GetFullFileName();
        if (sFileName.empty())
            sFileName = sTopLevelInFileName;
        int nLine = FindLineNbOfInclude(sName, sFileName);
        std::cerr << sFileName << ":" << nLine << ": " << sName << 
	    ": No such file or directory.\n";
    }
    if (!fReturn)
        return 0;

    /* store path and file name to map them later */
    CIncludeStatement b(false, bDefault, false, false, sName, string(), sCurPath, 0);
    m_vOpenBookmarks.push_back(b);

    /* switch to buffer */
    return (fReturn != 0) ? fReturn : stdin;
}

/** \brief try to execute cpp
 *  \param sCPP the cpp to test
 *  \return true if cpp could be executed
 *
 * If we can execute cpp, it is found in the PATH.
 */
bool CPreProcess::TestCPP(char* sCPP)
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
	char * const sArgs[3] = {sCPP, "--version", 0 };
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
 *  \param sFilename the filename to get the path for
 *  \return the name of the current include path
 */
string CPreProcess::GetIncludePath(string sFilename)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s) called.\n", __func__,
	sFilename.c_str());
    
    vector<CIncludeStatement>::iterator iter;
    for (iter = m_vOpenBookmarks.begin();
	 iter != m_vOpenBookmarks.end();
	 iter++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s comparing %s to %s\n", 
	    __func__, sFilename.c_str(),
	    (*iter).m_sFilename.c_str());
	if ((*iter).m_sFilename == sFilename)
	    return (*iter).m_sPath;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s nothing found, return \"\"\n",
	__func__);
    return string();
}

/** \brief tries to find the suitable path for a given file name
 *  \param sFilename the given filename
 *  \param nLineNb
 *  \return the path to the file
 *
 * The function searches the stored include statements to find one, where the
 * stored filename is part of the given filename and the linenumbers match. It
 * also checks if the resulting string is one of the given include paths.
 */
string CPreProcess::FindPathToFile(string sFilename, unsigned int nLineNb)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s, %d) called\n", __func__,
	sFilename.c_str(), nLineNb);
    
    if (sFilename.empty())
        return string();

    // normalize any double slashes (//) as was done for m_vIncludePaths
    RemoveSlashes(sFilename);
    
    vector<CIncludeStatement>::iterator i;
    for (i = m_vBookmarks.begin(); 
	i != m_vBookmarks.end();
	i++)
    {
        // if no file-name, move on
        if ((*i).m_sFilename.empty())
            continue;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s comparing to (%s, %d)\n",
	    __func__, (*i).m_sFilename.c_str(),
	    (*i).m_nLineNb);
	
        // test line numbers
        if ((*i).m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind((*i).m_sFilename)) < 0)
            continue;
	// file name found, now extract path
	string sPath = sFilename.substr(0, nPos);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s path should be %s\n", __func__,
	    sPath.c_str());
	
        // now search if path is in include paths
        for (vector<string>::iterator i2 = m_vIncludePaths.begin(); 
	     i2 != m_vIncludePaths.end();
	     i2++)
        {
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s comparing path to %s\n",
		__func__, (*i2).c_str());
	    
            if ((*i2) == sPath)
                return sPath;

	    /* This is a fix for gcc 3.3: newer version will generate
	     * preprocess output which contains relative paths, e.g.,
	     * /usr/include/linux/../blah.h. gcc 3.3 on the other hand will
	     * generate /usr/include/blah.h. To catch this, we have to call
	     * realpath on the stored paths.
	     */
	    char real_path_buffer[PATH_MAX];
	    char *real_path = realpath((*i2).c_str(), real_path_buffer);
	    if (real_path)
	    {
		/* realpath removes trainling slashes, but we need trailing
		 * slashes for comparison.
		 */
		string s = string(real_path) + "/";
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		    "%s comparing path to %s\n", __func__, s.c_str());

		if (sPath == s);
		    return sPath;
	    }
        }
    }

    // now we try to find the include path at the beginning of the string
    for (vector<string>::iterator i2 = m_vIncludePaths.begin();
	 i2 != m_vIncludePaths.end();
	 i2++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "%s try to find path %s in filename\n", __func__, (*i2).c_str());
	
        if (sFilename.substr(0, (*i2).length()) == (*i2))
            return *i2;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s nothing found, returning \"\"\n",
	__func__);
    return string();
}

/** \brief tries to find the original include statement for the given file name
 *  \param sFilename the name to search for
 *  \param nLineNb the line number where the include statemenet appeared on
 *  \return the text of the original include statement.
 */
string CPreProcess::GetOriginalIncludeForFile(string sFilename, unsigned int nLineNb)
{
    if (sFilename.empty())
        return string();

    RemoveSlashes(sFilename);

    vector<CIncludeStatement>::iterator i;
    for (i = m_vBookmarks.begin(); 
	i != m_vBookmarks.end();
	i++)
    {
        // if no file-name, move on
        if ((*i).m_sFilename.empty())
            continue;
        // test line numbers
        if ((*i).m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind((*i).m_sFilename)) < 0)
            continue;
        // file name found, return it
        return (*i).m_sFilename;
    }
    return string();
}

/** \brief checks if the given file is included as standard include
 *  \param sFilename the name of the file
 *  \param nLineNb the line number of the include statement
 *  \return true if standard include
 */
bool CPreProcess::IsStandardInclude(string sFilename, unsigned int nLineNb)
{
    if (sFilename.empty())
        return false;

    RemoveSlashes(sFilename);

    vector<CIncludeStatement>::iterator i;
    for (i = m_vBookmarks.begin();
	 i != m_vBookmarks.end();
	 i++)
    {
        // if no file-name, move on
        if ((*i).m_sFilename.empty())
            continue;
        // test line numbers
        if ((*i).m_nLineNb != nLineNb)
            continue;
        // test for file name
        int nPos;
        if ((nPos = sFilename.rfind((*i).m_sFilename)) < 0)
            continue;
        // file name found, now extract standard include
        return (*i).m_bStandard;
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

    vector<CIncludeStatement>::iterator i;
    for (i = m_vBookmarks.begin();
	i != m_vBookmarks.end();
	i++)
    {
        // if no file-name, move on
        if ((*i).m_sFilename.empty())
            continue;
        // if no from file-name, move on
        if ((*i).m_sFromFile.empty())
            continue;
        // test for file name
        if (sFilename.rfind((*i).m_sFilename) == string::npos)
            continue;
        // test for from file name
        if (sFromFile.rfind((*i).m_sFromFile) == string::npos)
            continue;
        // file name found, now return line number
        return (*i).m_nLineNb;
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
bool 
CPreProcess::AddInclude(string sFile,
    string sFromFile,
    unsigned int nLineNb,
    bool bImport,
    bool bStandard)
{
    RemoveSlashes(sFile);

    // check if this include statement already exists
    vector<CIncludeStatement>::iterator i;
    for (i = m_vBookmarks.begin();
	i != m_vBookmarks.end();
	i++)
    {
        if ((*i).m_sFilename != sFile)
            continue;
        if ((*i).m_sFromFile != sFromFile)
            continue;
        if ((*i).m_nLineNb != nLineNb)
            continue;
        if ((*i).m_bImport != bImport)
            continue;
        if ((*i).m_bStandard != bStandard)
            continue;
        return true;
    }
    // now create new entry
    CIncludeStatement inc(false, bStandard, false, bImport, sFile, sFromFile,
	string(), nLineNb);
    m_vBookmarks.push_back(inc);
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

/** \brief search for the first include statement in the given file
 *  \return a reference to the include bookmark
 */
vector<CIncludeStatement>::iterator
CPreProcess::GetFirstIncludeInFile()
{
    return m_vBookmarks.begin();
}

/** \brief search for the next include statement in the given file
 *  \param sFilename the name of the file containing the include statements
 *  \param iter the pointer to the bookmark of the previous include statement
 *  \return a reference to the next include statement or 0 if no more include statements
 */
CIncludeStatement*
CPreProcess::GetNextIncludeInFile(string sFilename,
    vector<CIncludeStatement>::iterator & iter)
{
    // iterate over bookmarks
    for (; iter != m_vBookmarks.end(); iter++)
    {
	if ((*iter).m_sFromFile == sFilename)
	{
	    /* I know it looks ugly, but we first have to derefence the
	     * iterator to get the element type and then create a reference to
	     * that element.
	     */
	    CIncludeStatement *ret = &(*iter);
	    iter++;
	    return ret;
	}
    }
    // nothing found
    return (CIncludeStatement*)0;
}

