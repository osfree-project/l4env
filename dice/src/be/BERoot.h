/**
 *    \file    dice/src/be/BERoot.h
 *    \brief   contains the declaration of the class CBERoot
 *
 *    \date    01/10/2002
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
#ifndef __DICE_BEROOT_H__
#define __DICE_BEROOT_H__

#include "be/BEObject.h"
#include <vector>
using namespace std;

class CBEContext;

class CBEClient;
class CBEComponent;
class CBETestsuite;
class CBEClass;
class CBETypedef;
class CBEType;
class CBEFunction;
class CBENameSpace;
class CBEConstant;
class CBEImplementationFile;
class CBEHeaderFile;

class CFETypedDeclarator;
class CFEConstDeclarator;
class CFEConstructedType;
class CFEInterface;
class CFELibrary;
class CFEFile;

/**    \class CBERoot
 *    \ingroup backend
 *    \brief the root of the back-end structure
 */
class CBERoot : public CBEObject
{
// Constructor
public:
    /**    \brief constructor
     */
    CBERoot();
    virtual ~CBERoot();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBERoot(CBERoot &src);

public: // Public methods
    virtual CBEFunction* FindFunction(string sFunctionName);
    virtual void Write(CBEContext *pContext);
    virtual bool CreateBE(CFEFile *pFEFile, CBEContext *pContext);

    virtual CBETypedef* FindTypedef(string sTypeName);
    virtual CBETypedef* GetNextTypedef(vector<CBETypedef*>::iterator &iter);
    virtual vector<CBETypedef*>::iterator GetFirstTypedef();
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual void AddTypedef(CBETypedef *pTypedef);

    virtual CBEConstant* FindConstant(string sConstantName);
    virtual CBEConstant* GetNextConstant(vector<CBEConstant*>::iterator &iter);
    virtual vector<CBEConstant*>::iterator GetFirstConstant();
    virtual void RemoveConstant(CBEConstant *pConstant);
    virtual void AddConstant(CBEConstant *pConstant);

    virtual CBENameSpace* FindNameSpace(string sNameSpaceName);
    virtual CBENameSpace* GetNextNameSpace(vector<CBENameSpace*>::iterator &iter);
    virtual vector<CBENameSpace*>::iterator GetFirstNameSpace();
    virtual void RemoveNameSpace(CBENameSpace *pNameSpace);
    virtual void AddNameSpace(CBENameSpace*pNameSpace);

    virtual CBEClass* FindClass(string sClassName);
    virtual CBEClass* GetNextClass(vector<CBEClass*>::iterator &iter);
    virtual vector<CBEClass*>::iterator GetFirstClass();
    virtual void RemoveClass(CBEClass *pClass);
    virtual void AddClass(CBEClass *pClass);

    virtual bool AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual bool AddOpcodesToFile(CBEHeaderFile *pHeader, CFEFile *pFEFile, CBEContext *pContext);

    virtual CBEFunction* FindGlobalFunction(string sFuncName);
    virtual CBEFunction* GetNextGlobalFunction(vector<CBEFunction*>::iterator &iter);
    virtual vector<CBEFunction*>::iterator GetFirstGlobalFunction();
    virtual void RemoveGlobalFunction(CBEFunction *pFunction);
    virtual void AddGlobalFunction(CBEFunction *pFunction);
    virtual void PrintTargetFiles(FILE *output, int &nCurCol, int nMaxCol);

    virtual CBEType* FindTaggedType(int nType, string sTag);
    virtual CBEType* GetNextTaggedType(vector<CBEType*>::iterator &iter);
    virtual vector<CBEType*>::iterator GetFirstTaggedType();
    virtual void AddTaggedType(CBEType *pType);
    virtual void RemoveTaggedType(CBEType *pType);

protected: // Protected methods
    virtual bool CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext);

protected:
    /**    \var CBEClient *m_pClient
     *    \brief reference to client part
     *
     * This variable is a reference, because we use the ClassFactory, which only returns references.
     */
    CBEClient *m_pClient;
    /**    \var CBEComponent *m_pComponent
     *    \brief reference to component part
     */
    CBEComponent *m_pComponent;
    /**    \var CBETestsuite *m_pTestsuite
     *    \brief reference to testsuite part
     *
     * This variable is a reference, because we use the ClassFactory, which only returns references. And
     * we may set this reference depending on the options of the compiler.
     */
    CBETestsuite *m_pTestsuite;
    /** \var vector<CBEConstant*> m_vConstants
     *  \brief contains the constants of the back-end
     */
    vector<CBEConstant*> m_vConstants;
    /** \var vector<CBETypedef*> m_vTypedefs
     *  \brief contains the type definitions of the back-end
     */
    vector<CBETypedef*> m_vTypedefs;
    /** \var vector<CBEType*> m_vTypeDeclarations
     *  \brief contains the type declarations, which are not typedefs (usually tagged)
     */
    vector<CBEType*> m_vTypeDeclarations;
    /** \var vector<CBEClass*> m_vClasses
     *  \brief contains the classes of the back-end
     */
    vector<CBEClass*> m_vClasses;
    /** \var vector<CBENameSpace*> m_vNamespaces
     *  \brief contains the namespaces of the back-end
     */
    vector<CBENameSpace*> m_vNamespaces;
    /** \var vector<CBEFunction*> m_vGlobalFunctions
     *  \brief contains global functions (outside of classes and name-spaces)
     */
    vector<CBEFunction*> m_vGlobalFunctions;
};

#endif // !__DICE_BEROOT_H__
