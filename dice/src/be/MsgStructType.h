/**
 *  \file    dice/src/be/MsgBufferType.h
 *  \brief   contains the declaration of the class CMsgStructType
 *
 *  \date    05/08/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __DICE_BE_MSGSTRUCTTYPE_H__
#define __DICE_BE_MSGSTRUCTTYPE_H__

#include "BEFunction.h"

/** \class CMsgStructType
 *  \ingroup backend
 *  \brief helper class to determine struct in msg-buf type
 */
class CMsgStructType
{
public:
    /** \enum Type
     *  \brief contains the valid message buffer struct types
     */
    enum Type { Generic, In, Out, Exc };

    /** \brief constructor
     *  \param type a message buffer struct type initializer
     */
    CMsgStructType(Type type)
    {
	nType = type;
    }

    /** \brief constructor
     *  \param nDir a DIRECTION_TYPE initializer
     */
    CMsgStructType(DIRECTION_TYPE nDir)
    {
	switch (nDir) {
	case 0:
	case DIRECTION_INOUT:
	    nType = Generic;
	    break;
	case DIRECTION_IN:
	    nType = In;
	    break;
	case DIRECTION_OUT:
	    nType = Out;
	    break;
	default:
	    nType = Exc;
	}
    }

    /** \brief cast operator
     *  \return an int value
     */
    operator int() const
    {
	return nType;
    }

    /** \brief cast operator
     *  \return a DIRECTION_TYPE value
     */
    operator DIRECTION_TYPE() const
    {
	switch (nType) {
	case In:
	    return DIRECTION_IN;
	case Out:
	    return DIRECTION_OUT;
	default:
	    break;
	}
	return DIRECTION_NONE;
    }

    friend bool operator== (const CMsgStructType&, const CMsgStructType&);
    friend bool operator== (const CMsgStructType::Type&, const CMsgStructType&);
    friend bool operator== (const CMsgStructType&, const CMsgStructType::Type&);
    friend bool operator!= (const CMsgStructType&, const CMsgStructType&);
    friend bool operator!= (const CMsgStructType::Type&, const CMsgStructType&);
    friend bool operator!= (const CMsgStructType&, const CMsgStructType::Type&);
private:
    /** \var Type nType
     *  \brief the message buffer struct type
     */
    Type nType;
};
#endif
