/**
 *  \file    dice/src/be/BETypedDeclarator.h
 *  \brief   contains the declaration of the class CBETypedDeclarator
 *
 *  \date    01/18/2002
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
#ifndef __DICE_BETYPEDDECLARATOR_H__
#define __DICE_BETYPEDDECLARATOR_H__

#include "be/BEObject.h"
#include "be/BEAttribute.h"
#include "be/BEDeclarator.h"
#include "template.h"
#include "Attribute-Type.h"
#include <vector>
#include <map>
using std::multimap;

class CFETypedDeclarator;
class CFEDeclarator;
class CFEAttribute;
class CBEContext;
class CBEType;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CDeclaratorStackLocation;
class CBEConstant;

/** \class CBETypedDeclarator
 *  \ingroup backend
 *  \brief the back-end parameter
 */
class CBETypedDeclarator : public CBEObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBETypedDeclarator();
    virtual ~CBETypedDeclarator();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBETypedDeclarator(CBETypedDeclarator & src);

public:
    virtual void CreateBackEnd(CBEType * pType, string sName);
    virtual void CreateBackEnd(string sUserDefinedType, string sName, int nStars);
    virtual void CreateBackEnd(CFETypedDeclarator * pFEParameter);

    int GetSize();
    int GetSize(string sName);
    CBEType *GetType();
    int GetBitfieldSize();
    bool GetMaxSize(bool bGuessSize, int & nSize, string sName = string());

    void ReplaceType(CBEType * pNewType);

    virtual bool IsVariableSized();
    bool IsString();
    bool IsDirection(int nDirection);
    bool IsFixedSized();
    bool HasReference();

    bool Match(string sName);
    CBEDeclarator *GetCallDeclarator();
    void RemoveCallDeclarator();
    void AddDeclarator(CFEDeclarator * pFEDeclarator);
    void AddDeclarator(string sName, int nStars);

    CBEAttribute* FindIsAttribute(string sDeclName);
    void AddAttribute(CFEAttribute *pFEAttribute);

    /** \brief creates a new instance of this class 
     *  \return a reference to the copy
     */
    virtual CObject *Clone()
    { return new CBETypedDeclarator(*this); }

    /** \brief sets the default initialization string
     *  \param sInitString the new initializatio string (may be empty)
     */
    void SetDefaultInitString(string sInitString)
    { m_sDefaultInitString = sInitString; }
    /** \brief accesses the default initialization string
     *  \return the value of the default initialization string
     */
    string GetDefaultInitString(void)
    { return m_sDefaultInitString; }

    bool AddLanguageProperty(string sProperty, string sPropertyString);
    bool FindLanguageProperty(string sProperty, string& sPropertyString);

    void WriteDeclarators(CBEFile * pFile);

    // delegated to langauge dependent part
    virtual void WriteDefinition(CBEFile *pFile);
    virtual void WriteDeclaration(CBEFile * pFile);
    void WriteSetZero(CBEFile* pFile);
    void WriteGetSize(CBEFile * pFile, 
	vector<CDeclaratorStackLocation*> *pStack, CBEFunction *pUsingFunc);
    void WriteGetMaxSize(CBEFile * pFile, 
	vector<CDeclaratorStackLocation*> *pStack, CBEFunction *pUsingFunc);
    void WriteCleanup(CBEFile* pFile, bool bDeferred);
    void WriteType(CBEFile * pFile, bool bUseConst = true);
    void WriteIndirect(CBEFile * pFile);
    void WriteIndirectInitialization(CBEFile * pFile, bool bMemory);
    void WriteInitDeclaration(CBEFile* pFile, string sInitString);
    // language specific
    virtual void WriteForwardDeclaration(CBEFile *pFile);
    virtual void WriteForwardTypeDeclaration(CBEFile * pFile,
	bool bUseConst = true);

    CBEType* GetTransmitType();

protected:
    int GetSizeOfDeclarator(CBEDeclarator *pDeclarator);
    CBETypedDeclarator* GetSizeVariable(CBEAttribute *pIsAttribute,
	vector<CDeclaratorStackLocation*> *pStack, CBEFunction *pUsingFunc,
	bool& bFoundInStruct);
    CBEConstant* GetSizeConstant(CBEAttribute *pIsAttribute);
    void WarnNoMax(int nSize);
    bool GetSizeOrDimensionOfAttr(ATTR_TYPE nAttr, int& nSize, int& nDimension);

    virtual void WriteAttributes(CBEFile * pFile);
    virtual void WriteConstPrefix(CBEFile *pFile);
    virtual void WriteProperties(CBEFile *pFile);

protected:
    /** \var CBEType *m_pType
     *  \brief the type of the parameter
     */
    CBEType * m_pType;
    /** \var string m_sDefaultInitString
     *  \brief contains the initialization sequence if no other is given
     */
    string m_sDefaultInitString;
    /** \var map<string, string> m_mProperties
     *  \brief map with language specific properties
     */
    multimap<string, string> m_mProperties;

public:
    /** \var CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes
     *  \brief contains the type's attributes
     */
    CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CSearchableCollection<CBEDeclarator, string> m_Declarators
     *  \brief the names of the parameter
     */
    CSearchableCollection<CBEDeclarator, string> m_Declarators;
};

#endif                //*/ !__DICE_BETYPEDDECLARATOR_H__
