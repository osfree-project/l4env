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
#include "template.h"
#include <string>

class CFEEnumDeclarator;

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
    virtual void CreateBackEnd(CFETypeSpec *pFEType);
    virtual void WriteZeroInit(CBEFile& pFile);
    virtual void Write(CBEFile& pFile);
    virtual void WriteCastToStr(std::string& str, bool bPointer);

    virtual bool HasTag(std::string sTag);
    virtual std::string GetTag();

    virtual long int GetIntValue(std::string sName);

protected:
    void AddMember(CFEEnumDeclarator* pFEDeclarator);

protected: // Protected attributes
    /** \var std::string m_sTag
     *  \brief the tag if the source is a tagged struct
     */
    std::string m_sTag;

public:
    /** \var CSearchableCollection<CBEDeclarator, std::string> m_Members
     *  \brief contains the enumerators
     */
    CSearchableCollection<CBEDeclarator, std::string> m_Members;
};

#endif
