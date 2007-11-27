/**
 *  \file    dice/src/Compiler.h
 *  \brief   contains the declaration of the class CCompiler
 *
 *  \date    01/31/2001
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

/** preprocessing symbol to check header file */
#ifndef __DICE_COMPILER_H__
#define __DICE_COMPILER_H__

#include <cstdio>
#include "defines.h"
#include <string>
using std::string;
#include <vector>
#include <bitset>
using std::bitset;
#include <map>
using std::map;

#include "ProgramOptions.h"

class CBERoot;
class CFEBase;
class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;
class CBESizes;

/** \enum FrontEnd_Type
 *  \brief defines the used front-ends
 */
enum FrontEnd_Type
{
	USE_FE_NONE,    /**< defines that no front-end is used */
	USE_FE_DCE,     /**< defines that the DCE front-end is used */
	USE_FE_CORBA,   /**< defines that the CORBA front-end is used */
	USE_FE_CAPIDL,  /**< defines that the CapIDL front-end is used */
	USE_FILE_C,     /**< defines that a C header file is currently parsed */
	USE_FILE_CXX,   /**< defines that a C++ header file is currently parsed */
	USE_FILE_IDL    /**< all IDL files (DCE, CORBA) */
};

/**
 *  \class CCompiler
 *  \brief is the place where all the problems start ;)
 *
 * This class contains the "logic" of the compiler. The main function creates
 * an object of this class and calls its methods.
 */
class CCompiler
{
public:
	/** creates a compiler object */
	CCompiler();
	~ CCompiler();

	// Operations
public:
	void Write();
	void PrepareWrite();
	void ShowHelp(bool bShort = false);
	void ShowCopyright();
	void ParseArguments(int argc, char *argv[]);
	static void Verbose(ProgramVerbose_Type level, const char *format, ...)
		__attribute__(( format(printf, 2, 3) ));
	static void VerboseI(ProgramVerbose_Type level, const char *format, ...)
		__attribute__(( format(printf, 2, 3) ));
	static void VerboseD(ProgramVerbose_Type level, const char *format, ...)
		__attribute__(( format(printf, 2, 3) ));
	void Parse();

	static bool IsOptionSet(ProgramOption_Type nOption);
	static bool IsDependsOptionSet(ProgramDepend_Type nDepends);
	static bool IsVerboseLevel(ProgramVerbose_Type nVerboseLevel);
	static bool IsFileOptionSet(ProgramFile_Type nOption);
	static bool IsBackEndInterfaceSet(BackEnd_Interface_Type nOption);
	static bool IsBackEndPlatformSet(BackEnd_Platform_Type nOption);
	static bool IsBackEndLanguageSet(BackEnd_Language_Type nOption);
	static bool IsWarningSet(ProgramWarning_Type nLevel);
	static int GetTraceMsgBufDwords();
	static CBESizes* GetSizes();
	static void SetDebug(bool bOn = true);
	static bool GetBackEndOption(const string sOption, string& sValue);
	static void CheckRequire(const char* str);
	static void Error();
	static void Warning();

protected:
	void ShowVersion();

	void InitTraceLib(int argc, char* argv[]);

	static void SetOption(ProgramOption_Type nOption);
	static void UnsetOption(ProgramOption_Type nOption);
	static void SetVerboseLevel(ProgramVerbose_Type nVerboseLevel);
	static void SetDependsOption(ProgramDepend_Type nDepends);
	static void SetFileOption(ProgramFile_Type nOption);
	static void SetBackEndInterface(BackEnd_Interface_Type nAdd);
	static void SetBackEndPlatform(BackEnd_Platform_Type nAdd);
	static void SetBackEndLanguage(BackEnd_Language_Type nAdd);
	static void SetWarningLevel(ProgramWarning_Type nLevel);
	static void UnsetWarningLevel(ProgramWarning_Type nLevel);
	static void SetTraceMsgBufDwords(int nDwords);
	static void SetSizes(CBESizes *pSizes);
	static void SetBackEndOption(const string sOption, const string sValue);

