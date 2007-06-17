/**
 *    \file    dice/src/be/BEEnumType.h
 *  \brief   contains the declaration of the class CBEEnumType
 *
 *    \date    Tue Jul 23 2002
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

#ifndef BEENUMTYPE_H
#define BEENUMTYPE_H

#include <be/BEType.h>
#include <string>
#include <vector>

/** \class CBEEnumType
 *  \ingroup backend
 *  \brief is the type of an enum declarator
 */
class CBEEnumType : public CBEType
{
public:
    /** \brief constructs an enum type class */
    CBEEnumType();
    ~CBEEnumType();

public: // Public methods
    virtual int GetMemberCount();
    virtual string GetMemberAt(unsigned int nIndex);
    virtual void RemoveMember(string sMember);
    virtual void AddMember(string sMember);
    virtual void CreateBackEnd(CFETypeSpec *pFEType);
    virtual void WriteZeroInit(CBEFile& pFile);
    virtual void Write(CBEFile& pFile);
    virtual void WriteCast(CBEFile& pFile, bool bPointer);
    virtual bool HasTag(string sTag);

protected: // Protected attributes
    /** \var vector<string> m_vMembers
     *  \brief contains the members of the enum type
     */
    vector<string> m_vMembers;
    /**    \var string m_sTag
     *  \brief the tag if the source is a tagged struct
     */
    string m_sTag;
};

#endif
