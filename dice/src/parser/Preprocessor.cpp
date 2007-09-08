/**
 *  \file    dice/src/parser/Preprocessor.cpp
 *  \brief   contains the definition of the CPreprocessor class
 *
 *  \date    06/19/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "Preprocessor.h"
#include "Compiler.h"
#include "Messages.h"
#include "Error.h"
#include <cstdio> // fclose
#include <unistd.h> // dup2
#include <fcntl.h> // open
#include <sys/wait.h> // waitpid
#include <sys/stat.h> // S_ISREG, S_ISLNK
#include <errno.h> // errno
#include <iostream> // cerr

using namespace dice::parser;

CPreprocessor* CPreprocessor::pPreprocessor = 0;

CPreprocessor::CPreprocessor()
{
    char *sCPP = getenv("CXX");
    char *sgcc = strdup("gcc");
    if (!sCPP)
	sCPP = getenv("CC");
    if (sCPP && (strlen(sCPP) > 0))
	sExecutable = sCPP;
    else
	sExecutable = sgcc;
    sExecutable = CheckForArguments(sExecutable);
    // 1. argument: "-E"
    AddArgument("-E");
    // to allow distinction between GCC's CPP and DICE's CPP invocation
    AddArgument("-DDICE");
//     AddCPPArgument("-P");
    nCurrentIncPath = -1;
}

CPreprocessor::~CPreprocessor()
{
    pPreprocessor = 0;
}

/** \brief get the reference to the ONE preprocessor
 *  \return the reference
 */
CPreprocessor* CPreprocessor::GetPreprocessor()
{
    if (!pPreprocessor)
	pPreprocessor = new CPreprocessor();
    return pPreprocessor;
}

/** \brief add an argument
 *  \param sArgument the argument to add
 */
void CPreprocessor::AddArgument(string sArgument)
{
    if (sArgument.empty())
	return;
    sArguments.push_back(sArgument);
}

/** \brief add an argument
 *  \param sArgument the argument to add
 */
void CPreprocessor::AddArgument(const char* sArgument)
{
    AddArgument(string(sArgument));
}

/** \brief set the name of the executable
 *  \param sExec the name of the executable
 *
 * Before setting the executable we check for additional arguments and if the
 * executable can be executed.
 */
void CPreprocessor::SetPreprocessor(string sExec)
{
    if (sExec.empty())
	return;
    sExec = CheckForArguments(sExec);
    if (!TestExecutable(sExec))
	return;
    sExecutable = sExec;
}

/** \brief set the name of the executable
 *  \param sExec the name of the executable
 */
void CPreprocessor::SetPreprocessor(const char *sExec)
{
    SetPreprocessor(sExec);
}

/** \brief check if the string for the executable contains arguments
 *  \param sExec the string to check
 *  \return the executable string stripped by the arguments
 *
 * This function sets the first argument, which is supposed to be the name of
 * the program itself and if sCPP contains arguments, them as well.
 *
 * We iterate over the string and check if there is a space in it.  If there
 * is, we chop off the stuff after it, add it as argument and start over
 * again. The very first argument is then returned.
 *
 * This function is used to catch something like "gcc -E". However, we want to
 * keep something like "distcc gcc". However, testing for "ccache gcc" won't
 * work, so we use a special case for distcc and stick with removing all
 * trainling options.
 */
string CPreprocessor::CheckForArguments(string sExec)
{
    // trim
    while (sExec[0] == ' ')
        sExec.erase(sExec.begin());
    while (*(sExec.end()-1) == ' ')
        sExec.erase(sExec.end()-1);
    // find substrings
    string::size_type nPos;
    while ((nPos = sExec.rfind(" ")) != string::npos)
    {
        string sRight = sExec.substr(nPos+1);
        sExec = sExec.substr(0, nPos);
        while (*(sExec.end()-1) == ' ')
            sExec.erase(sExec.end()-1);
	/** *sigh* distcc has to be used with the executable as argument. All
	 * others, such as ccache don't
	 */
	if (sExec.rfind("distcc") != string::npos &&
	    sExec.substr(sExec.rfind("distcc")) == "distcc")
	    sExec += " " + sRight;
	else
	    AddArgument(sRight);
    }
    return sExec;
}

