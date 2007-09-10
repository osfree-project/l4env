/**
 *  \file    dice/src/be/BEFile.cpp
 *  \brief   contains the implementation of the class CBEFile
 *
 *  \date    01/10/2002
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

#include "BEFile.h"
#include "BEClass.h"
#include "BENameSpace.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BETarget.h"
#include "BEFunction.h"
#include "BEContext.h"
#include "BEConstant.h"
#include "BEType.h"
#include "BETypedef.h"
#include "IncludeStatement.h"
#include "Compiler.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>

//@{
/** some config variables */
extern const char* dice_build;
//@}

/** maimum possible indent */
const unsigned int CBEFile::MAX_INDENT = 80;
/** the standard indentation value */
const unsigned int CBEFile::STD_INDENT = 4;

CBEFile::CBEFile()
: m_Classes(0, (CObject*)0),
  m_NameSpaces(0, (CObject*)0),
  m_Functions(0, (CObject*)0),
  m_IncludedFiles(0, this)
{
    m_nFileType = FILETYPE_NONE;
}

/** \brief class destructor
 */
CBEFile::~CBEFile()
{ }

/** \brief writes user-defined and helper functions
 */
void CBEFile::WriteHelperFunctions()
{ }

/** \brief adds another filename to the list of included files
 *  \param sFileName the new filename
 *  \param bIDLFile true if the file is an IDL file
 *  \param bIsStandardInclude true if the file was included as standard include
 *         (using <>)
 *  \param pRefObj if not 0, it is used to set source file info
 */
void
CBEFile::AddIncludedFileName(string sFileName,
    bool bIDLFile,
    bool bIsStandardInclude,
    CObject* pRefObj)
{
	if (sFileName.empty())
		return;
	// first check if we have this name already registered
	if (m_IncludedFiles.Find(sFileName))
		return;
	// add new include file
	CIncludeStatement *pNewInc = new CIncludeStatement(bIDLFile,
		bIsStandardInclude, false, sFileName, string(),
		string(), 0);
	m_IncludedFiles.Add(pNewInc);
	if (pRefObj)
		pNewInc->m_sourceLoc = pRefObj->m_sourceLoc;
}

/** \brief tries to find the function with the given name
 *  \param sFunctionName the name to seach for
 *  \param nFunctionType the function type to look for
 *  \return a reference to the function or 0 if not found
 *
 * To find a function, we iterate over the classes and namespaces.
 */
CBEFunction *CBEFile::FindFunction(string sFunctionName,
    FUNCTION_TYPE nFunctionType)
{
	if (sFunctionName.empty())
		return 0;

	// classes
	CBEFunction *pFunction = 0;
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_Classes.begin();
		iterC != m_Classes.end();
		iterC++)
	{
		if ((pFunction = (*iterC)->FindFunction(sFunctionName,
					nFunctionType)) != 0)
			return pFunction;
	}
	// namespaces
	vector<CBENameSpace*>::iterator iterN;
	for (iterN = m_NameSpaces.begin();
		iterN != m_NameSpaces.end();
		iterN++)
	{
		if ((pFunction = (*iterN)->FindFunction(sFunctionName,
					nFunctionType)) != 0)
			return pFunction;
	}
	// search own functions
	vector<CBEFunction*>::iterator iterF;
	for (iterF = m_Functions.begin();
		iterF != m_Functions.end();
		iterF++)
	{
		if ((*iterF)->GetName() == sFunctionName &&
			(*iterF)->IsFunctionType(nFunctionType))
			return *iterF;
	}
	// no match found -> return 0
	return 0;
}

/** \brief writes includes, which always have to be there
 */
void CBEFile::WriteDefaultIncludes()
{ }

/** \brief writes one include statement
 *  \param pInclude the include statement to write
 */
