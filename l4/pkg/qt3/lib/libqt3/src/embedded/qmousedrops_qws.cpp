#include "qmousedrops_qws.h"

#ifndef QT_NO_QWS_MOUSE_DROPS
#include "qwindowsystem_qws.h"
#include "qsocketnotifier.h"
#include "qsocket.h"
#include "qapplication.h"
#include "qgfx_qws.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// maximum number of mouse event messages to be read from the socket
// at once
#define MAX_READ_MESSAGES 64

extern void drops_qws_init_devices(void);
extern int drops_qws_channel_mouse(void);

class QWSDROPSMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QWSDROPSMouseHandlerPrivate( QWSDROPSMouseHandler *h );
    ~QWSDROPSMouseHandlerPrivate();

private slots:
    void readMouseData();

private:
    typedef struct message_s {
        int type;
        int val0, val1;
    } message_t;

    message_t *buffer;

    //int prevstate;
    int x, y, button;
    QWSDROPSMouseHandler *handler;
    QSocket *socket;
};

QWSDROPSMouseHandler::QWSDROPSMouseHandler( const QString &, const QString & )
{
    d = new QWSDROPSMouseHandlerPrivate( this );
}

QWSDROPSMouseHandler::~QWSDROPSMouseHandler()
{
    delete d;
}

QWSDROPSMouseHandlerPrivate::QWSDROPSMouseHandlerPrivate( QWSDROPSMouseHandler *h )
    : handler( h )
{
    drops_qws_init_devices();

    buffer = new message_t[MAX_READ_MESSAGES];
    socket = new QSocket(this);
    socket->setSocket(drops_qws_channel_mouse());

    connect(socket, SIGNAL(readyRead()), this, SLOT(readMouseData()));
}

QWSDROPSMouseHandlerPrivate::~QWSDROPSMouseHandlerPrivate()
{
    delete buffer;
    if (socket)
	socket->close();
}

void QWSDROPSMouseHandlerPrivate::readMouseData()
{
    message_t *message     = buffer;
    int        numBytes    = socket->bytesAvailable();
    int        numMessages = QMIN(numBytes / sizeof(*message), MAX_READ_MESSAGES);

    if (numBytes % sizeof(*message) != 0)
        qDebug("Incomplete mouse event message waiting in socket");

    // read lots of mouse event messages from socket
    socket->readBlock((char *) buffer, numMessages * sizeof(*message));

    int  i;
    bool posted = true;

    // process mouse event messages
    for (i = 0; i < numMessages; i++, message++) {

        if (message->type == 1)	{
            
            x      = message->val0;
            y      = message->val1;
            posted = false;
            
        } else if (message->type == 0) {
            
            button = message->val0;
            if (button == 1)     button = Qt::LeftButton;
            else if(button == 2) button = Qt::RightButton;
            else if(button == 3) button = Qt::MidButton;
            
            // mouse button events are always posted immediately
            QPoint pt(x, y);
            handler->mouseChanged(pt, button);            
            posted = true;
        }
    }
    
    if ( !posted) {
        // mouse motion events were compressed previously, send them
        // now
        QPoint pt(x, y);
        handler->mouseChanged(pt, button);
    }
}

#include "qmousedrops_qws.moc"
#endif //QT_NO_QWS_MOUSE_DROPS
