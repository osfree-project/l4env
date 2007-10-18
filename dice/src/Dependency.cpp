/**
 *  \file    dice/src/Dependency.cpp
 *  \brief   contains the implementation of the class CDependency
 *
 *  \date    08/15/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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

#include "Dependency.h"
#include "Compiler.h"
#include "Messages.h"
#include "ProgramOptions.h"
#include "be/BENameFactory.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "be/BEContext.h"
#include "be/BERoot.h"

#include <cerrno>
#include <cassert>
#include <iostream>
#include <climits>
#include <cstdlib>
#include <cstring>

/** defines the number of characters for dependency output per line */
#define MAX_SHELL_COLS 80

CDependency::CDependency(string sFile,
	CFEFile *pFERoot,
	CBERoot *pBERoot)
: m_sDependsFile(sFile)
{
	m_pRootBE = pBERoot;
	m_pRootFE = pFERoot;
	m_nCurCol = 0;
	m_output = &std::cout;
}

/** \brief print the dependency tree
 *
 * The dependency tree contains the files the top IDL file depends on. These
 * are the included files.
 */
void CDependency::PrintDependencies()
{
	assert (m_pRootBE);

	std::ofstream *of = new std::ofstream();
	// if file, open file
	if (CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MD) ||
		CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MMD) ||
		CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MF))
	{
		string sOutName;
		CCompiler::GetBackEndOption(string("output-dir"), sOutName);
		if (CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MF))
			sOutName += m_sDependsFile;
		else
			sOutName += m_pRootFE->GetFileNameWithoutExtension() + ".d";
		of->open(sOutName.c_str());
		if (!of->is_open())
		{
			CMessages::Warning("Could not open %s, use <stdout>\n", sOutName.c_str());
		}
		else
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"%s: opened \"%s\" for output\n", __func__, sOutName.c_str());
			m_output = of;
		}
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: start printing target files\n", __func__);
	m_nCurCol = 0;
	m_pRootBE->PrintTargetFiles(*m_output, m_nCurCol, MAX_SHELL_COLS);
	*m_output << ": ";
	m_nCurCol += 2;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: start with files\n", __func__);
	PrintDependentFile(m_pRootFE->GetFullFileName());
	PrintDependencyTree(m_pRootFE);
	*m_output << "\n\n";

	// if phony dependencies are set then print them now
	if (CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MP))
	{
		vector<string>::iterator iter = m_vPhonyDependencies.begin();
		for (; iter != m_vPhonyDependencies.end(); iter++)
		{
			*m_output << *iter << ": \n\n";
		}
	}

	// if file, close file
	if ((CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MD) ||
			CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MMD)) &&
		of->is_open())
		of->close();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: finished\n", __func__);
}

/** \brief prints the included files
 *  \param pFEFile the current front-end file
 *
 * This implementation print the file names of the included files. It first
 * prints the names and then iterates over the included files.
 *
 * If the options PROGRAM_DEPEND_MM or PROGRAM_DEPEND_MMD are specified only
 * files included with '\#include "file"' are printed.
 */
void CDependency::PrintDependencyTree(CFEFile * pFEFile)
{
	if (!pFEFile)
		return;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for \"%s\" called\n", __func__,
		pFEFile->GetFileName().c_str());
	// print names
	vector<CFEFile*>::iterator iterF;
	for (iterF = pFEFile->m_ChildFiles.begin();
		iterF != pFEFile->m_ChildFiles.end();
		iterF++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"%s: checking child file \"%s\" as directly included\n",
			__func__, (*iterF)->GetFileName().c_str());
		if ((*iterF)->IsStdIncludeFile() &&
			(CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MM) ||
			 CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MMD)))
			continue;
		PrintDependentFile((*iterF)->GetFullFileName());
	}
	// ierate over included files
	for (iterF = pFEFile->m_ChildFiles.begin();
		iterF != pFEFile->m_ChildFiles.end();
		iterF++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"%s: checking child file \"%s\" for trees\n", __func__,
			(*iterF)->GetFileName().c_str());
		if ((*iterF)->IsStdIncludeFile() &&
			(CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MM) ||
			 CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MMD)))
			continue;
		PrintDependencyTree(*iterF);
	}
}

/** \brief prints a list of generated files for the given options
 *  \param pFEFile the file to scan for generated files
 */
