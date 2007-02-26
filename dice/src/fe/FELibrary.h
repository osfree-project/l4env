/**
 *    \file    dice/src/fe/FELibrary.h
 *    \brief   contains the declaration of the class CFELibrary
 *
 *    \date    02/22/2001
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
#ifndef __DICE_FE_FELIBRARY_H__
#define __DICE_FE_FELIBRARY_H__

#include "fe/FEFileComponent.h"
#include <string>
#include <vector>
using namespace std;

class CFEIdentifier;
class CFETypedDeclarator;
class CFEConstDeclarator;
class CFETypedDeclarator;
class CFEinterface;
class CFEAttribute;

/**    \class CFELibrary
 *    \ingroup frontend
 *    \brief describes the front-end library
 */
class CFELibrary : public CFEFileComponent
{

// standard constructor/destructor
public:
    /** constructs a library object
     *    \param sName the name of the library
     *    \param pAttributes the attributes of the library
     *    \param pElements the elements (components) of the library*/
    CFELibrary(string sName, vector<CFEAttribute*> *pAttributes, vector<CFEFileComponent*> *pElements);
    virtual ~CFELibrary();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CFELibrary(CFELibrary &src);

// Operations
public:
    virtual CFEInterface* FindInterface(string sName, CFELibrary *pStart = NULL);
    virtual CFELibrary* FindLibrary(string sName);
    virtual void AddSameLibrary(CFELibrary *pLibrary);
    virtual void Serialize(CFile *pFile);
    virtual void Dump();
    virtual CFEConstDeclarator* FindConstant(string sName);
    virtual CFELibrary* GetNextLibrary(vector<CFELibrary*>::iterator &iter);
    virtual vector<CFELibrary*>::iterator GetFirstLibrary();
    virtual bool CheckConsistency();
    virtual CFETypedDeclarator* GetNextTypedef(vector<CFETypedDeclarator*>::iterator &iter);
    virtual vector<CFETypedDeclarator*>::iterator GetFirstTypedef();
    virtual CFEConstDeclarator* GetNextConstant(vector<CFEConstDeclarator*>::iterator &iter);
    virtual vector<CFEConstDeclarator*>::iterator GetFirstConstant();
    virtual CObject* Clone();
    virtual CFETypedDeclarator* FindUserDefinedType(string sName);
    virtual string GetName();
    virtual CFEAttribute* GetNextAttribute(vector<CFEAttribute*>::iterator &iter);
    virtual vector<CFEAttribute*>::iterator GetFirstAttribute();
    virtual CFEInterface* GetNextInterface(vector<CFEInterface*>::iterator &iter);
    virtual vector<CFEInterface*>::iterator GetFirstInterface();

    virtual void AddTypedef(CFETypedDeclarator *pFETypedef);
    virtual void AddInterface(CFEInterface *pFEInterface);
    virtual void AddConstant(CFEConstDeclarator *pFEConstant);
    virtual void AddLibrary(CFELibrary *pFELibrary);

    virtual void AddTaggedDecl(CFEConstructedType *pFETaggedDecl);
    virtual CFEConstructedType* GetNextTaggedDecl(vector<CFEConstructedType*>::iterator &iter);
    virtual vector<CFEConstructedType*>::iterator GetFirstTaggedDecl();
    virtual CFEConstructedType* FindTaggedDecl(string sName);

// Attributes
protected:
    /**    \var vector<CFEAttribute*> m_vAttributes
     *    \brief contains the library's attributes
     */
    vector<CFEAttribute*> m_vAttributes;
    /**    \var string m_sLibName
     *    \brief contains the library's name
     */
    string m_sLibName;
    /**    \var CFELibrary* m_pSameLibraryNext
     *    \brief the same library in other files (this is like a next pointer)
     */
    CFELibrary* m_pSameLibraryNext;
    /**    \var CFELibrary* m_pSameLibraryPrev
     *    \brief the same library in other files (this is like a prev pointer)
     */
    CFELibrary* m_pSameLibraryPrev;
    /** \var vector<CFEConstDeclarator*> m_vConstants
     *  \brief constains the constants defined in the library
     */
    vector<CFEConstDeclarator*> m_vConstants;
    /** \var vector<CFETypedDeclarator*> m_vTypedefs
     *  \brief contains the typedefs of this library
     */
    vector<CFETypedDeclarator*> m_vTypedefs;
    /** \var vector<CFEInterface*> m_vInterfaces
     *  \brief contains the interfaces of this library
     */
    vector<CFEInterface*> m_vInterfaces;
    /** \var vector<CFELibrary*> m_vLibraries
     *  \brief contains the nested libraries
     */
    vector<CFELibrary*> m_vLibraries;
    /** \var vector<CFEConstructedType*> m_vTaggedDeclarators
     *  \brief contains the tagged constructed types of this library
     */
    vector<CFEConstructedType*> m_vTaggedDeclarators;
};

#endif // __DICE_FE_FELIBRARY_H__