void CBEFile::WriteInclude(CIncludeStatement *pInclude)
{
	string sPrefix;
	CCompiler::GetBackEndOption(string("include-prefix"), sPrefix);
	string sFileName = pInclude->m_sFilename;
	if (!sFileName.empty())
	{
		if (pInclude->m_bStandard)
			*this << "#include <";
		else
			*this << "#include \"";
		if (!pInclude->m_bIDLFile || sPrefix.empty())
		{
			*this << sFileName;
		}
		else // bIDLFile && !sPrefix.empty()()
		{
			*this << sPrefix << "/" << sFileName;
		}
		if (pInclude->m_bStandard)
			*this << ">\n";
		else
			*this << "\"\n";
	}
}

/** \brief tries to find a class using its name
 *  \param sClassName the name of the searched class
 *  \param pPrev previously found class
 *  \return a reference to the found class or 0 if not found
 */
CBEClass* CBEFile::FindClass(string sClassName, CBEClass *pPrev)
{
	// first search classes
	CBEClass *pClass = m_Classes.Find(sClassName, pPrev);
	if (pClass)
		return pClass;
	// then search namespaces
	vector<CBENameSpace*>::iterator iterN;
	for (iterN = m_NameSpaces.begin();
		iterN != m_NameSpaces.end();
		iterN++)
	{
		if ((pClass = (*iterN)->FindClass(sClassName, pPrev)) != 0)
			return pClass;
	}
	// not found
	return 0;
}

/** \brief tries to find a namespace using a name
 *  \param sNameSpaceName the name of the searched namespace
 *  \return a reference to the found namespace or 0 if none found
 */
CBENameSpace* CBEFile::FindNameSpace(string sNameSpaceName)
{
	// search the namespace
	CBENameSpace *pNameSpace = m_NameSpaces.Find(sNameSpaceName);
	if (pNameSpace)
		return pNameSpace;
	// search nested namespaces
	CBENameSpace *pFoundNameSpace = 0;
	vector<CBENameSpace*>::iterator iterN;
	for (iterN = m_NameSpaces.begin();
		iterN != m_NameSpaces.end();
		iterN++)
	{
		if ((pFoundNameSpace = (*iterN)->FindNameSpace(sNameSpaceName)) != 0)
			return pFoundNameSpace;
	}
	// nothing found
	return 0;
}

/** \brief returns the number of functions in the function vector
 *  \return the number of functions in the function vector
 */
int CBEFile::GetFunctionCount()
{
    return m_Functions.size();
}

/** \brief test if the target file is of a certain type
 *  \param nFileType the file type to test for
 *  \return true if the file is of this type
 *
 * A special condition is the test for FILETYPE_CLIENT or FILETYPE_COMPONENT,
 * because they have to test for both, header and implementation file.
 */
bool CBEFile::IsOfFileType(FILE_TYPE nFileType)
{
	if (m_nFileType == nFileType)
		return true;
	if ((nFileType == FILETYPE_CLIENT) &&
		((m_nFileType == FILETYPE_CLIENTHEADER) ||
		 (m_nFileType == FILETYPE_CLIENTIMPLEMENTATION)))
		return true;
	if ((nFileType == FILETYPE_COMPONENT) &&
		((m_nFileType == FILETYPE_COMPONENTHEADER) ||
		 (m_nFileType == FILETYPE_COMPONENTIMPLEMENTATION)))
		return true;
	if ((nFileType == FILETYPE_HEADER) &&
		((m_nFileType == FILETYPE_CLIENTHEADER) ||
		 (m_nFileType == FILETYPE_COMPONENTHEADER) ||
		 (m_nFileType == FILETYPE_OPCODE)))
		return true;
	if ((nFileType == FILETYPE_IMPLEMENTATION) &&
		((m_nFileType == FILETYPE_CLIENTIMPLEMENTATION) ||
		 (m_nFileType == FILETYPE_COMPONENTIMPLEMENTATION)))
		return true;
	return false;
}