	/** \class Version
	 *  \brief helper class to compare versions of Dice
	 */
	class Version
	{
	public:
		/** \enum oper
		 *  \brief the comparison operator
		 */
		enum oper
		{
			LT,		/**< less than */
			EQ,		/**< equals */
			GT,		/**< greater than */
			LTE,	/**< less than or euqal */
			GTE		/**< greater than or equal */
		};

		/** \brief constructs a Compiler Version object
		 *  \param s the string to construct the version from
		 *
		 * The string should be of the format
		 * major [ "." minor [ "." sub-minor ] ]
		 *
		 * The constructor throws a bad_version exception if the version
		 * string was invalid.
		 */
		Version(string s);
		/** \brief compare operation
		 *  \param o the operator to use for comparison
		 *  \param r the version to compare to
		 *  \return true if this version and \a r compare according to \a o
		 */
		bool compare(oper o, Version& r);

	private:
		/** \var int maj
		 *  \brief major of version
		 */
		int maj;
		/** \var int min
		 *  \brief minor of version
		 */
		int min;
		/** \var int sub
		 *  \brief subminor of version
		 */
		int sub;
		/** \var bool checkMaj
		 *  \brief used to remember the if that part of the version was checked
		 */
		bool checkMaj;
		/** \var bool checkMin
		 *  \brief used to remember the if that part of the version was checked
		 */
		bool checkMin;
		/** \var bool checkSub
		 *  \brief used to remember the if that part of the version was checked
		 */
		bool checkSub;
	};

	// Attributes
protected:
	/** \var int m_VerboseLevel
	 *  \brief the verbose level
	 */
	static ProgramVerbose_Type m_VerboseLevel;
	/** \var bitset<PROGRAM_WARNING_MAX> m_WarningLevel
	 *  \brief the warning level
	 */
	static bitset<PROGRAM_WARNING_MAX> m_WarningLevel;
	/** \var int m_nUseFrontEnd
	 *  \brief contains the front-end to use
	 */
	FrontEnd_Type m_nUseFrontEnd;
	/** \var CBERoot* m_pRootBE
	 *  \brief the root point of the back end
	 */
	CBERoot *m_pRootBE;
	/** \var CFEFile *m_pRootFE
	 *  \brief a reference to the top most file of the front-end
	 */
	CFEFile *m_pRootFE;
	/** \var string m_sOutFileName
	 *  \brief the name of the output file
	 */
	string m_sOutFileName;
	/** \var string m_sInFileName
	 *  \brief the name of the input file, NULL if stdin is used
	 */
	string m_sInFileName;
	/** \var bitset<PROGRAM_OPTIONS_MAX> m_Options
	 *  \brief the options which are specified with the compiler call
	 */
	static bitset<PROGRAM_OPTIONS_MAX> m_Options;
	/** \var ProgramFile_Type
	 *  \brief contains the options for the granularity of generated files
	 */
	static ProgramFile_Type m_FileOptions;
	/** \var bitset<PROGRAM_DEPEND_MAX> m_Depend
	 *  \brief contains the option to print dependencies
	 */
	static bitset<PROGRAM_DEPEND_MAX> m_Depend;
	/** \var BackEnd_Interface_Type m_BackEndInterface
	 *  \brief the back-end options regarding the interface
	 */
	static BackEnd_Interface_Type m_BackEndInterface;
	/** \var BackEnd_Platform_Type m_BackEndPlatform
	 *  \brief the back-end options regarding the platform
	 */
	static BackEnd_Platform_Type m_BackEndPlatform;
	/** \var BackEnd_Language_Type m_BackEndLanguage
	 *  \brief the back-end options regarding the language
	 */
	static BackEnd_Language_Type m_BackEndLanguage;
	/** \var int m_nOpcodeSize
	 *  \brief contains the size of the opcode type in bytes
	 */
	int m_nOpcodeSize;
	/** \var bool m_bVerbose
	 *  \brief is the same as (m_nOptions & PROGRAM_VERBOSE)
	 */
	bool m_bVerbose;
	/** \var int m_nVerboseInd
	 *  \brief indent level of verbose messages
	 */
	static int m_nVerboseInd;
	/** \var string m_sDependsFile
	 *  \brief the file to write depends output to if set
	 */
	string m_sDependsFile;
	/** \var int m_nDumpMsgBufDwords
	 *  \brief contains the number of dwords to dump
	 */
	static int m_nDumpMsgBufDwords;
	/** \var CBESizes *m_pSizes
	 *  \brief contains a reference to the sizes class of a traget architecture
	 */
	static CBESizes *m_pSizes;
	/** \var map<string, string> m_mBackEndOptions
	 *  \brief back-end options
	 */
	static map<string, string> m_mBackEndOptions;
	/** \var int errcount
	 *  \brief number of errors that occurred
	 */
	static int errcount;
	/** \var int warningcount
	 *  \brief number of warnings issued
	 */
	static int warningcount;
};

