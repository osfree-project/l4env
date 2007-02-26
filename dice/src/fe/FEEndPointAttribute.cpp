/**
 *	\file	dice/src/fe/FEEndPointAttribute.cpp
 *	\brief	contains the implementation of the class CFEEndPointAttribute
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

#include "fe/FEEndPointAttribute.h"
#include "File.h"
#include "Vector.h"

/////////////////////////////////////////////////////////////////////////////////
// PortSpec - helper class 

IMPLEMENT_DYNAMIC(CFEPortSpec) 

CFEPortSpec::CFEPortSpec()
{
    IMPLEMENT_DYNAMIC_BASE(CFEPortSpec, CFEBase);

}

CFEPortSpec::CFEPortSpec(String FamilyString, String PortString)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPortSpec, CFEBase);

    m_sFamily = FamilyString;
    m_sPort = PortString;
}

CFEPortSpec::CFEPortSpec(String PortStr)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPortSpec, CFEBase);

    int iColon = PortStr.Find(':');
    if (iColon >= 0)
      {
	  m_sPort = PortStr.Mid(iColon + 1);
	  m_sFamily = PortStr.Left(iColon);
      }
    else
      {
	  m_sPort.Empty();
	  m_sFamily = PortStr;
      }
}

CFEPortSpec::CFEPortSpec(CFEPortSpec & src):CFEBase(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEPortSpec, CFEBase);

    m_sFamily = src.m_sFamily;
    m_sPort = src.m_sPort;
}

/** cleans up the port spec object */
CFEPortSpec::~CFEPortSpec()
{

}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEPortSpec::Clone()
{
    return new CFEPortSpec(*this);
}

/** function to access family string
 *	\return family string
 */
String CFEPortSpec::GetFamiliyString()
{
    return m_sFamily;
}

/** function to access port string
 *	\return port string
 */
String CFEPortSpec::GetPortString()
{
    return m_sPort;
}

/** serializes object to/from file
 *	\param pFile the file to serialize from/to
 */
void CFEPortSpec::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
	  if (m_sPort.IsEmpty())
	      pFile->Print("%s", (const char *) m_sFamily);
	  else
	      pFile->Print("%s:%s", (const char *) m_sFamily, (const char *) m_sPort);
      }
}

//////////////////////////////////////////////////////////////////////////////////
// End Point Attribute

IMPLEMENT_DYNAMIC(CFEEndPointAttribute) CFEEndPointAttribute::CFEEndPointAttribute(Vector * pPortSpecs):CFEAttribute
    (ATTR_ENDPOINT)
{
    IMPLEMENT_DYNAMIC_BASE(CFEEndPointAttribute, CFEAttribute);

    m_pPortSpecs = pPortSpecs;
}

CFEEndPointAttribute::CFEEndPointAttribute(CFEEndPointAttribute & src):CFEAttribute(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEEndPointAttribute, CFEAttribute);

    if (src.m_pPortSpecs)
      {
	  m_pPortSpecs = src.m_pPortSpecs->Clone();
	  m_pPortSpecs->SetParentOfElements(this);
      }
    else
	m_pPortSpecs = 0;
}

/** cleans up the end-point attribute (delete all port-specs) */
CFEEndPointAttribute::~CFEEndPointAttribute()
{
    if (m_pPortSpecs)
	delete m_pPortSpecs;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEEndPointAttribute::Clone()
{
    return new CFEEndPointAttribute(*this);
}

/** retrieves a pointer to the first port spec
 *	\return a pointer to the first port spec
 */
VectorElement *CFEEndPointAttribute::GetFirstPortSpec()
{
    if (!m_pPortSpecs)
	return 0;
    return m_pPortSpecs->GetFirst();
}

/** retrieves the next port spec
 *	\param iter the pointer to the next port spec
 *	\return a reference to the next port specification
 */
CFEPortSpec *CFEEndPointAttribute::GetNextPortSpec(VectorElement * &iter)
{
    if (!m_pPortSpecs)
	return 0;
    if (!iter)
	return 0;
    CFEPortSpec *pRet = (CFEPortSpec *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** serializes this object to/from a file
 *	\param pFile the file to serialize from/to
 */
void CFEEndPointAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<attribute>endpoint(");
        VectorElement *pIter = GetFirstPortSpec();
        CFEPortSpec *pSpec;
        while ((pSpec = GetNextPortSpec(pIter)) != 0)
        {
            pSpec->Serialize(pFile);
            if (pIter)
                if (pIter->GetElement())
                    pFile->Print(", ");
        }
        pFile->Print(")</attribute>\n");
    }
}