/** \brief test for the executable
 *  \param sExec the executable to test
 *  \return true if executable can be executed
 *
 * We simply call the executable with the "--version" argument. This
 * (hopefully) always works.
 */
bool CPreprocessor::TestExecutable(string sExec)
{
    if (sExec.empty())
	return false;

    int pid = fork();
    int status;
    if (pid == -1)
	return false;

    if (pid == 0)
    {
	string::size_type l = sExec.length() + 1;
	char *e = new char[l];
	strncpy(e, sExec.c_str(), l);
	char *v = new char[sizeof("--version")];
	strcpy(v, "--version");
	char* const sArgs[] = { e, v, 0 };
	// close stdout
	int fd = open("/dev/null", O_APPEND);
	fclose(stdout);
	dup2(fd, 1 /* stdout */);
	// child -> run cpp
	// last argument: the file (stdin)
	// function sets last argument always to 0
	execvp(sExec.c_str(), sArgs);
	delete[] v;
	delete[] e;
	return errno;
    }
    waitpid(pid, &status, 0);

    return status == 0;
}

/** \brief helper function to remove double slashes ("//")
 *  \param sPath the path to fix
 *  \retval sPath the fixed path
 *
 * To make all paths look uniform we replace all double slashes by single
 * ones.
 */
void CPreprocessor::RemoveSlashes(string& sPath)
{
    string::size_type pos;
    while ((pos = sPath.find("//")) != string::npos)
	sPath.erase(pos, 1);
}

/** \brief add an include path
 *  \param sPath the include path to add
 *
 * This functions adds the trailing slash ('/') to the path and then stores
 * it.  To ensure that all paths are the same, we remove double slashes before
 * storing.
 */
void CPreprocessor::AddIncludePath(string sPath)
{
    sPath += "/";
    RemoveSlashes(sPath);
    sIncludePaths.push_back(sPath);
}

/** \brief preprocess the given file
 *  \param sFile the name of the file to preprocess
 *  \retval sPath the path where the file was found
 *  \return a file handle to the preprocessed file
 */
FILE* CPreprocessor::Preprocess(string sFile, string& sPath)
{
    FILE* fInput = OpenFile(sFile, sPath);
    if (!fInput)
	return 0; // error message printed by OpenFile

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CPreprocessor::%s: start preprocessing input file \"%s\" ...\n", __func__,
	sFile.c_str());

    FILE *fOutput;
    if (CCompiler::IsOptionSet(PROGRAM_KEEP_TMP_FILES))
    {
	string s = "temp-" + sFile.substr(sFile.rfind('/')+1);
	fOutput = fopen(s.c_str(), "w+");
    }
    else
	fOutput = tmpfile();
    if (!fOutput)
    {
	std::cerr << "CPreprocessor: could not create temporary file\n";
	if (fInput)
	    fclose(fInput);
	return 0;
    }

    if (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_PARSER))
    {
	AddArgument("-v");
	AddArgument("-H");
    }

    rewind(fInput);

    int ret;
    if ((ret = ExecPreprocessor(fInput, fOutput)))
    {
	std::cerr << "CPreprocessor: preprocessing \"" << sFile << "\" returned " << ret << ".\n";
	return 0;
    }

    fclose(fInput);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CPreprocessor::%s: finished preprocessing input file \"%s\".\n", __func__,
	sFile.c_str());

    rewind(fOutput);
    return fOutput;
}

/** \brief print diagnostic message if error in preprocess execution
 *
 * This function is called in the child process only if the preprocessor could
 * not be started.
 */
void CPreprocessor::ErrorHandling()
{
    char *s = strerror(errno);
    if (s)
	CMessages::Error("execvp(\"%s\", cpp_args) returned: %s\n",
	    sExecutable.c_str(), s);
    else
	CMessages::Error("execvp(\"%s\", cpp_args) returned an unknown error\n",
	    sExecutable.c_str());
}

/** \brief runs the C pre-processor
 *  \param fInput the input file
 *  \param fOutput the output file
 *  \return an error code if something went wrong, zero (0) otherwise
 */
