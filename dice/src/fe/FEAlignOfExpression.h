/**
 *  \file    dice/src/fe/FEAlignOfExpression.h
 *  \brief   contains the declaration of the class CFEAlignOfExpression
 *
 *  \date    07/30/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
#ifndef FEALIGNOFEXPRESSION_H
#define FEALIGNOFEXPRESSION_H

#include <fe/FEExpression.h>

class CFETypeSpec;

/** \class CFEAlignOfExpression
 *  \ingroup frontend
 *  \brief represents an align-of statement
 */
class CFEAlignOfExpression : public CFEExpression
{
public:
    /** constructs this object */
    CFEAlignOfExpression();
    virtual ~CFEAlignOfExpression();

    /** \brief constructs a size-of class
     *  \param sTypeName a type name alias
     */
    CFEAlignOfExpression(std::string sTypeName);
    /** \brief constructs a size-of class
     *  \param pType the type to take the size of
     */
    CFEAlignOfExpression(CFETypeSpec *pType);
    /** \brief constructs a size-of class
     *  \param pExpression the expression inside the size of statement
     */
    CFEAlignOfExpression(CFEExpression *pExpression);

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEAlignOfExpression(CFEAlignOfExpression &src);

public:
    virtual std::string ToString();
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
