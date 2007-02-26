/**
 *  \file    dice/src/be/BETarget.h
 *  \brief   contains the declaration of the class CBETarget
 *
 *  \date    01/11/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BETARGET_H__
#define __DICE_BETARGET_H__

#include "BEObject.h"
#include "BEContext.h" // for FILE_TYPE
#include "template.h"
#include <vector>
#include <ostream>
using std::ostream;

class CFEFile;
class CFEOperation;
class CFEInterface;
class CFELibrary;
class CFEFile;
class CFETypedDeclarator;
class CFEConstDeclarator;

class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEContext;
class CBETypedef;
class CBEFunction;

/** \class CBETarget
 *  \ingroup backend
 *  \brief the base class for the target components (client, component,
 *          testsuite, root)
 *
 * This class is introduced as base class to client, component, testsuite and
 * root, because these target classes all have the posession of files in
 * common. The client and component obviously posess several header and
 * implementation files. The root may also contain some header files, such as
 * opcode files or similar. The testsuite posseses the implementation files
 * containing the tesuite application.
 */
class CBETarget : public CBEObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBETarget();
    virtual ~CBETarget();

protected:
    /** \brief copy constructor */
    CBETarget(CBETarget &src);

public:
    virtual void CreateBackEnd(CFEFile *pFEFile);
    
    virtual CBEFunction* FindFunction(string sFunctionName,
	FUNCTION_TYPE nFunctionType);
    virtual CBETypedef* FindTypedef(string sTypeName);
    
    /** \brief generates the output files and code */
    virtual void Write() = 0;
    virtual void PrintTargetFiles(ostream& output, int &nCurCol, int nMaxCol);
    virtual bool HasFunctionWithUserType(string sTypeName);

protected:
    virtual void WriteImplementationFiles();
    virtual void WriteHeaderFiles();
    virtual bool DoAddIncludedFiles();
    // constants
    virtual bool AddConstantToFile(CBEFile *pFile, 
	CFEConstDeclarator *pFEConstant);
    virtual bool AddConstantToFile(CBEFile *pFile, CFEInterface *pFEInterface);
    virtual bool AddConstantToFile(CBEFile *pFile, CFELibrary *pFELibrary);
    virtual bool AddConstantToFile(CBEFile *pFile, CFEFile *pFEFile);
    // type definitions
    virtual bool AddTypedefToFile(CBEFile *pFile, 
	CFETypedDeclarator *pFETypedDeclarator);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFEInterface *pFEInterface);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFELibrary *pFELibrary);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFEFile *pFEFile);
    // header file search
    virtual CBEHeaderFile* FindHeaderFile(CFEOperation *pFEOperation,
	FILE_TYPE nFileType);
    virtual CBEHeaderFile* FindHeaderFile(CFEInterface *pFEInterface,
	FILE_TYPE nFileType);
    virtual CBEHeaderFile* FindHeaderFile(CFELibrary *pFELibrary,
	FILE_TYPE nFileType);
    virtual CBEHeaderFile* FindHeaderFile(CFEFile *pFEFile, FILE_TYPE nFileType); 

    /** \brief create target for header file 
     *  \param pFEFile the front-end file to use as reference
     */
    virtual void CreateBackEndHeader(CFEFile *pFEFile) = 0;
    /** \brief create target for implementation file 
     *  \param pFEFile the front-end file to use as reference
     */
    virtual void CreateBackEndImplementation(CFEFile *pFEFile) = 0;
    
    virtual void PrintTargetFileName(ostream& output, string sFilename, 
	int &nCurCol, int nMaxCol);

public:
    /** \var CSearchableCollection<CBEHeaderFile, string> m_HeaderFiles
     *  \brief contains the header files of the respective target part
     *
     * Because the handling for header and implementation files is different
     * at some points, we keep them in different vectors.
     */
    CSearchableCollection<CBEHeaderFile, string> m_HeaderFiles;
    /** \var CCollection<CBEImplementationFile> m_ImplementationFiles
     *  \brief contains the implementation files for the respective target part
     */
    CCollection<CBEImplementationFile> m_ImplementationFiles;
};

#endif // !__DICE_BETARGET_H__
