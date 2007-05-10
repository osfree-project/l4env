/**
 *    \file    dice/src/be/MsgStructType.cpp
 *    \brief   contains the implementation of the class CMsgStructType
 *
 *    \date    05/08/2007
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "MsgStructType.h"

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are equal
 */
bool operator== (const CMsgStructType& l, const CMsgStructType& r)
{
    return l.nType == r.nType;
}

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are equal
 */
bool operator== (const CMsgStructType::Type& l, const CMsgStructType& r)
{
    return l == r.nType;
}

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are equal
 */
bool operator== (const CMsgStructType& l, const CMsgStructType::Type& r)
{
    return l.nType == r;
}

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are not equal
 */
bool operator!= (const CMsgStructType& l, const CMsgStructType& r)
{
    return l.nType != r.nType;
}

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are not equal
 */
bool operator!= (const CMsgStructType::Type& l, const CMsgStructType& r)
{
    return l != r.nType;
}

/** \brief comparison operator
 *  \param l a message buffer struct type
 *  \param r another message buffer struct type
 *  \return true if both are not equal
 */
bool operator!= (const CMsgStructType& l, const CMsgStructType::Type& r)
{
    return l.nType != r;
}

