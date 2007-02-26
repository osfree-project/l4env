/**
 *    \file    dice/src/fe/FESizeOfExpression.h
 *  \brief    contains the declaration of the class CFESizeOfExpression
 *
 *    \date    07/10/2003
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
#ifndef FESIZEOFEXPRESSION_H
#define FESIZEOFEXPRESSION_H

#include <fe/FEExpression.h>

class CFETypeSpec;

/** \class CFESizeOfExpression
 *  \ingroup frontend
 *  \brief represents an size-of statement
 */
class CFESizeOfExpression : public CFEExpression
{
public:
    /** constructs this object */
    CFESizeOfExpression();
    virtual ~CFESizeOfExpression();

    /** \brief constructs a size-of class
     *  \param sTypeName a type name alias
     */
    CFESizeOfExpression(string sTypeName);
    /** \brief constructs a size-of class
     *  \param pType the type to take the size of
     */
    CFESizeOfExpression(CFETypeSpec *pType);
    /** \brief constructs a size-of class
     *  \param pExpression the expression inside the size of statement
     */
    CFESizeOfExpression(CFEExpression *pExpression);

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFESizeOfExpression(CFESizeOfExpression &src);

public:
    virtual string ToString();
    virtual CObject* Clone();
    virtual CFETypeSpec* GetSizeOfType();
    virtual CFEExpression* GetSizeOfExpression();

protected:
    /** \var CFETypeSpec *m_pType
     *  \brief the type to take the size of
     */
    CFETypeSpec *m_pType;
    /** \var CFEExpression *m_pExpression
     *  \brief the expression inside the size of statement
     */
    CFEExpression *m_pExpression;
};

#endif
