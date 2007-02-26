#include "qkbddrops_qws.h"

#ifndef QT_NO_QWS_KBD_DROPS
#include "qsocketnotifier.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern void drops_qws_init_devices(void);
extern int drops_qws_channel_keyboard(void);
extern int drops_qws_qt_key(int key);
extern int drops_qws_qt_keycode(int key, int press);

class QWSDROPSKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSDROPSKbPrivate( QWSDROPSKeyboardHandler *h, const QString& );
    virtual ~QWSDROPSKbPrivate();

private slots:
    void readKeyboardData();

private:
    QWSKeyboardHandler *handler;
    int keyboardFD;
};

QWSDROPSKeyboardHandler::QWSDROPSKeyboardHandler(const QString &device)
{
    d = new QWSDROPSKbPrivate( this, device );
}

QWSDROPSKeyboardHandler::~QWSDROPSKeyboardHandler()
{
    delete d;
}

QWSDROPSKbPrivate::QWSDROPSKbPrivate( QWSDROPSKeyboardHandler *h, const QString &device )
: handler(h)
{
    drops_qws_init_devices();
    keyboardFD = drops_qws_channel_keyboard();

    QSocketNotifier *keyboardNotifier;
    keyboardNotifier = new QSocketNotifier( keyboardFD, QSocketNotifier::Read, this );
    connect(keyboardNotifier, SIGNAL(activated(int)),this, SLOT(readKeyboardData()));
}

QWSDROPSKbPrivate::~QWSDROPSKbPrivate()
{
}

void QWSDROPSKbPrivate::readKeyboardData()
{
	int message[3];
	int key, press, unicode, qtkey;

	read(keyboardFD, message, sizeof(message));

	key = message[1];
	press = message[2];
	unicode = drops_qws_qt_keycode(key, press);
	qtkey = drops_qws_qt_key(key);

	handler->processKeyEvent(unicode, qtkey, 0, press, false);
}

#include "qkbddrops_qws.moc"

#endif // QT_NO_QWS_KBD_DROPS

