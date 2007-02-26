/**
 *    \file    dice/src/be/l4/L4BEMsgBufferType.h
 *  \brief   contains the declaration of the class CL4BEMsgBufferType
 *
 *    \date    02/10/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2005
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
#ifndef __DICE_L4BEMSGBUFFER_H__
#define __DICE_L4BEMSGBUFFER_H__

#include "be/BEMsgBufferType.h"

/** \class CL4BEMsgBufferType
 *  \ingroup backend
 *  \brief the back-end struct type
 */
class CL4BEMsgBufferType : public CBEMsgBufferType
{

// Constructor
public:
    /** \brief constructor */
    CL4BEMsgBufferType();
    /** \brief destructor of this instance */
    virtual ~CL4BEMsgBufferType()
    { }

protected: // Protected methods
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CL4BEMsgBufferType(CL4BEMsgBufferType &src);

public: // Public methods
    /** \brief creates a copy of this object
     *  \return a reference to the new instance
     */
    virtual CObject * Clone()
    { return new CL4BEMsgBufferType(*this); }

protected:
    virtual void AddElements(CFEOperation *pFEOperation, int nDirection);
    virtual void AddElement(CFETypedDeclarator *pFEParameter, int nDirection);
    virtual void AddRefstringElement(CFETypedDeclarator *pFEParameter, 
	int nDirection);
    virtual void AddFlexpageElement(CFETypedDeclarator *pFEParameter,
	int nDirection);
    virtual void AddZeroFlexpage(CFEOperation *pFEOperation, int nDirection);
};

#endif // !__DICE_L4BEMSGBUFFERTYPE_H__
