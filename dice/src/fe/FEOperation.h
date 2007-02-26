/**
 *    \file    dice/src/fe/FEOperation.h
 *    \brief   contains the declaration of the class CFEOperation
 *
 *    \date    01/31/2001
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
#ifndef __DICE_FE_FEOPERATION_H__
#define __DICE_FE_FEOPERATION_H__

#include "fe/FEInterfaceComponent.h"
#include "Attribute-Type.h" // needed for ATTR_TYPE
#include <string>
#include <vector>
using namespace std;

class CFEIdentifier;
class CFETypeSpec;
class CFETypedDeclarator;
class CFEAttribute;

/**    \class CFEOperation
 *    \ingroup frontend
 *    \brief the description of a function in an interface
 *
 * This class is used to represent the functions of an interface. Do not mix with
 * CFEFunction.
 */
class CFEOperation : public CFEInterfaceComponent
{

// standard constructor/destructor
public:
    /** \brief the operation constructor
     *  \param pReturnType the return type
     *  \param sName the name of the operation
     *  \param pParameters the parameters
     *  \param pAttributes the attributes
     *  \param pRaisesDeclarators the names of the exceptions, which can be raised
     */
    CFEOperation(CFETypeSpec *pReturnType,
               string sName,
               vector<CFETypedDeclarator*> *pParameters,
               vector<CFEAttribute*> *pAttributes = 0,
               vector<CFEIdentifier*> *pRaisesDeclarators = 0);
    virtual ~CFEOperation();

protected:
    /** \brief copy constructor
     *    \param src the source to copy from
     */
    CFEOperation(CFEOperation &src);

// operations
public:
    virtual CFEIdentifier* GetNextRaisesDeclarator(vector<CFEIdentifier*>::iterator &iter);
    virtual vector<CFEIdentifier*>::iterator GetFirstRaisesDeclarator();
    virtual void Serialize(CFile *pFile);
    virtual void Dump();
    virtual CObject* Clone();
    virtual void RemoveParameter(CFETypedDeclarator *pParam);
    virtual void AddAttribute(CFEAttribute *pNewAttr);
    virtual void RemoveAttribute(ATTR_TYPE eAttrType);
    virtual void AddParameter(CFETypedDeclarator* pParam, bool bStart = false);
    virtual bool CheckConsistency();
    virtual CFEAttribute* FindAttribute(ATTR_TYPE eAttrType);

    virtual CFETypedDeclarator* FindParameter(string sName);
    virtual CFEAttribute* GetNextAttribute(vector<CFEAttribute*>::iterator &iter);
    virtual vector<CFEAttribute*>::iterator GetFirstAttribute();
    virtual CFETypedDeclarator* GetNextParameter(vector<CFETypedDeclarator*>::iterator &iter);
    virtual vector<CFETypedDeclarator*>::iterator GetFirstParameter();
    virtual string GetName();
    virtual CFETypeSpec* GetReturnType();

protected:
    virtual bool CheckAttributeParameters(CFETypedDeclarator *pParameter, ATTR_TYPE nAttribute, const char* sAttribute);

// attributes
protected:
    /** \var CFETypeSpec *m_pReturnType
     *  \brief return type of the function
     */
    CFETypeSpec *m_pReturnType;
    /** \var vector<CFEAttribute*> m_vOpAttributes
     *  \brief the attributes of the function
     */
    vector<CFEAttribute*> m_vOpAttributes;
    /** \var string m_sOpName
     *  \brief the name of the function
     */
    string m_sOpName;
    /** \var vector<CFETypedDeclarator*> m_vOpParameters
     *  \brief the parameters of the function
     */
    vector<CFETypedDeclarator*> m_vOpParameters;
    /** \var vector<CFEIdentifier*> m_vRaisesDeclarators
     *  \brief the exception, which can be raised by the function
     */
    vector<CFEIdentifier*> m_vRaisesDeclarators;
};

#endif /* __DICE_FE_FEOPERATION_H__ */
