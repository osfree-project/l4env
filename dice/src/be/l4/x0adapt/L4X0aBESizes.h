
#ifndef L4X0aBESIZES_H
#define L4X0aBESIZES_H


#include "be/l4/L4BESizes.h"

/** \class CL4X0aBESizes
 *  \brief implementes L4 X0 specific sizes
 *
 * Ronald Aigner
 **/
class CL4X0aBESizes : public CL4BESizes
{

public:
    /** creates a sizes object */
    CL4X0aBESizes();
    virtual ~CL4X0aBESizes();

public: // Public methods
    virtual int GetSizeOfEnvType(string sName);
    virtual int GetMaxShortIPCSize(int nDirection);
};

#endif
