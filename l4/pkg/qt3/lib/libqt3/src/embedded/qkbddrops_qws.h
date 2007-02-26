#ifndef QKBDDROPS_QWS_H
#define QKBDDROPS_QWS_H

#ifndef QT_H
#include "qkbd_qws.h"
#endif // QT_H

#ifndef QT_NO_QWS_KBD_DROPS

class QWSDROPSKbPrivate;

class QWSDROPSKeyboardHandler : public QWSKeyboardHandler
{
public:
    QWSDROPSKeyboardHandler(const QString&);
    virtual ~QWSDROPSKeyboardHandler();

private:
    QWSDROPSKbPrivate *d;
};

#endif // QT_NO_QWS_KBD_DROPS

#endif // QKBDDROPS_QWS_H
