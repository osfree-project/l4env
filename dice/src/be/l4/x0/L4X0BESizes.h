
#ifndef L4X0BESIZES_H
#define L4X0BESIZES_H


#include "be/l4/L4BESizes.h"

/** \class CL4X0BESizes
 *  \brief implementes L4 X0 specific sizes
 *
 * Ronald Aigner
 **/
class CL4X0BESizes : public CL4BESizes
{

public:
    /** creates a size object */
    CL4X0BESizes();
    virtual ~CL4X0BESizes();

public: // Public methods
    virtual int GetSizeOfEnvType(string sName);
    virtual int GetMaxShortIPCSize(int nDirection);
};

#endif