void CDependency::PrintGeneratedFiles(CFEFile * pFEFile)
{
	if (!pFEFile->IsIDLFile())
		return;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for \"%s\" called\n", __func__,
		pFEFile->GetFileName().c_str());

	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_IDLFILE) ||
		CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
	{
		PrintGeneratedFiles4File(pFEFile);
	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_MODULE))
	{
		PrintGeneratedFiles4Library(pFEFile);
	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
	{
		PrintGeneratedFiles4Interface(pFEFile);
	}
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION))
	{
		PrintGeneratedFiles4Operation(pFEFile);
	}

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;
	// create client header file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		sName = pNF->GetFileName(pFEFile, FILETYPE_CLIENTHEADER);
		PrintDependentFile(sName);
	}
	// create server header file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// component file
		sName = pNF->GetFileName(pFEFile, FILETYPE_COMPONENTHEADER);
		PrintDependentFile(sName);
	}
	// opcodes
	if (!CCompiler::IsOptionSet(PROGRAM_NO_OPCODES))
	{
		sName = pNF->GetFileName(pFEFile, FILETYPE_OPCODE);
		PrintDependentFile(sName);
	}

	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
	{
		vector<CFEFile*>::iterator iter;
		for (iter = pFEFile->m_ChildFiles.begin();
			iter != pFEFile->m_ChildFiles.end();
			iter++)
		{
			PrintGeneratedFiles(*iter);
		}
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done\n", __func__);
}

/** \brief prints the file-name generated for the front-end file
 *  \param pFEFile the current front-end file
 */
void CDependency::PrintGeneratedFiles4File(CFEFile * pFEFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for %s called\n", __func__,
		pFEFile->GetFileName().c_str());

	// client
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		// client implementation file
		sName = pNF->GetFileName(pFEFile, FILETYPE_CLIENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// server
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// component file
		sName = pNF->GetFileName(pFEFile, FILETYPE_COMPONENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief prints a list of generated files for the library granularity
 *  \param pFEFile the front-end file to scan for generated files
 */
void CDependency::PrintGeneratedFiles4Library(CFEFile * pFEFile)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for file \"%s\" called\n",
		__func__, pFEFile->GetFileName().c_str());

	// iterate over libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFEFile->m_Libraries.begin();
		iterL != pFEFile->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Library(*iterL);
	}
	// iterate over interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEFile->m_Interfaces.begin();
		iterI != pFEFile->m_Interfaces.end();
		iterI++)
	{
		PrintGeneratedFiles4Library(*iterI);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done\n", __func__);
}

/** \brief print the list of generated files for the library granularity
 *  \param pFELibrary the library to scan
 */
void CDependency::PrintGeneratedFiles4Library(CFELibrary * pFELibrary)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for lib \"%s\" called\n", __func__,
		pFELibrary->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;
	// client file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		// implementation
		sName = pNF->GetFileName(pFELibrary, FILETYPE_CLIENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// component file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// implementation
		sName = pNF->GetFileName(pFELibrary, FILETYPE_COMPONENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFELibrary->m_Libraries.begin();
		iterL != pFELibrary->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Library(*iterL);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for the library granularity
 *  \param pFEInterface the interface to scan
 */
void CDependency::PrintGeneratedFiles4Library(CFEInterface * pFEInterface)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for interface \"%s\" called\n", __func__,
		pFEInterface->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;
	// client file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEInterface, FILETYPE_CLIENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// component file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEInterface, FILETYPE_COMPONENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for interface granularity
 *  \param pFEFile the front-end file to be scanned
 */
void CDependency::PrintGeneratedFiles4Interface(CFEFile * pFEFile)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for file \"%s\" called\n",
		__func__, pFEFile->GetFileName().c_str());

	// iterate over interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEFile->m_Interfaces.begin();
		iterI != pFEFile->m_Interfaces.end();
		iterI++)
	{
		PrintGeneratedFiles4Interface(*iterI);
	}
	// iterate over libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFEFile->m_Libraries.begin();
		iterL != pFEFile->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Interface(*iterL);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for interface granularity
 *  \param pFELibrary the front-end library to be scanned
 */