int CPreprocessor::ExecPreprocessor(FILE *fInput, FILE* fOutput)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_PARSER, "CPreprocessor::%s called\n", __func__);

    if (sExecutable.empty())
	throw new error::preprocess_error("CPreprocessor: No preprocessor set.\n");

    int pipes[2];
    if (pipe(pipes) == -1)
	throw new error::preprocess_error("CPreprocessor: Could not open pipe.\n");

    int pid = fork();
    int status;
    if (pid == -1)
	throw new error::preprocess_error("CPreprocessor: Could not fork preprocess.\n");

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
        AddArgument("-");
        char * sArgs[sArguments.size() + 2];
        sArgs[0] = strdup(sExecutable.c_str());
	unsigned int i;
        for (i=0; i<sArguments.size(); i++)
            sArgs[i+1] = strdup(sArguments[i].c_str());
	sArgs[i+1] = 0;

        execvp(sArgs[0], sArgs);
        ErrorHandling();
	throw new error::preprocess_error("CPreprocessor: Could not start preprocess program.\n");
    }
    // parent -> wait for cpp
    waitpid(pid, &status, 0);

    CCompiler::Verbose(PROGRAM_VERBOSE_PARSER, "CPreprocessor::%s returns %d\n", __func__,
	WEXITSTATUS(status));
    return WEXITSTATUS(status);
}

/** \brief check if the file exists
 *  \param sFile the file to check
 *  \return true if file can be opened
 */
bool CPreprocessor::CheckName(string sFile)
{
    struct stat st;
    if (stat(sFile.c_str(), &st))
        return false;
    if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
        return false;
    return true;
}

/** \brief open a file using the include paths
 *  \param sFile the file name
 *  \retval sPath the path where the file was found
 *  \return a handle to the opened file
 *
 * Try to open the file and search the given include paths.
 */
FILE* CPreprocessor::OpenFile(string sFile, string& sPath)
{
    RemoveSlashes(sFile);

    string sCurPath;
    if (nCurrentIncPath < 0)
        sCurPath = "";
    else
        sCurPath = sIncludePaths[nCurrentIncPath];
    /* try to open included file */
    if (sCurPath.empty())
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open include \"%s\"\n", sFile.c_str());
    else
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open include \"%s\" in path \"%s\"\n",
	    sFile.c_str(), sCurPath.c_str());
    FILE *fReturn = 0;
    // check if this is a file
    string s = sCurPath + sFile;
    if (CheckName(s))
	fReturn = fopen(s.c_str(), "r");
    else
	fReturn = 0;

    if (!fReturn)
    {
        /* not in current include directory -> search all include directories */
	for (int i = 0; i < (int)sIncludePaths.size() && !fReturn; i++)
	{
	    s = sIncludePaths[i];
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Search file in path \"%s\".\n", s.c_str());
	    s += sFile;
	    if (CheckName(s))
	    {
		nCurrentIncPath = i;
		sCurPath = sIncludePaths[i];
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Try to open with path %s.\n", s.c_str());
		// it's there
		fReturn = fopen(s.c_str(), "r");
	    }
        }
	/* we can have a current include path, but the file can be located in
	 * the current directory (./). Thus check this one as well.
	 */
	if (!fReturn && CheckName(sFile))
	    fReturn = fopen(sFile.c_str(), "r");
    }

    if (fReturn)
    {
	if (sCurPath.empty())
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Found file \"%s\" in current path.\n",
		sFile.c_str());
	else
	    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Found file \"%s\" in path \"%s\".\n",
		sFile.c_str(), sIncludePaths[nCurrentIncPath].c_str());
    }
    else
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "Couldn't find file\n");
        return 0; // causes error to be printed (with file stack)
    }

    sPath = sCurPath;
    return (fReturn != 0) ? fReturn : stdin;
}

/** \brief try to find the include path inside a given file name
 *  \param sName the file name
 *  \return the include path
 *
 * Usually the include paths are used first in. That is, if we iterate the
 * include paths, the first that matches the begin if the name should be the
 * looked for include path.
 */
string
CPreprocessor::FindIncludePathInName(string sName)
{
    vector<string>::iterator i;
    for (i = sIncludePaths.begin(); i != sIncludePaths.end(); i++)
    {
	if (sName.find(*i) == 0)
	    return *i;
    }
    return string();
}

