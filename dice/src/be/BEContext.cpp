/**
 * \file dice/src/be/BEContext.cpp
 * \brief contains the implementation of the class CBEContext
 *
 * \date 01/10/2002
 * \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "Compiler.h"
#include <string.h>

IMPLEMENT_DYNAMIC(CBEContext)

CBEContext::CBEContext(CBEClassFactory * pCF, CBENameFactory * pNF)
{
    m_pClassFactory = pCF;
    m_pNameFactory = pNF;
    m_pSizes = 0;
    m_nOptions = PROGRAM_NONE;
    m_nBackEnd = 0;
    m_nFunctionType = 0;
    m_nFileType = 0;
    m_nWarningLevel = 0;
	m_sSymbols = 0;
	m_nSymbolCount = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEContext, CBEObject);
}

CBEContext::CBEContext(CBEContext & src):CBEObject(src)
{
    m_pClassFactory = src.m_pClassFactory;
    m_pNameFactory = src.m_pNameFactory;
    m_pSizes = src.m_pSizes;
    m_nOptions = src.m_nOptions;
    m_nBackEnd = src.m_nBackEnd;
    m_sFilePrefix = src.m_sFilePrefix;
    m_sIncludePrefix = src.m_sIncludePrefix;
    m_nFunctionType = src.m_nFunctionType;
    m_nFileType = src.m_nFileType;
    m_nWarningLevel = src.m_nWarningLevel;
	m_nSymbolCount = 0;
	m_sSymbols = 0;
	for (int i=0; i<src.m_nSymbolCount-1; i++)
	    AddSymbol(src.m_sSymbols[i]);
    IMPLEMENT_DYNAMIC_BASE(CBEContext, CBEObject);
}

/** CBEContext destructor */
CBEContext::~CBEContext()
{
    if (m_pSizes)
        delete m_pSizes;
    for (int i=0; i<m_nSymbolCount-1; i++)
	    free(m_sSymbols[i]);
    free(m_sSymbols);
}

/**
 * \brief returns a reference to the class factory
 * \return the reference to the class-factory
 */
CBEClassFactory *CBEContext::GetClassFactory()
{
    return m_pClassFactory;
}

/**
 * \brief returns a reference to the name factory
 * \return the reference to the name factory
 */
CBENameFactory *CBEContext::GetNameFactory()
{
    return m_pNameFactory;
}

/**
 * \brief changes the options of the context
 * \param nAdd the options to add
 * \param nRemove the options to remove
 *
 * The options are usually flags indication which parameters have been used to call
 * the IDL compiler.
 */
void CBEContext::ModifyOptions(ProgramOptionType nAdd, ProgramOptionType nRemove)
{
    m_nOptions = ((m_nOptions & (~nRemove)) | nAdd);
}

/**
 * \brief check if a specific option is set
 * \param nOption the value of the option to check
 * \return true if the option is set, false if not
 */
bool CBEContext::IsOptionSet(ProgramOptionType nOption)
{
    return (m_nOptions & nOption) > 0;
}

/**
 * \brief special implementation of IsOptionSet(PROGRAM_VERBOSE)
 * \return true if verbose is set, false if not
 */
bool CBEContext::IsVerbose()
{
    return (m_nOptions & PROGRAM_VERBOSE) > 0;
}

/**
 * \brief set the string for the file prefix
 * \param sFilePrefix the new prefix
 *
 * Frees the memory of the old prefix and duplicates the new prefix.
 * Dispose the the new prefix reference yourself.
 */
void CBEContext::SetFilePrefix(String sFilePrefix)
{
    m_sFilePrefix = sFilePrefix;
}

/**
 * \brief return the string of the file prefix
 * \return the m_sFilePrefix pointer
 */
String CBEContext::GetFilePrefix()
{
    return m_sFilePrefix;
}

/** sets the include prefix for this context
 * \param sIncludePrefix the new prefix string
 */
void CBEContext::SetIncludePrefix(String sIncludePrefix)
{
    m_sIncludePrefix = sIncludePrefix;
}

/** returns the string for the include path prefixing
 * \return the include prefix string
 */
String CBEContext::GetIncludePrefix()
{
    return m_sIncludePrefix;
}

/** \brief returns the function type
 * \return the function type
 */
int CBEContext::GetFunctionType()
{
    return m_nFunctionType;
}

/** \brief sets the function type
 * \param nNewType the new function type
 * \return old function type
 */
int CBEContext::SetFunctionType(int nNewType)
{
    int nRet = m_nFunctionType;
    m_nFunctionType = nNewType;
    return nRet;
}

/** \brief sets the file type
 * \param nFileType the new file type
 * \return the old file type
 */
int CBEContext::SetFileType(int nFileType)
{
    int nRet = m_nFileType;
    m_nFileType = nFileType;
    return nRet;
}

/** \brief returns the file type
 * \return the file type
 *
 * The file type is used to distinguish the current target file. Because the current file of a function can be a header or
 * implementation file, the function has no way to distinguish between a client-header or component-header file. To allow
 * this distinction we use this file type.
 */
