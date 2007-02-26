/**
 *    \file    dice/src/fe/FEEndPointAttribute.cpp
 *    \brief   contains the implementation of the class CFEEndPointAttribute
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

#include "fe/FEEndPointAttribute.h"
#include "File.h"

CFEEndPointAttribute::CFEEndPointAttribute(vector<PortSpec> *pPortSpecs)
: CFEAttribute(ATTR_ENDPOINT)
{
    m_vPortSpecs.swap(*pPortSpecs);
}

CFEEndPointAttribute::CFEEndPointAttribute(CFEEndPointAttribute & src)
: CFEAttribute(src)
{
    vector<PortSpec>::iterator iter = src.m_vPortSpecs.begin();
    for (; iter != src.m_vPortSpecs.end(); iter++)
    {
        m_vPortSpecs.push_back(*iter);
    }
}

/** cleans up the end-point attribute (delete all port-specs) */
CFEEndPointAttribute::~CFEEndPointAttribute()
{
    m_vPortSpecs.clear();
}

/**    creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFEEndPointAttribute::Clone()
{
    return new CFEEndPointAttribute(*this);
}

/** retrieves a pointer to the first port spec
 *    \return a pointer to the first port spec
 */
vector<PortSpec>::iterator CFEEndPointAttribute::GetFirstPortSpec()
{
    return m_vPortSpecs.begin();
}

/** retrieves the next port spec
 *    \param iter the pointer to the next port spec
 *    \return a reference to the next port specification
 */
PortSpec CFEEndPointAttribute::GetNextPortSpec(vector<PortSpec>::iterator &iter)
{
    PortSpec ret;
    ret.sFamily.erase(ret.sFamily.begin(), ret.sFamily.end());
    ret.sPort.erase(ret.sPort.begin(), ret.sPort.end());
    if (iter == m_vPortSpecs.end())
        return ret;
    return *iter++;
}

/** serializes this object to/from a file
 *    \param pFile the file to serialize from/to
 */
void CFEEndPointAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<attribute>endpoint(");
        vector<PortSpec>::iterator iter = m_vPortSpecs.begin();
        for (; iter != m_vPortSpecs.end(); iter++)
        {
            *pFile << (*iter).sFamily;
            if (!(*iter).sPort.empty())
                *pFile << ":" << (*iter).sPort;
            if (iter != m_vPortSpecs.end() - 1)
                *pFile << ", ";
        }
        pFile->Print(")</attribute>\n");
    }
}
