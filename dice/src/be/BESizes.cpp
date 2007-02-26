/**
 *    \file    dice/src/be/BESizes.cpp
 *    \brief   contains the implementation of the class CBESizes
 *
 *    \date    Wed Oct 9 2002
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

#include "be/BESizes.h"
#include "TypeSpec-Type.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

CBESizes::CBESizes()
{
    m_nOpcodeSize = sizeof(long);
}

/** \brief destroys the object of class CBESizes
 */
CBESizes::~CBESizes()
{
}

/** \brief gets the size of a type
 *  \param nFEType the type to look up
 *  \param nFESize the size in the front-end
 *  \return the size of the type in bytes
 */
int CBESizes::GetSizeOfType(int nFEType, int nFESize)
{

    int nSize = 0;
    switch (nFEType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
        {
            switch (nFESize)
            {
            case 1:
                nSize = sizeof(unsigned char); // IDL: small == 1 byte
                break;
            case 2:
                nSize = sizeof(short int);
                break;
            case 4:
                nSize = sizeof(int);
                break;
            case 8:
#if SIZEOF_LONG_LONG > 0
                nSize = sizeof(long long);
#else
                nSize = sizeof(long);
#endif
                break;
            default:
                nSize = sizeof(int);
                break;
            }
        }
        break;
    case TYPE_VOID:
        nSize = 0;
        break;
    case TYPE_FLOAT:
        nSize = sizeof(float);
        break;
    case TYPE_DOUBLE:
        nSize = sizeof(double);
        break;
    case TYPE_LONG_DOUBLE:
        nSize = sizeof(long double);
        break;
    case TYPE_CHAR:
        nSize = sizeof(char);
        break;
    case TYPE_WCHAR:
        nSize = sizeof(char);
        break;
    case TYPE_BOOLEAN:
        nSize = sizeof(char);
        break;
    case TYPE_BYTE:
        nSize = sizeof(unsigned char);
        break;
    case TYPE_VOID_ASTERISK:
        nSize = sizeof(void *);
        break;
    case TYPE_CHAR_ASTERISK:
        nSize = sizeof(char *);
        break;
    case TYPE_ERROR_STATUS_T:
        nSize = 4;    // define error_status_t as unsigned int (4 bytes)
        break;
    case TYPE_FLEXPAGE:
        nSize = 8;    // a flexpage needs 2 dwords to be transmitted
        break;
    case TYPE_RCV_FLEXPAGE:
        nSize = 0;    // is not send, simple helper type
        break;
    case TYPE_STRING:
        nSize = sizeof(char);
        break;
    case TYPE_WSTRING:
        nSize = sizeof(char);
        break;
    case TYPE_MWORD:
        nSize = sizeof(unsigned long);
        break;
    case TYPE_ISO_LATIN_1:
    case TYPE_ISO_MULTILINGUAL:
    case TYPE_ISO_UCS:
        nSize = 1;    // I don't know how big this should be -> use only one byte
        break;
    default:
        break;
    }
    return nSize;
}

/** \brief retrieves the size of an environment type
 *  \param sName the name of the type
 *  \return the size of this type in bytes
 *
 * The code sometimes uses types, which are defined by the environment. The compiler
 * may use them as user defined types. If the environment type is used it cannot be found
 * in the back-end types and thus has to be queried for size explicetly.
 *
 * This implementation does not know any specific environment types.
 */
int CBESizes::GetSizeOfEnvType(string sName)
{
    return 0;
}

/** \brief returns a value for the maximum  size of a specific type
 *  \param nFEType the type to get the max size for
 *  \return the maximum size of an array of that type
 *
 * This function is used to determine a maximum size of an array of a specifc
 * type if the parameter has no maximum size attribute.
 */
int CBESizes::GetMaxSizeOfType(int nFEType)
{
    return 512;
}

/** \brief calculates the size of the opcode
 *  \return the size of the opcode's type in bytes
 */
int CBESizes::GetOpcodeSize()
{
    return m_nOpcodeSize;
}

/** \brief sets the opcode type size
 *  \param nSize the new size in bytes
 */
void CBESizes::SetOpcodeSize(int nSize)
{
    if (nSize < 0)
        return;
    m_nOpcodeSize = nSize;
}

/** \brief returns the size of the exception
 *  \return the size of the exception in bytes
 */
int CBESizes::GetExceptionSize()
{
    // currently only the first word of the exception is transmitted
    return 4;
}