int CBEContext::GetFileType()
{
    return m_nFileType;
}

/** \brief Read property of int m_nOptimizeLevel.
 *  \return the current value of m_nOptimizeLevel
 */
int CBEContext::GetOptimizeLevel()
{
    return m_nOptimizeLevel;
}

/** \brief Write property of int m_nOptimizeLevel.
 *  \param nNewLevel the new optimize level
 */
void CBEContext::SetOptimizeLevel(int nNewLevel)
{
    m_nOptimizeLevel = nNewLevel;
}

/** \brief Read property of int m_nWarningLevel.
 *  \return the current value of m_nWarningLevel
 */
unsigned long CBEContext::GetWarningLevel()
{
    return m_nWarningLevel;
}

/** \brief modifies property of unsigned long m_nOptimizeLevel.
 *  \param nAdd the options to add
 *  \param nRemove the options to remove
 */
void CBEContext::ModifyWarningLevel(unsigned long nAdd, unsigned long nRemove)
{
    m_nWarningLevel = ((m_nWarningLevel & ~nRemove) | nAdd);
}

/** \brief test if a certain warning level is turned on
 *  \param nLevel the level to test for
 *  \return true if level is set
 */
bool CBEContext::IsWarningSet(unsigned long nLevel)
{
    return (m_nWarningLevel & nLevel) > 0;
}

/** \brief modifies the back-end option
 *  \param nAdd the options to add
 *  \param nRemove the optiosn to remove
 */
void CBEContext::ModifyBackEnd(unsigned long nAdd, unsigned long nRemove)
{
    m_nBackEnd = ((m_nBackEnd & ~nRemove) | nAdd);
}

/** \brief test the back-end option
 *  \param nOption the option to test
 *  \return true if option is set
 */
bool CBEContext::IsBackEndSet(unsigned long nOption)
{
    return (m_nBackEnd & nOption) > 0;
}

/** \brief restrieves a reference to the sizes class
 *  \return a reference to the sizes class
 */
CBESizes* CBEContext::GetSizes()
{
    if (!m_pSizes)
    {
        m_pSizes = m_pClassFactory->GetNewSizes();
        if (m_nOpcodeSize > 0)
            m_pSizes->SetOpcodeSize(m_nOpcodeSize);
    }
    return m_pSizes;
}

/** \brief sets the value of the m_nOpcodeSize member
 *  \param nSize the new value;
 */
void CBEContext::SetOpcodeSize(int nSize)
{
    m_nOpcodeSize = nSize;
}

/** \brief sets the name of the init-rcv-string function
 *  \param sName the name
 */
void CBEContext::SetInitRcvStringFunc(String sName)
{
    m_sInitRcvStringFunc = sName;
}

/** \brief retrieves the name of the init-rcv-string function
 *  \return the name
 */
String CBEContext::GetInitRcvStringFunc()
{
    return m_sInitRcvStringFunc;
}

/** \brief adds another symbol to the internal list
 *  \param sNewSymbol the symbol to add
 */
void CBEContext::AddSymbol(const char *sNewSymbol)
{
    if (!sNewSymbol)
        return;
    if (!m_sSymbols)
        m_nSymbolCount = 2;
    else
        m_nSymbolCount++;
    m_sSymbols = (char **) realloc(m_sSymbols, m_nSymbolCount * sizeof(char *));
    m_sSymbols[m_nSymbolCount - 2] = strdup(sNewSymbol);
    m_sSymbols[m_nSymbolCount - 1] = 0;
}

/** \brief adds another symbol to the internal list
 *  \param sNewSymbol the symbol to add
 */
void CBEContext::AddSymbol(String sNewSymbol)
{
    if (sNewSymbol.IsEmpty())
	    return;
    AddSymbol((const char*)sNewSymbol);
}

/** \brief checks if the symbol has been defined
 *  \param sSymbol the symbol to check for
 *  \return true if the symbol could be found
 *
 * The symbol is found if either the whole string matches or
 * teh string matches to the first '=' character.
 */
bool CBEContext::HasSymbol(const char *sSymbol)
{
    String sS1(sSymbol);
	int nPos = sS1.Find('=');
	bool bIgnoreValue = true;
	if (nPos >= 0)
		bIgnoreValue = false;
    for (int i=0; i<m_nSymbolCount-1; i++)
	{
        String sS2(m_sSymbols[i]);
		if (bIgnoreValue)
		{
		    nPos = sS2.Find('=');
		    if (nPos >= 0)
    		    sS2 = sS2.Left(nPos);
	    }
		if (sS1 == sS2)
		    return true;
    }
	return false;
}

/** \brief sets the name of the trace function
 *  \param sName the name
 */
void CBEContext::SetTraceClientFunc(String sName)
{
    m_sTraceClientFunc = sName;
}

/** \brief retrieves the name of the Trace function
 *  \return the name
 */
String CBEContext::GetTraceClientFunc()
{
    return m_sTraceClientFunc;
}

