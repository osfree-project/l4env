/**
 *    \file    dice/src/be/BETarget.h
 *    \brief   contains the declaration of the class CBETarget
 *
 *    \date    01/11/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/BEObject.h"
#include <vector>
using namespace std;

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

#define FILETYPE_HEADER                1    /**< helper define for header files */
#define FILETYPE_IMPLEMENTATION        2    /**< helper define for implementation files */

/**    \class CBETarget
 *    \ingroup backend
 *    \brief the base class for the target components (client, component, testsuite, root)
 *
 * This class is introduced as base class to client, component, testsuite and root, because these target classes
 * all have the posession of files in common. The client and component obviously posess several header and implementation
 * files. The root may also contain some header files, such as opcode files or similar. The testsuite posseses the implementation
 * files containing the tesuite application.
 */
class CBETarget : public CBEObject
{
// Constructor
public:
    /**    \brief constructor
     */
    CBETarget();
    virtual ~CBETarget();

protected:
    /**    \brief copy constructor */
    CBETarget(CBETarget &src);

public:
    virtual CBEFunction* FindFunction(string sFunctionName);
    virtual void RemoveFile(CBEImplementationFile *pImplementation);
    virtual void RemoveFile(CBEHeaderFile *pHeader);
    virtual CBETypedef* FindTypedef(string sTypeName);
    virtual CBEImplementationFile* GetNextImplementationFile(vector<CBEImplementationFile*>::iterator &iter);
    virtual vector<CBEImplementationFile*>::iterator GetFirstImplementationFile();
    virtual CBEHeaderFile* GetNextHeaderFile(vector<CBEHeaderFile*>::iterator &iter);
    virtual vector<CBEHeaderFile*>::iterator GetFirstHeaderFile();
    virtual void AddFile(CBEImplementationFile *pImplementationFile);
    virtual void AddFile(CBEHeaderFile *pHeaderFile);
    virtual void Write(CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
    virtual void PrintTargetFiles(FILE *output, int &nCurCol, int nMaxCol);
    virtual bool HasFunctionWithUserType(string sTypeName, CBEContext *pContext);

protected:
    virtual void WriteImplementationFiles(CBEContext *pContext);
    virtual void WriteHeaderFiles(CBEContext *pContext);
    virtual void SetFileType(CBEContext *pContext, int nHeaderOrImplementation);
    virtual bool DoAddIncludedFiles(CBEContext *pContext);
    // constants
    virtual bool AddConstantToFile(CBEFile *pFile, CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool AddConstantToFile(CBEFile *pFile, CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddConstantToFile(CBEFile *pFile, CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool AddConstantToFile(CBEFile *pFile, CFEFile *pFEFile, CBEContext *pContext);
    // type definitions
    virtual bool AddTypedefToFile(CBEFile *pFile, CFETypedDeclarator *pFETypedDeclarator, CBEContext *pContext);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool AddTypedefToFile(CBEFile *pFile, CFEFile *pFEFile, CBEContext *pContext);
    // header file search
    virtual CBEHeaderFile* FindHeaderFile(string sFileName, CBEContext *pContext);
    virtual CBEHeaderFile* FindHeaderFile(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual CBEHeaderFile* FindHeaderFile(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual CBEHeaderFile* FindHeaderFile(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual CBEHeaderFile* FindHeaderFile(CFEFile *pFEFile, CBEContext *pContext);

    virtual bool CreateBackEndHeader(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEndImplementation(CFEFile *pFEFile, CBEContext *pContext);
    virtual void PrintTargetFileName(FILE *output, string sFilename, int &nCurCol, int nMaxCol);

protected:
    /**    \var vector<CBEHeaderFile*> m_vHeaderFiles
     *    \brief contains the header files of the respective target part
     *
     * Because the handling for header and implementation files is different at some points, we keep
     * them in different vectors.
     */
    vector<CBEHeaderFile*> m_vHeaderFiles;
    /**    \var vector<CBEImplementationFile*> m_vImplementationFiles
     *    \brief contains the implementation files for the respective target part
     */
    vector<CBEImplementationFile*> m_vImplementationFiles;
};

#endif // !__DICE_BETARGET_H__
