/**
 *  \file    dice/src/Dependency.h
 *  \brief   contains the declaration of the class CDependency
 *
 *  \date    08/15/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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
#ifndef __DICE_DEPENDENCY_H__
#define __DICE_DEPENDENCY_H__

#include <cstdio>
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <fstream>
using std::ostream;

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;
class CBERoot;

/** \class CDependency
 *  \ingroup frontend
 *  \brief prints the dependencies for IDL file
 */
class CDependency
{
public:
    /** constructs the dependency object
     *  \param sFile the file name to write dependencies to
     *  \param pFERoot the root of the front-end
     *  \param pBERoot the root of the back-end
     */
    CDependency(string sFile, CFEFile *pFERoot, CBERoot *pBERoot);
    /** destroys the dependency object */
    ~CDependency()
    { }

public:
    void PrintDependencies();

protected:
    void PrintDependencyTree(CFEFile * pFEFile);
    void PrintGeneratedFiles(CFEFile * pFEFile);
    void PrintGeneratedFiles4File(CFEFile * pFEFile);
    void PrintGeneratedFiles4Library(CFEFile * pFEFile);
    void PrintGeneratedFiles4Library(CFELibrary * pFELibrary);
    void PrintGeneratedFiles4Library(CFEInterface * pFEInterface);
    void PrintGeneratedFiles4Interface(CFEFile * pFEFile);
    void PrintGeneratedFiles4Interface(CFELibrary * pFEpFELibrary);
    void PrintGeneratedFiles4Interface(CFEInterface * pFEInterface);
    void PrintGeneratedFiles4Operation(CFEFile * pFEFile);
    void PrintGeneratedFiles4Operation(CFELibrary * pFELibrary);
    void PrintGeneratedFiles4Operation(CFEInterface * pFEInterface);
    void PrintGeneratedFiles4Operation(CFEOperation * pFEOperation);
    void PrintDependentFile(string sFileName);

    // Attributes
protected:
    /** \var ostream* m_output
     *  \brief the output file
     */
    ostream* m_output;
    /** \var string m_sDependsFile
     *  \brief the file to write the dependencies to
     */
    string m_sDependsFile;
    /** \var CBERoot * m_pRootBE
     *  \brief reference to the back-end root
     */
    CBERoot *m_pRootBE;
    /** \var CFEFile *m_pRootFE
     *  \brief reference to the front-end root
     */
    CFEFile *m_pRootFE;
    /** \var int m_nCurCol
     *  \brief is the current column for dependency output
     */
    int m_nCurCol;
    /** \var vector m_vPhonyDependencies
     *  \brief contains file names with phony dependencies
     */
    vector<string> m_vPhonyDependencies;
};

#endif /* __DICE_DEPENDENCY_H__ */