/** \brief set the option
 *  \param nOption the option in raw format
 */
inline
void
CCompiler::SetOption(ProgramOption_Type nOption)
{
	m_Options.set(nOption);
}

/** \brief unset an option
 *  \param nOption the option in raw format
 */
inline
void
CCompiler::UnsetOption(ProgramOption_Type nOption)
{
	m_Options.reset(nOption);
}

/** \brief tests if an option is set
 *  \param nOption the option in raw format
 */
inline
bool
CCompiler::IsOptionSet(ProgramOption_Type nOption)
{
	return m_Options.test(nOption);
}

/** \brief sets the dependency option
 *  \param nDepends the new dependency option
 *
 * This assumes that all exlusive dependency options have a value smaller that
 * the non-exclusive options, and the exclusive option with the largest value
 * is PROGRAM_DEPEND_MMD.
 */
inline
void
CCompiler::SetDependsOption(ProgramDepend_Type nDepends)
{
	if (nDepends <= PROGRAM_DEPEND_MMD)
	{
		/* first disable all exclusive options */
		m_Depend.reset(PROGRAM_DEPEND_M);
		m_Depend.reset(PROGRAM_DEPEND_MM);
		m_Depend.reset(PROGRAM_DEPEND_MD);
		m_Depend.reset(PROGRAM_DEPEND_MMD);
		/* then set new exclusive option */
	}
	/* for non-exclusive options, just set bit */
	m_Depend.set(nDepends);
}

/** \brief checks if dependency option is set
 *  \param nDepends option to check
 *  \return true if set
 */
inline
bool
CCompiler::IsDependsOptionSet(ProgramDepend_Type nDepends)
{
	return m_Depend.test(nDepends);
}

/** \brief sets the verboseness level
 *  \param nVerboseLevel the new verboseness level
 */
inline
void
CCompiler::SetVerboseLevel(ProgramVerbose_Type nVerboseLevel)
{
	m_VerboseLevel = nVerboseLevel;
}

/** \brief tests if a certain verbose level is set
 *  \param nVerboseLevel the verboseness level to test
 */
inline
bool
CCompiler::IsVerboseLevel(ProgramVerbose_Type nVerboseLevel)
{
	return nVerboseLevel <= m_VerboseLevel;
}

/** \brief set the backend interface to use
 *  \param nAdd the backend interface to use
 */
inline
void
CCompiler::SetBackEndInterface(BackEnd_Interface_Type nAdd)
{
	m_BackEndInterface = nAdd;
}

/** \brief set the backend platform to use
 *  \param nAdd the backend platform to use
 */
inline
void
CCompiler::SetBackEndPlatform(BackEnd_Platform_Type nAdd)
{
	m_BackEndPlatform = nAdd;
}

/** \brief set the backend language to use
 *  \param nAdd the backend language to use
 */
inline
void
CCompiler::SetBackEndLanguage(BackEnd_Language_Type nAdd)
{
	m_BackEndLanguage = nAdd;
}

/** \brief test the back-end option
 *  \param nOption the option to test
 *  \return true if option is set
 */
inline
bool
CCompiler::IsBackEndInterfaceSet(BackEnd_Interface_Type nOption)
{
	if (nOption == PROGRAM_BE_INTERFACE)
		return m_BackEndInterface > PROGRAM_BE_NONE_I;
	return m_BackEndInterface  == nOption;
}

/** \brief test the back-end option
 *  \param nOption the option to test
 *  \return true if option is set
 */
inline
bool
CCompiler::IsBackEndPlatformSet(BackEnd_Platform_Type nOption)
{
	if (nOption == PROGRAM_BE_PLATFORM)
		return m_BackEndPlatform > PROGRAM_BE_NONE_P;
	return m_BackEndPlatform  == nOption;
}