/** \brief sets the name of the trace function
 *  \param sName the name
 */
void CBEContext::SetTraceServerFunc(String sName)
{
    m_sTraceServerFunc = sName;
}

/** \brief retrieves the name of the Trace function
 *  \return the name
 */
String CBEContext::GetTraceServerFunc()
{
    return m_sTraceServerFunc;
}

/** \brief sets the name of the trace function
 *  \param sName the name
 */
void CBEContext::SetTraceMsgBufFunc(String sName)
{
    m_sTraceMsgBufFunc = sName;
}

/** \brief retrieves the name of the Trace function
 *  \return the name
 */
String CBEContext::GetTraceMsgBufFunc()
{
    return m_sTraceMsgBufFunc;
}

/** \brief sets the number of dwords to dump from the message buffer
 *  \param nDwords the number of dwords
 */
void CBEContext::SetTraceMsgBufDwords(int nDwords)
{
    m_nDumpMsgBufDwords = nDwords;
}

/** \brief retrieves the nunmber of dwords to dump from the message buffer
 *  \return the number of dwords
 */
int CBEContext::GetTraceMsgBufDwords()
{
    return m_nDumpMsgBufDwords;
}

/** \brief writes the actually used malloc function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *
 * We use CORBA_alloc if it is explicetly forced, or if the function
 * is used at the client's side, or if used at the server's side only
 * if the -fserver-parameter option is set. Only then do we have an
 * environment which might contain a valid malloc member.
 *
 * Another option is to force the usage of env.malloc.
 */
void CBEContext::WriteMalloc(CBEFile* pFile, CBEFunction* pFunction)
{
    bool bUseMalloc = !IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC) &&
	    (pFunction && (!pFunction->IsComponentSide() ||
		               (pFunction->IsComponentSide() && IsOptionSet(PROGRAM_SERVER_PARAMETER))));
    bUseMalloc |= IsOptionSet(PROGRAM_FORCE_ENV_MALLOC);
	if (bUseMalloc)
	{
		CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
		CBEDeclarator *pDecl = 0;
		if (pEnv)
		{
			VectorElement* pIter = pEnv->GetFirstDeclarator();
			pDecl = pEnv->GetNextDeclarator(pIter);
		}
		if (pDecl)
		{
			pFile->Print("(%s", (const char*)pDecl->GetName());
			if (pDecl->GetStars())
				pFile->Print("->malloc)");
			else
				pFile->Print(".malloc)");
			if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
				CCompiler::Warning("CORBA_Environment.malloc is used to set receive buffer in %s.",
				    (const char*)pFunction->GetName());
		}
		else
		{
		    if (IsOptionSet(PROGRAM_FORCE_ENV_MALLOC))
				CCompiler::Warning("Using CORBA_alloc because function %s has no environment.",
				    (const char*)pFunction->GetName());
			if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
				CCompiler::Warning("CORBA_alloc is used to set receive buffer in %s.",
				    (const char*)pFunction->GetName());
			pFile->Print("CORBA_alloc");
		}
	}
	else
	{
		if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
			CCompiler::Warning("CORBA_alloc is used to set receive buffer in %s.", (const char*)pFunction->GetName());
		pFile->Print("CORBA_alloc");
	}
}

/** \brief writes the actual used free function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBEContext::WriteFree(CBEFile* pFile, CBEFunction* pFunction)
{
    bool bUseFree = !IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC) &&
	    (pFunction && (!pFunction->IsComponentSide() ||
		               (pFunction->IsComponentSide() && IsOptionSet(PROGRAM_SERVER_PARAMETER))));
    bUseFree |= IsOptionSet(PROGRAM_FORCE_ENV_MALLOC);
	if (bUseFree)
	{
		CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
		CBEDeclarator *pDecl = 0;
		if (pEnv)
		{
			VectorElement* pIter = pEnv->GetFirstDeclarator();
			pDecl = pEnv->GetNextDeclarator(pIter);
		}
		if (pDecl)
		{
			pFile->Print("(%s", (const char*)pDecl->GetName());
			if (pDecl->GetStars())
				pFile->Print("->free)");
			else
				pFile->Print(".free)");
			if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
				CCompiler::Warning("CORBA_Environment.free is used to set receive buffer in %s.",
				    (const char*)pFunction->GetName());
		}
		else
		{
		    if (IsOptionSet(PROGRAM_FORCE_ENV_MALLOC))
				CCompiler::Warning("Using CORBA_free because function %s has no environment.",
				    (const char*)pFunction->GetName());
			if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
				CCompiler::Warning("CORBA_free is used to set receive buffer in %s.",
				    (const char*)pFunction->GetName());
			pFile->Print("CORBA_free");
		}
	}
	else
	{
		if (IsWarningSet(PROGRAM_WARNING_PREALLOC))
			CCompiler::Warning("CORBA_free is used to set receive buffer in %s.", (const char*)pFunction->GetName());
		pFile->Print("CORBA_free");
	}
}
