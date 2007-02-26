/**
 *	\file	dice/src/fe/FEAttribute.cpp
 *	\brief	contains the implementation of the class CFEAttribute
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "fe/FEAttribute.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEAttribute) 

CFEAttribute::CFEAttribute()
{
    IMPLEMENT_DYNAMIC_BASE(CFEAttribute, CFEBase);

    m_nType = ATTR_NONE;
}

CFEAttribute::CFEAttribute(ATTR_TYPE nType)
:m_nType(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEAttribute, CFEBase);
}

CFEAttribute::CFEAttribute(CFEAttribute & src):CFEBase(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEAttribute, CFEBase);

    m_nType = src.m_nType;
}

/** cleans up the attribute */
CFEAttribute::~CFEAttribute()
{
    // nothing to clean up
}

/** returns the attribute's type
 *	\return the attribute's type
 */
ATTR_TYPE CFEAttribute::GetAttrType()
{
    return m_nType;
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEAttribute::Clone()
{
    return new CFEAttribute(*this);
}

/** serializes this object to/from a file
 *	\param pFile the file to serialize from/to
 */
void CFEAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
    pFile->PrintIndent("<attribute>");
    switch (m_nType)
    {
    case ATTR_NONE:
    case ATTR_LAST_ATTR:
    pFile->Print("none");
    break;
    case ATTR_UUID:
    pFile->Print("uuid");
    break;
    case ATTR_VERSION:
    pFile->Print("version");
    break;
    case ATTR_ENDPOINT:
    pFile->Print("endpoint");
    break;
    case ATTR_EXCEPTIONS:
    pFile->Print("exceptions");
    break;
    case ATTR_LOCAL:
    pFile->Print("local");
    break;
    case ATTR_POINTER_DEFAULT:
    pFile->Print("ptr_default");
    break;
    case ATTR_OBJECT:
    pFile->Print("object");
    break;
    case ATTR_UUID_REP:
    pFile->Print("uuid_rep");
    break;
    case ATTR_CONTROL:
    pFile->Print("control");
    break;
    case ATTR_HELPCONTEXT:
    pFile->Print("helpcontext");
    break;
    case ATTR_HELPFILE:
    pFile->Print("helpfile");
    break;
    case ATTR_HELPSTRING:
    pFile->Print("helpstring");
    break;
    case ATTR_HIDDEN:
    pFile->Print("hidden");
    break;
    case ATTR_LCID:
    pFile->Print("lcid");
    break;
    case ATTR_RESTRICTED:
    pFile->Print("restricted");
    break;
    case ATTR_SWITCH_IS:
    pFile->Print("switch_is");
    break;
    case ATTR_IDEMPOTENT:
    pFile->Print("idempotent");
    break;
    case ATTR_BROADCAST:
    pFile->Print("broadcast");
    break;
    case ATTR_MAYBE:
    pFile->Print("maybe");
    break;
    case ATTR_REFLECT_DELETIONS:
    pFile->Print("reflect_deletions");
    break;
    case ATTR_TRANSMIT_AS:
    pFile->Print("transmit_as");
    break;
    case ATTR_HANDLE:
    pFile->Print("handle");
    break;
    case ATTR_FIRST_IS:
    pFile->Print("first_is");
    break;
    case ATTR_LAST_IS:
    pFile->Print("last_is");
    break;
    case ATTR_LENGTH_IS:
    pFile->Print("length_is");
    break;
    case ATTR_MIN_IS:
    pFile->Print("min_is");
    break;
    case ATTR_MAX_IS:
    pFile->Print("max_is");
    break;
    case ATTR_SIZE_IS:
    pFile->Print("size_is");
    break;
    case ATTR_IGNORE:
    pFile->Print("ignore");
    break;
    case ATTR_IN:
    pFile->Print("in");
    break;
    case ATTR_OUT:
    pFile->Print("out");
    break;
    case ATTR_REF:
    pFile->Print("ref");
    break;
    case ATTR_UNIQUE:
    pFile->Print("unique");
    break;
    case ATTR_PTR:
    pFile->Print("ptr");
    break;
    case ATTR_IID_IS:
    pFile->Print("iid_is");
    break;
    case ATTR_STRING:
    pFile->Print("string");
    break;
    case ATTR_CONTEXT_HANDLE:
    pFile->Print("context_handle");
    break;
    case ATTR_SWITCH_TYPE:
    pFile->Print("switch_type");
    break;
    case ATTR_ABSTRACT:
    pFile->Print("abstract");
    break;
    case ATTR_DEFAULT_FUNCTION:
    pFile->Print("default_function");
    break;
    case ATTR_ERROR_FUNCTION:
    pFile->Print("error_function");
    break;
    case ATTR_SERVER_PARAMETER:
    pFile->Print("server_parameter");
    break;
    case ATTR_INIT_RCVSTRING:
    pFile->Print("init_rcvstring");
    break;
	case ATTR_PREALLOC:
	pFile->Print("init_with_in");
	break;
	case ATTR_ALLOW_REPLY_ONLY:
	pFile->Print("allow_reply_only");
	break;
    }
    pFile->Print("</attribute>\n");
    }
}