/** \brief test if this file contains a function with a given type
 *  \param sTypeName the name of the tye to search for
 *  \return true if a parameter of that type is found
 */
bool CBEFile::HasFunctionWithUserType(string sTypeName)
{
	vector<CBENameSpace*>::iterator iterN;
	for (iterN = m_NameSpaces.begin();
		iterN != m_NameSpaces.end();
		iterN++)
	{
		if ((*iterN)->HasFunctionWithUserType(sTypeName, this))
			return true;
	}
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_Classes.begin();
		iterC != m_Classes.end();
		iterC++)
	{
		if ((*iterC)->HasFunctionWithUserType(sTypeName, this))
			return true;
	}
	vector<CBEFunction*>::iterator iterF;
	for (iterF = m_Functions.begin();
		iterF != m_Functions.end();
		iterF++)
	{
		if (dynamic_cast<CBEHeaderFile*>(this) &&
			(*iterF)->DoWriteFunction((CBEHeaderFile*)this) &&
			(*iterF)->FindParameterType(sTypeName))
			return true;
		if (dynamic_cast<CBEImplementationFile*>(this) &&
			(*iterF)->DoWriteFunction((CBEImplementationFile*)this) &&
			(*iterF)->FindParameterType(sTypeName))
			return true;
	}
	return false;
}

/** \brief writes an introductionary notice
 *
 * This method should always be called first when writing into
 * a file.
 */
void CBEFile::WriteIntro()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEFile::%s called\n", __func__);

	*this <<
		"/*\n" <<
		" * THIS FILE IS MACHINE GENERATED!";
	if (m_nFileType == FILETYPE_TEMPLATE)
	{
		*this << "\n" <<
			" *\n" <<
			" * Implement the server templates here.\n" <<
			" * This file is regenerated with every run of 'dice -t ...'.\n" <<
			" * Move it to another location after modifications to\n" <<
			" * keep your changes!\n";
	}
	else
		*this << " DO NOT EDIT!\n";
	*this << " *\n";
	*this << " * " << m_sFilename << "\n";
	*this << " * created ";

	char * user = getlogin();
	if (user)
	{
		*this << "by " << user;

		char host[255];
		if (!gethostname(host, sizeof(host)))
			*this << "@" << host;

		*this << " ";
	}

	time_t t = time(0);
	*this << "on " << ctime(&t);
	*this << " * with Dice version " << PACKAGE_VERSION << " (compiled on " <<
		dice_build << ")\n";
	*this << " * send bug reports to <" << PACKAGE_BUGREPORT << ">\n";
	*this << " */\n\n";
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBEFile::CreateOrderedElementList()
{
	// clear vector
	m_vOrderedElements.clear();
	// Includes
	vector<CIncludeStatement*>::iterator iterI;
	for (iterI = m_IncludedFiles.begin();
		iterI != m_IncludedFiles.end();
		iterI++)
	{
		InsertOrderedElement(*iterI);
	}
	// namespaces
	vector<CBENameSpace*>::iterator iterN;
	for (iterN = m_NameSpaces.begin();
		iterN != m_NameSpaces.end();
		iterN++)
	{
		InsertOrderedElement(*iterN);
	}
	// classes
	vector<CBEClass*>::iterator iterC;
	for (iterC = m_Classes.begin();
		iterC != m_Classes.end();
		iterC++)
	{
		InsertOrderedElement(*iterC);
	}
	// functions
	vector<CBEFunction*>::iterator iterF;
	for (iterF = m_Functions.begin();
		iterF != m_Functions.end();
		iterF++)
	{
		InsertOrderedElement(*iterF);
	}
}

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBEFile::InsertOrderedElement(CObject *pObj)
{
	// get source line number
	unsigned int nLine = pObj->m_sourceLoc.getBeginLine();
	// search for element with larger number
	vector<CObject*>::iterator iter = m_vOrderedElements.begin();
	for (; iter != m_vOrderedElements.end(); iter++)
	{
		if ((*iter)->m_sourceLoc.getBeginLine() > nLine)
		{
			// insert before that element
			m_vOrderedElements.insert(iter, pObj);
			return;
		}
	}
	// new object is bigger that all existing
	m_vOrderedElements.push_back(pObj);
}