void CDependency::PrintGeneratedFiles4Interface(
	CFELibrary * pFELibrary)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for lib \"%s\" called\n", __func__,
		pFELibrary->GetName().c_str());

	// iterate over interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFELibrary->m_Interfaces.begin();
		iterI != pFELibrary->m_Interfaces.end();
		iterI++)
	{
		PrintGeneratedFiles4Interface(*iterI);
	}
	// iterate over nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFELibrary->m_Libraries.begin();
		iterL != pFELibrary->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Interface(*iterL);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for interface granularity
 *  \param pFEInterface the front-end interface to be scanned
 */
void CDependency::PrintGeneratedFiles4Interface(
	CFEInterface * pFEInterface)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for interface \"%s\" called\n", __func__,
		pFEInterface->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;
	// client file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEInterface, FILETYPE_CLIENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// component file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEInterface, FILETYPE_COMPONENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for operation granularity
 *  \param pFEFile the front-end file to be scanned
 */
void CDependency::PrintGeneratedFiles4Operation(CFEFile * pFEFile)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for file \"%s\" called\n",
		__func__, pFEFile->GetFileName().c_str());

	// iterate over interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEFile->m_Interfaces.begin();
		iterI != pFEFile->m_Interfaces.end();
		iterI++)
	{
		PrintGeneratedFiles4Operation(*iterI);
	}
	// iterate over libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFEFile->m_Libraries.begin();
		iterL != pFEFile->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Operation(*iterL);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for operation granularity
 *  \param pFELibrary the front-end library to be scanned
 */
void CDependency::PrintGeneratedFiles4Operation(
	CFELibrary * pFELibrary)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: for lib \"%s\" called\n",
		__func__, pFELibrary->GetName().c_str());

	// iterate over interfaces
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFELibrary->m_Interfaces.begin();
		iterI != pFELibrary->m_Interfaces.end();
		iterI++)
	{
		PrintGeneratedFiles4Operation(*iterI);
	}
	// iterate over nested libraries
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFELibrary->m_Libraries.begin();
		iterL != pFELibrary->m_Libraries.end();
		iterL++)
	{
		PrintGeneratedFiles4Operation(*iterL);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done\n", __func__);
}

/** \brief print the list of generated files for operation granularity
 *  \param pFEInterface the front-end interface to be scanned
 */
void CDependency::PrintGeneratedFiles4Operation(
	CFEInterface * pFEInterface)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for interface \"%s\" caled\n", __func__,
		pFEInterface->GetName().c_str());

	// iterate over operations
	vector<CFEOperation*>::iterator iter;
	for (iter = pFEInterface->m_Operations.begin();
		iter != pFEInterface->m_Operations.end();
		iter++)
	{
		PrintGeneratedFiles4Operation(*iter);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief print the list of generated files for operation granularity
 *  \param pFEOperation the front-end operation to be scanned
 */
void CDependency::PrintGeneratedFiles4Operation(
	CFEOperation * pFEOperation)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s: for op \"%s\" called\n", __func__,
		pFEOperation->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName;
	// client file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEOperation, FILETYPE_CLIENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}
	// component file
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		// implementation
		sName = pNF->GetFileName(pFEOperation, FILETYPE_COMPONENTIMPLEMENTATION);
		PrintDependentFile(sName);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: done.\n", __func__);
}

/** \brief prints a filename to the dependency tree
 *  \param sFileName the name to print
 *
 * This implementation adds a spacer after each file name and also checks
 * before writing if the maximum column number would be exceeded by this file.
 * If it would a new line is started The length of the filename is added to
 * the columns.
 */
void CDependency::PrintDependentFile(string sFileName)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s) called\n", __func__, sFileName.c_str());

	char real_path_buffer[PATH_MAX];
	/* Cite: Avoid using this function. It is broken by design... */
	char *real_path = realpath(sFileName.c_str(), real_path_buffer);
	if (!real_path)
	{
		CMessages::Error("Calling realpath(%s) returned an error: %s\n",
			sFileName.c_str(), strerror(errno));
	}

	size_t nLength = strlen(real_path);
	if ((m_nCurCol + nLength + 1) >= MAX_SHELL_COLS)
	{
		*m_output << "\\\n ";
		m_nCurCol = 1;
	}
	*m_output << real_path << " ";
	m_nCurCol += static_cast<int>(nLength) + 1;

	// if phony dependency option is set, add this file to phony dependency
	// list
	if (CCompiler::IsDependsOptionSet(PROGRAM_DEPEND_MP))
		m_vPhonyDependencies.push_back(string(real_path));

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s done\n", __func__);
}

