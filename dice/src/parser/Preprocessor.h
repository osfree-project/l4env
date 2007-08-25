/**
 *  \file    dice/src/parser/Preprocess.h
 *  \brief   contains the declaration of the preprocessor class
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

/** preprocessing symbol to check header file */
#ifndef __DICE_PARSER_PREPROCESSOR_H__
#define __DICE_PARSER_PREPROCESSOR_H__

#include <string>
using std::string;
#include <vector>
using std::vector;

namespace dice {
    namespace parser {
	/** \class CPreprocessor
	 *  \ingroup parser
	 *  \brief encapsulates the execution of the preprocessor
	 *
	 * We should use an own instance of this class for each preprocess run
	 * we have to perform.  However, include paths, executable name, and
	 * arguments should be the same for all instances.  Therefore, we make
	 * this class a singleton, that is, only one instance of this class
	 * exists (constructor is private) which can be obtained using the
	 * GetPreprocessor method.  This method creates the instance if it
	 * doesn't exist.
	 */
	class CPreprocessor
	{
	    CPreprocessor();
	public:
	    ~CPreprocessor();

	    void AddArgument(string sArgument);
	    void AddArgument(const char* sArgument);
	    void SetPreprocessor(string sExec);
	    void SetPreprocessor(const char *sExec);

	    void AddIncludePath(string sPath);

	    FILE* Preprocess(string sFile, string& sPath);

	    static CPreprocessor* GetPreprocessor();

	    string FindIncludePathInName(string sName);

	private:
	    string CheckForArguments(string sExec);
	    bool TestExecutable(string sExec);
	    void RemoveSlashes(string& sPath);
	    int ExecPreprocessor(FILE* fInput, FILE* fOuput);
	    void ErrorHandling();
	    FILE* OpenFile(string sFile, string& sPath);
	    bool CheckName(string sFile);

	private:
	    /** \var CPreprocessor* pPreprocessor
	     *  \brief reference to the single instance of this class
	     */
	    static CPreprocessor* pPreprocessor;
	    /** \var string sExecutable
	     *  \brief the name of the preprocessor executable
	     */
	    string sExecutable;
	    /** \var vector<string> sArguments
	     *  \brief the argument list
	     */
	    vector<string> sArguments;
	    /** \var vector<string> sIncludePaths
	     *  \brief the list of include paths specified with -I
	     */
	    vector<string> sIncludePaths;
	    /** \var long nCurrentIncPath
	     *  \brief index to the currently used include path
	     *
	     * This index indicates the last include path used. We always look
	     * for the currently opened file in the last include path used.
	     */
	    long nCurrentIncPath;
	};

    };
};

#endif /* __DICE_PARSER_CONVERTER_H__ */
