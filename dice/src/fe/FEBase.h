/**
 *    \file    dice/src/fe/FEBase.h
 *    \brief   contains the declaration of the class CFEBase
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
#ifndef __DICE_FE_FEBASE_H__
#define __DICE_FE_FEBASE_H__

#include "Object.h"
#include <string>
using namespace std;

class CFile;
class CFEFile;
class CFEInterface;
class CFEOperation;
class CFELibrary;
class CFEConstructedType;

/** \class CFEBase
 *    \ingroup frontend
 *    \brief The front-end base class
 *
 * This class is the base class for all classes of the front-end. It
 * implements several features, each front-end class might use.
 */
class CFEBase : public CObject
{


// standard constructor/destructor
public:
    /** constructs a front-end base object
     *    \param pParent the parent object of this one */
    CFEBase(CObject* pParent = 0);
    virtual ~CFEBase();

protected:
    /** \brief copy constructor
     *    \param src the source to copy from
     */
    CFEBase(CFEBase &src);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual void Dump();
    virtual string ToString();

    virtual bool CheckConsistency();
    virtual CObject* Clone();
    virtual CFEBase* GetRoot();

protected:
};

#endif /* __DICE_FE_FEBASE_H__ */