/** \brief specializaion of stream operator for strings
 *  \param s string parameter
 *
 * Here we implement the special treatment for indentation. To make it right
 * we have to check if there are tabs after line-breaks.
 */
std::ofstream& CBEFile::operator<<(string s)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEFile::%s(str:\"%s\") called\n", __func__, s.c_str());

	if (s.empty())
		return *this;

	if (s[0] == '\t')
	{
		PrintIndent();
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEFile::%s(str) print substr after indent\n", __func__);
		*this << s.substr(1);
		return *this;
	}

	string::size_type pos = s.find('\n');
	if (pos != string::npos &&
		pos != s.length())
	{
		/* first print everything up to \n */
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEFile::%s(str) print substr 1\n", __func__);
		write(s.substr(0, pos + 1).c_str(), s.substr(0, pos + 1).length());

		/* then call ourselves with the rest */
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEFile::%s(str) print substr 2\n", __func__);
		*this << s.substr(pos + 1);
		return *this;
	}

	/* simple string */
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEFile::%s(str) calling base class\n", __func__);
	write(s.c_str(), s.length());
	return *this;
}

/** \brief specialization of stream operator for character arrays
 *  \param s character array to print
 */
std::ofstream& CBEFile::operator<<(char const * s)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEFile::%s(char const:%s) called\n", __func__, s);

    this->operator<<(string(s));
    return *this;
}

/** \brief specialization of stream operator for character arrays
 *  \param s character array to print
 */
std::ofstream& CBEFile::operator<<(char* s)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEFile::%s(char:%s) called\n", __func__, s);

    this->operator<<(string(s));
    return *this;
}

/** \brief prints the indentation
 */
void CBEFile::PrintIndent()
{
    for (unsigned int i = 0; i < m_nIndent; i++)
	this->operator<<(" ");
}

/** \brief increases the identation for this file
 *
 * The standard value to increase the ident is specified in the constant
 * STD_INDENT.  If the ident reaches the values specified in MAX_IDENT it
 * ignores the ident increase.
 */
CBEFile& CBEFile::operator++()
{
    return this->operator+=(STD_INDENT);
}

/** \brief increases the identation for this file
 *  \param by the number of characters, the ident should be increased.
 *
 * The standard value to increase the ident is specified in the constant
 * STD_INDENT.  If the ident reaches the values specified in MAX_IDENT it
 * ignores the ident increase.
 */
CBEFile& CBEFile::operator+=(int by)
{
    m_nLastIndent = (m_nIndent + by > MAX_INDENT) ? MAX_INDENT - m_nIndent : by;
    m_nIndent = std::min(m_nIndent + by, MAX_INDENT);
    return *this;
}

/** \brief decreases the ident
 *
 * The standard value for the decrement operation is STD_IDENT. If the ident
 * reaches zero (0) the operation ignores the decrement.  If by is -1 the
 * indent is decremented by the value of the last increment.
 */
CBEFile& CBEFile::operator--()
{
    return this->operator-=(STD_INDENT);
}

/** \brief decreases the ident
 *  \param by the number of character, by which the ident should be decreased
 *
 * The standard value for the decrement operation is STD_IDENT. If the ident
 * reaches zero (0) the operation ignores the decrement.  If by is -1 the
 * indent is decremented by the value of the last increment.
 */
CBEFile& CBEFile::operator-=(int by)
{
    m_nIndent = std::max((int)m_nIndent - ((by == -1) ? m_nLastIndent : by), 0);
    m_nLastIndent = 0;
    return *this;
}

