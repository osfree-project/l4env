/**
 *	\file	dice/src/fe/FESimpleType.cpp
 *	\brief	contains the implementation of the class CFESimpleType
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

#include "fe/FESimpleType.h"
#include "Compiler.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFESimpleType)
    
CFESimpleType::CFESimpleType(TYPESPEC_TYPE nType,
			     bool bUnSigned,
			     bool bUnsignedFirst,
			     int nSize,
			     bool bShowType)
:CFETypeSpec(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFESimpleType, CFETypeSpec);
    m_bUnSigned = bUnSigned;
    m_bUnsignedFirst = bUnsignedFirst;
    m_bShowType = bShowType;
    m_nSize = nSize;
}

CFESimpleType::CFESimpleType(CFESimpleType & src)
:CFETypeSpec(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFESimpleType, CFETypeSpec);

    m_bUnSigned = src.m_bUnSigned;
    m_bUnsignedFirst = src.m_bUnsignedFirst;
    m_bShowType = src.m_bShowType;
    m_nSize = src.m_nSize;
}

/** CFESimpleType destructor */
CFESimpleType::~CFESimpleType()
{
    // nothing to clean up
}

/** clones the object using the copy constructor */
CObject *CFESimpleType::Clone()
{
    return new CFESimpleType(*this);
}

/** checks if this type is unsigned
 *	\return true if unsigned
 */
bool CFESimpleType::IsUnsigned()
{
    return m_bUnSigned;
}

/** \brief checks consistency
 *  \return false if error occured, true otherwise
 *
 * A simple type is consistent if it is not TYPE_NONE and has a size >= 0.
 */
bool CFESimpleType::CheckConsistency()
{
    if (m_nType == TYPE_NONE)
      {
	  CCompiler::GccError(this, 0, "The type has no type?!");
	  return false;
      }
    if (m_nSize < 0)
      {
	  CCompiler::GccError(this, 0, "Type with negative size.");
	  return false;
      }
    return true;
}

/**	sets the signed/unsigned variable
 *	\param bUnsigned the new unsigned value
 */
void CFESimpleType::SetUnsigned(bool bUnsigned)
{
    m_bUnSigned = bUnsigned;
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFESimpleType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  pFile->PrintIndent("<type>");
	  switch (GetType())
	    {
	    case TYPE_INTEGER:
		pFile->Print("int");
		break;
		case TYPE_LONG:
		pFile->Print("long");
		break;
	    case TYPE_VOID:
		pFile->Print("void");
		break;
	    case TYPE_FLOAT:
		pFile->Print("float");
		break;
	    case TYPE_DOUBLE:
		pFile->Print("double");
		break;
        case TYPE_LONG_DOUBLE:
        pFile->Print("long double");
        break;
	    case TYPE_CHAR:
		pFile->Print("char");
		break;
	    case TYPE_WCHAR:
		pFile->Print("wchar");
		break;
	    case TYPE_BOOLEAN:
		pFile->Print("boolean");
		break;
	    case TYPE_BYTE:
		pFile->Print("byte");
		break;
	    case TYPE_VOID_ASTERISK:
		pFile->Print("void*");
		break;
	    case TYPE_CHAR_ASTERISK:
		pFile->Print("char*");
		break;
	    case TYPE_ERROR_STATUS_T:
		pFile->Print("error_status_t");
		break;
	    case TYPE_FLEXPAGE:
		pFile->Print("flexpage");
		break;
	    case TYPE_RCV_FLEXPAGE:
		pFile->Print("rcv_flexpage");
		break;
	    case TYPE_STRING:
		pFile->Print("string");
		break;
	    case TYPE_WSTRING:
		pFile->Print("wstring");
		break;
	    case TYPE_ISO_LATIN_1:
		pFile->Print("iso_latin_1");
		break;
	    case TYPE_ISO_MULTILINGUAL:
		pFile->Print("iso_multilingual");
		break;
	    case TYPE_ISO_UCS:
		pFile->Print("iso_ucs");
		break;
	    default:
		break;
	    }
	  pFile->Print("</type>\n");
      }
}

/** \brief resturns the size of the type
 *  \return the size of the type specified in the IDL file
 */
int CFESimpleType::GetSize()
{
    return m_nSize;
}
