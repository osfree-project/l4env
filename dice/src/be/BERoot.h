/**
 *  \file    dice/src/be/BERoot.h
 *  \brief   contains the declaration of the class CBERoot
 *
 *  \date    01/10/2002
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
#ifndef __DICE_BEROOT_H__
#define __DICE_BEROOT_H__

#include "BEObject.h"
#include "BEContext.h"
#include "TypeSpec-Type.h"
#include "template.h"
#include <vector>
#include <ostream>
using std::ostream;

class CBEClient;
class CBEComponent;
class CBEClass;
class CBETypedef;
class CBEType;
class CBEEnumType;
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

/** \class CBERoot
 *  \ingroup backend
 *  \brief the root of the back-end structure
 */
class CBERoot : public CBEObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBERoot();
    ~CBERoot();

public: // Public methods
    void Write();
    void CreateBE(CFEFile *pFEFile);

    CBETypedef* FindTypedef(std::string sTypeName, CBETypedef *pPrev = 0);
    CBEConstant* FindConstant(std::string sConstantName);
    CBENameSpace* FindNameSpace(std::string sNameSpaceName);
    CBEClass* FindClass(std::string sClassName, CBEClass *pPrev = 0);
    CBEType* FindTaggedType(unsigned int nType, std::string sTag);
    CBEFunction* FindFunction(std::string sFunctionName, FUNCTION_TYPE nFunctionType);
    CBEEnumType* FindEnum(std::string sEnumerator);

    void AddToImpl(CBEImplementationFile* pImpl);
    void AddToHeader(CBEHeaderFile* pHeader);
    void AddOpcodesToFile(CBEHeaderFile* pHeader, CFEFile *pFEFile);

    void PrintTargetFiles(ostream& output, int &nCurCol, int nMaxCol);

protected: // Protected methods
    void CreateBackEnd(CFEConstDeclarator *pFEConstant);
    void CreateBackEnd(CFEInterface *pFEInterface);
    void CreateBackEnd(CFELibrary *pFELibrary);
    void CreateBackEnd(CFETypedDeclarator *pFETypedef);
    void CreateBackEnd(CFEFile *pFEFile);
    void CreateBackEnd(CFEConstructedType *pFEType);

protected:
    /** \var CBEClient *m_pClient
     *  \brief reference to client part
     *
     * This variable is a reference, because we use the ClassFactory, which
     * only returns references.
     */
    CBEClient *m_pClient;
    /** \var CBEComponent *m_pComponent
     *  \brief reference to component part
     */
    CBEComponent *m_pComponent;

public:
    /** \var CSearchableCollection<CBEConstant, std::string> m_Constants
     *  \brief contains the constants of the back-end
     */
    CSearchableCollection<CBEConstant, std::string> m_Constants;
    /** \var CCollection<CBENameSpace> m_Namespaces
     *  \brief contains the namespaces of the back-end
     */
    CCollection<CBENameSpace> m_Namespaces;
    /** \var CSearchableCollection<CBEClass, std::string> m_Classes
     *  \brief contains the classes of the back-end
     */
    CSearchableCollection<CBEClass, std::string> m_Classes;
    /** \var CSearchableCollection<CBETypedef, std::string> m_Typedefs
     *  \brief contains the type definitions of the back-end
     */
    CSearchableCollection<CBETypedef, std::string> m_Typedefs;
    /** \var CCollection<CBEType> m_TypeDeclarations
     *  \brief contains the type declarations, which are not typedefs (usually
     *         tagged)
     */
    CCollection<CBEType> m_TypeDeclarations;
    /** \var CSearchableCollection<CBEFunction, std::string> m_GlobalFunctions
     *  \brief contains global functions (outside of classes and name-spaces)
     */
    CSearchableCollection<CBEFunction, std::string> m_GlobalFunctions;
};

#endif // !__DICE_BEROOT_H__