/** \brief test the back-end option
 *  \param nOption the option to test
 *  \return true if option is set
 */
inline
bool
CCompiler::IsBackEndLanguageSet(BackEnd_Language_Type nOption)
{
	if (nOption == PROGRAM_BE_LANGUAGE)
		return m_BackEndLanguage > PROGRAM_BE_NONE_L;
	return m_BackEndLanguage  == nOption;
}

/** \brief set the file granularity options
 *  \param nOption the new file granularity
 */
inline
void
CCompiler::SetFileOption(ProgramFile_Type nOption)
{
	m_FileOptions = nOption;
}

/** \brief test the file granularity option
 *  \param nOption the option to test
 *  \return true if the option is set
 */
inline
bool
CCompiler::IsFileOptionSet(ProgramFile_Type nOption)
{
	return m_FileOptions == nOption;
}

/** \brief set a warning level
 *  \param nLevel the new warning level to set
 */
inline
void
CCompiler::SetWarningLevel(ProgramWarning_Type nLevel)
{
	if (nLevel == PROGRAM_WARNING_ALL)
		m_WarningLevel.set();
	else
		m_WarningLevel.set(nLevel);
}

/** \brief unset a warning level
 *  \param nLevel the warning level to delete
 */
inline
void
CCompiler::UnsetWarningLevel(ProgramWarning_Type nLevel)
{
	if (nLevel == PROGRAM_WARNING_ALL)
		m_WarningLevel.reset();
	else
		m_WarningLevel.reset(nLevel);
}

/** \brief test for a specific warning level
 *  \param nLevel the level to test for
 *  \return true if level is set
 */
inline
bool
CCompiler::IsWarningSet(ProgramWarning_Type nLevel)
{
	if (nLevel == PROGRAM_WARNING_ALL)
		return m_WarningLevel.any();
	return m_WarningLevel.test(nLevel);
}

/** \brief sets the number of dwords to dump from the message buffer
 *  \param nDwords the number of dwords
 */
inline
void
CCompiler::SetTraceMsgBufDwords(int nDwords)
{
	m_nDumpMsgBufDwords = nDwords;
}

/** \brief get the number of dumpable dwords
 *  \return number of dwords
 */
inline
int
CCompiler::GetTraceMsgBufDwords()
{
	return m_nDumpMsgBufDwords;
}

/** \brief restrieves a reference to the sizes class
 *  \return a reference to the sizes class
 */
inline
CBESizes*
CCompiler::GetSizes()
{
	return m_pSizes;
}

/** \brief sets the sizes member
 *  \param pSizes the new sizes member
 */
inline
void
CCompiler::SetSizes(CBESizes *pSizes)
{
	m_pSizes = pSizes;
}

/** \brief set debug output on or off
 *  \param bOn true if on, otherwise off
 */
inline
void
CCompiler::SetDebug(bool bOn)
{
	if (bOn)
		m_VerboseLevel = PROGRAM_VERBOSE_DEBUG;
	else
		m_VerboseLevel = PROGRAM_VERBOSE_NONE;
}

/** \brief set the back-end option in the compiler
 *  \param sOption the option
 *  \param sValue the corresponding value
 */
inline
void
CCompiler::SetBackEndOption(const string sOption, const string sValue)
{
	map<string, string>::value_type e(sOption, sValue);
	if (m_mBackEndOptions.find(sOption) == m_mBackEndOptions.end())
		m_mBackEndOptions.insert(e);
	else
		m_mBackEndOptions[sOption] = sValue;
}

/** \brief try to retrieve the value for the given back-end option
 *  \param sOption the value to look for
 *  \retval sValue set to the respective option if found
 *  \return true if value was found, false if not
 *
 * The return value can be used to check for an option without a value
 * (although that should be handled with the bitmap).
 */
inline bool
CCompiler::GetBackEndOption(const string sOption, string& sValue)
{
	if (m_mBackEndOptions.find(sOption) == m_mBackEndOptions.end())
		return false;
	sValue = m_mBackEndOptions[sOption];
	return true;
}


#endif                // __DICE_COMPILER_H__
