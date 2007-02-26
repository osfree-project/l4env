/**
 *  \file   dice/src/be/BEComponent.h
 *  \brief  contains the declaration of the class CBEComponent
 *
 *  \date   01/11/2002
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_BECOMPONENT_H__
#define __DICE_BECOMPONENT_H__

#include "be/BETarget.h"

class CFEBase;
class CFEInterface;
class CFELibrary;
class CFEFile;

/** \class CBEComponent
 *  \ingroup backend
 *  \brief the component - a collection of files
 */
class CBEComponent : public CBETarget
{
// Constructor
public:
    /** \brief constructor
     */
    CBEComponent();
    virtual ~CBEComponent();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEComponent(CBEComponent &src);

public:
    virtual void Write(void);

protected:
    virtual bool NeedServerLoop(CFEInterface *pFEInterface);
    virtual void CreateBackEndHeader(CFEFile * pFEFile);
    virtual void CreateBackEndImplementation(CFEFile * pFEFile);
};

#endif // !__DICE_BECOMPONENT_H__
