#ifndef QMOUSEDROPS_QWS_H
#define QMOUSEDROPS_QWS_H

#ifndef QT_H
#include "qmouse_qws.h"
#endif // QT_H

#ifndef QT_NO_QWS_MOUSE_DROPS

class QWSDROPSMouseHandlerPrivate;

class QWSDROPSMouseHandler : public QWSMouseHandler
{
public:
    QWSDROPSMouseHandler( const QString & = QString::null, const QString & = QString::null );
    ~QWSDROPSMouseHandler();

protected:
    QWSDROPSMouseHandlerPrivate *d;
};

#endif

#endif

