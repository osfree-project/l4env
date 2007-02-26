/**
 * \file dice/src/be/BEContext.cpp
 * \brief contains the implementation of the class CBEContext
 *
 * \date 01/10/2002
 * \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
    IMPLEMENT_DYNAMIC_BASE(CBEContext, CBEObject);
}

/** CBEContext destructor */
CBEContext::~CBEContext()
{
    if (m_pSizes)
        delete m_pSizes;
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
