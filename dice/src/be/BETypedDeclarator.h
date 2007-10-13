/**
 *  \file    dice/src/be/BETypedDeclarator.h
 *  \brief   contains the declaration of the class CBETypedDeclarator
 *
 *  \date    01/18/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BETYPEDDECLARATOR_H__
#define __DICE_BETYPEDDECLARATOR_H__

#include "BEObject.h"
#include "BEAttribute.h"
#include "BEDeclarator.h"
#include "BEFunction.h"
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
    CBETypedDeclarator(CBETypedDeclarator* src);

public:
    virtual void CreateBackEnd(CBEType * pType, std::string sName);
    virtual void CreateBackEnd(std::string sUserDefinedType, std::string sName, int nStars);
    virtual void CreateBackEnd(CFETypedDeclarator * pFEParameter);

    int GetSize();
    int GetSize(std::string sName);
    CBEType *GetType();
    int GetBitfieldSize();
    virtual bool GetMaxSize(int & nSize, std::string sName = std::string());

    void ReplaceType(CBEType * pNewType);

    virtual bool IsVariableSized();
    bool IsString();
    bool IsDirection(DIRECTION_TYPE nDirection);
    bool IsFixedSized();
    bool HasReference();

    bool Match(std::string sName);
    CBEDeclarator *GetCallDeclarator();
    void RemoveCallDeclarator();
    void AddDeclarator(CFEDeclarator * pFEDeclarator);
    void AddDeclarator(std::string sName, int nStars);

    CBEAttribute* FindIsAttribute(std::string sDeclName);
    void AddAttribute(CFEAttribute *pFEAttribute);

	virtual CObject* Clone();

    /** \brief sets the default initialization string
     *  \param sInitString the new initializatio string (may be empty)
     */
    void SetDefaultInitString(std::string sInitString)
    { m_sDefaultInitString = sInitString; }
    /** \brief accesses the default initialization string
     *  \return the value of the default initialization string
     */
    std::string GetDefaultInitString()
    { return m_sDefaultInitString; }

    bool AddLanguageProperty(std::string sProperty, std::string sPropertyString);
    bool FindLanguageProperty(std::string sProperty, std::string& sPropertyString);

    void WriteDeclarators(CBEFile& pFile);

    // delegated to langauge dependent part
    virtual void WriteDeclaration(CBEFile& pFile);
    void WriteSetZero(CBEFile& pFile);
    void WriteGetSize(CBEFile& pFile,
	CDeclStack* pStack, CBEFunction *pUsingFunc);
    void WriteGetMaxSize(CBEFile& pFile,
	CDeclStack* pStack, CBEFunction *pUsingFunc);
    void WriteCleanup(CBEFile& pFile, bool bDeferred);
    void WriteType(CBEFile& pFile, bool bUseConst = true);
    void WriteIndirect(CBEFile& pFile);
    void WriteIndirectInitialization(CBEFile& pFile, bool bMemory);
    void WriteInitDeclaration(CBEFile& pFile, std::string sInitString);

    CBEType* GetTransmitType();

protected:
    int GetSizeOfDeclarator(CBEDeclarator *pDeclarator);
    CBETypedDeclarator* GetSizeVariable(CBEAttribute *pIsAttribute,
	CDeclStack* pStack, CBEFunction *pUsingFunc,
	bool& bFoundInStruct);
    CBEConstant* GetSizeConstant(CBEAttribute *pIsAttribute);
    void WarnNoMax(int nSize);
    bool GetSizeOrDimensionOfAttr(ATTR_TYPE nAttr, int& nSize, int& nDimension);

    virtual void WriteAttributes(CBEFile& pFile);
    virtual void WriteConstPrefix(CBEFile& pFile);
    virtual void WriteProperties(CBEFile& pFile);
    virtual bool DoAllocateMemory(CBEFile& pFile);

private:
    bool UsePointer();

protected:
    /** \var CBEType *m_pType
     *  \brief the type of the parameter
     */
    CBEType * m_pType;
    /** \var std::string m_sDefaultInitString
     *  \brief contains the initialization sequence if no other is given
     */
    std::string m_sDefaultInitString;
    /** \var map<std::string, std::string> m_mProperties
     *  \brief map with language specific properties
     */
    multimap<std::string, std::string> m_mProperties;

public:
    /** \var CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes
     *  \brief contains the type's attributes
     */
    CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CSearchableCollection<CBEDeclarator, std::string> m_Declarators
     *  \brief the names of the parameter
     */
    CSearchableCollection<CBEDeclarator, std::string> m_Declarators;
};

#endif                //*/ !__DICE_BETYPEDDECLARATOR_H__
