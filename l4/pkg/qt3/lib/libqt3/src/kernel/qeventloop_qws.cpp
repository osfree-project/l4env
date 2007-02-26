/****************************************************************************
** $Id$
**
** Implementation of QEventLoop class
**
** Copyright (C) 2000-2003 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Qt/Embedded may use this file in accordance with the
** Qt Embedded Commercial License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qeventloop_p.h" // includes qplatformdefs.h
#include "qeventloop.h"
#include "qapplication.h"
#include "qbitarray.h"
#include "qwscommand_qws.h"
#include "qwsdisplay_qws.h"
#include "qwsevent_qws.h"
#include "qwindowsystem_qws.h"
#include "qptrqueue.h"

#if defined(QT_THREAD_SUPPORT)
#  include "qmutex.h"
#endif // QT_THREAD_SUPPORT

#include <errno.h>

#if defined(Q_OS_DROPS)
//extern int drops_qws_get_mouse(int *x, int *y, int *button);
//extern int drops_qws_get_keyboard(int *key, int *press);
//extern int drops_qws_get_keymodifiers(int *alt, int *shift, int *ctrl, int *caps, int *num);
//extern int drops_qws_qt_key(int key);
//extern int drops_qws_qt_keycode(int key, int press);
extern void drops_qws_refresh_screen(void);
#endif

// from qapplication.cpp
extern bool qt_is_gui_used;

// from qeventloop_unix.cpp
extern timeval *qt_wait_timer();
extern void cleanupTimers();

// from qapplication_qws.cpp
extern QWSDisplay* qt_fbdpy; // QWS `display'
QLock *QWSDisplay::lock = 0;

bool qt_disable_lowpriority_timers=FALSE;

// ### this needs to go away at some point...
typedef void (*VFPTR)();
typedef QValueList<VFPTR> QVFuncList;
void qt_install_preselect_handler( VFPTR );
void qt_remove_preselect_handler( VFPTR );
static QVFuncList *qt_preselect_handler = 0;
void qt_install_postselect_handler( VFPTR );
void qt_remove_postselect_handler( VFPTR );
static QVFuncList *qt_postselect_handler = 0;
void qt_install_preselect_handler( VFPTR handler )
{
    if ( !qt_preselect_handler )
	qt_preselect_handler = new QVFuncList;
    qt_preselect_handler->append( handler );
}
void qt_remove_preselect_handler( VFPTR handler )
{
    if ( qt_preselect_handler ) {
	QVFuncList::Iterator it = qt_preselect_handler->find( handler );
	if ( it != qt_preselect_handler->end() )
		qt_preselect_handler->remove( it );
    }
}
void qt_install_postselect_handler( VFPTR handler )
{
    if ( !qt_postselect_handler )
	qt_postselect_handler = new QVFuncList;
    qt_postselect_handler->prepend( handler );
}
void qt_remove_postselect_handler( VFPTR handler )
{
    if ( qt_postselect_handler ) {
	QVFuncList::Iterator it = qt_postselect_handler->find( handler );
	if ( it != qt_postselect_handler->end() )
		qt_postselect_handler->remove( it );
    }
}


void QEventLoop::init()
{
    // initialize the common parts of the event loop
#if !defined(Q_OS_DROPS)
    pipe( d->thread_pipe );
    fcntl(d->thread_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(d->thread_pipe[1], F_SETFD, FD_CLOEXEC);
#else
    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, d->thread_pipe) < 0)
	qDebug("failed to create socket pair");
#endif

    d->sn_highest = -1;
}

void QEventLoop::cleanup()
{
    // cleanup the common parts of the event loop
    close( d->thread_pipe[0] );
    close( d->thread_pipe[1] );
    cleanupTimers();
}

#if defined(Q_OS_LINUX) && !defined(QT_NO_QWS_KEYBOARD) && !defined(QT_NO_QWS_KBD_TTY)
#include <signal.h> //for sig_atomic_t
// from qkbdtty_qws.cpp:
extern volatile sig_atomic_t qt_qws_tty_signal;
extern void qt_qws_handle_tty_signal();
#endif

bool QEventLoop::processEvents( ProcessEventsFlags flags )
{
    // process events from the QWS server
    int	   nevents = 0;
#if defined(Q_OS_DROPS)
//    int x, y, button;
//    int key, qtkey, press;
//    int mod, unicode;
//    int alt, shift, ctrl, caps, num;
#endif

#if defined(QT_THREAD_SUPPORT)
    QMutexLocker locker( QApplication::qt_mutex );
#endif

    // handle gui and posted events
    if (qt_is_gui_used ) {
	QApplication::sendPostedEvents();

	while ( qt_fbdpy->eventPending() ) {	// also flushes output buffer
	    if ( d->shortcut ) {
		return FALSE;
	    }

	    QWSEvent *event = qt_fbdpy->getEvent();	// get next event
	    nevents++;

	    bool ret = qApp->qwsProcessEvent( event ) == 1;
	    delete event;
	    if ( ret ) {
		return TRUE;
	    }
	}
    }

    if ( d->shortcut ) {
	return FALSE;
    }

    extern QPtrQueue<QWSCommand> *qt_get_server_queue();
    if ( !qt_get_server_queue()->isEmpty() ) {
	QWSServer::processEventQueue();
    }

    QApplication::sendPostedEvents();

    // don't block if exitLoop() or exit()/quit() has been called.
    bool canWait = d->exitloop || d->quitnow ? FALSE : (flags & WaitForMore);

    // Process timers and socket notifiers - the common UNIX stuff

    // return the maximum time we can wait for an event.
    static timeval zerotm;
    timeval *tm = qt_wait_timer();		// wait for timer or event
    if ( !canWait ) {
	if ( !tm )
	    tm = &zerotm;
	tm->tv_sec  = 0;			// no time to wait
	tm->tv_usec = 0;
    }

    int highest = 0;
    if ( ! ( flags & ExcludeSocketNotifiers ) ) {
	// return the highest fd we can wait for input on
	if ( d->sn_highest >= 0 ) {                     // has socket notifier(s)
	    if ( d->sn_vec[0].list && ! d->sn_vec[0].list->isEmpty() )
		d->sn_vec[0].select_fds = d->sn_vec[0].enabled_fds;
	    else
		FD_ZERO( &d->sn_vec[0].select_fds );

	    if ( d->sn_vec[1].list && ! d->sn_vec[1].list->isEmpty() )
		d->sn_vec[1].select_fds = d->sn_vec[1].enabled_fds;
	    else
		FD_ZERO( &d->sn_vec[1].select_fds );

	    if ( d->sn_vec[2].list && ! d->sn_vec[2].list->isEmpty() )
		d->sn_vec[2].select_fds = d->sn_vec[2].enabled_fds;
	    else
		FD_ZERO( &d->sn_vec[2].select_fds );
	} else {
	    FD_ZERO( &d->sn_vec[0].select_fds );
	    FD_ZERO( &d->sn_vec[1].select_fds );
	    FD_ZERO( &d->sn_vec[2].select_fds );
	}

	highest = d->sn_highest;
    } else {
	FD_ZERO( &d->sn_vec[0].select_fds );
	FD_ZERO( &d->sn_vec[1].select_fds );
	FD_ZERO( &d->sn_vec[2].select_fds );
    }

    FD_SET( d->thread_pipe[0], &d->sn_vec[0].select_fds );
    highest = QMAX( highest, d->thread_pipe[0] );

    if ( canWait )
	emit aboutToBlock();

    if ( qt_preselect_handler ) {
	QVFuncList::Iterator it, end = qt_preselect_handler->end();
	for ( it = qt_preselect_handler->begin(); it != end; ++it )
	    (**it)();
    }

    // unlock the GUI mutex and select.  when we return from this function, there is
    // something for us to do
#if defined(QT_THREAD_SUPPORT)
    locker.mutex()->unlock();
#endif

#if defined(Q_OS_DROPS)
    if(qApp->type() == QApplication::GuiServer) {
    // mouse (drops->qt translation done here)
//    if(drops_qws_get_mouse(&x, &y, &button))
//    {
//        //qDebug("--> %i %i %i\n", x, y, button);
//        if(button == 1) button = Qt::LeftButton;
//	else if(button == 2) button = Qt::RightButton;
//	else if(button == 3) button = Qt::MidButton;
//        QWSServer::sendMouseEvent(QPoint(x, y), button);
//    }
    // keyboard (drops->qt translation done in driver):
//    if(drops_qws_get_keyboard(&key, &press))
//    {
//	drops_qws_get_keymodifiers(&alt, &shift, &ctrl, &caps, &num);
//	mod = (alt ? /*Key_Alt*/ALT : 0)
//		| (shift ? /*Key_Shift*/SHIFT : 0)
//		| (ctrl ? /*Key_Control*/CTRL : 0)
//		| (caps ? /*Key_CapsLock*/0 : 0)
//		| (num ? /*Key_NumLock*/0 : 0);
//	unicode = drops_qws_qt_keycode(key, press);
//	qtkey = drops_qws_qt_key(key);
//qDebug("### KEY EVENT unicode=%i key=%i mod=%i press=%i autorepeat=%i (raw code %i)\n", unicode, qtkey, mod, press, false, key);
//        QWSServer::processKeyEvent(unicode, qtkey, mod, press, false);
//    }

    drops_qws_refresh_screen();
    }
#endif

    int nsel;
    do {
#if defined(Q_OS_LINUX) && !defined(QT_NO_QWS_KEYBOARD) && !defined(QT_NO_QWS_KBD_TTY)
	while (qt_qws_tty_signal) {
	    qt_qws_tty_signal=0;
	    qt_qws_handle_tty_signal();
	}
#endif
	nsel = select( highest + 1,
		       &d->sn_vec[0].select_fds,
		       &d->sn_vec[1].select_fds,
		       &d->sn_vec[2].select_fds,
		       tm );
    } while (nsel == -1 && (errno == EINTR || errno == EAGAIN));

    // relock the GUI mutex before processing any pending events
#if defined(QT_THREAD_SUPPORT)
    locker.mutex()->lock();
#endif

    // we are awake, broadcast it
    emit awake();
    emit qApp->guiThreadAwake();

    if (nsel == -1) {
	if (errno == EBADF) {
	    // it seems a socket notifier has a bad fd... find out
	    // which one it is and disable it
	    fd_set fdset;
	    zerotm.tv_sec = zerotm.tv_usec = 0l;

	    for (int type = 0; type < 3; ++type) {
		QPtrList<QSockNot> *list = d->sn_vec[type].list;
		if (!list) continue;

		QSockNot *sn = list->first();
		while (sn) {
		    FD_ZERO(&fdset);
		    FD_SET(sn->fd, &fdset);

		    int ret;
		    do {
			switch (type) {
			case 0: // read
			    ret = select(sn->fd + 1, &fdset, 0, 0, &zerotm);
			    break;
			case 1: // write
			    ret = select(sn->fd + 1, 0, &fdset, 0, &zerotm);
			    break;
			case 2: // except
			    ret = select(sn->fd + 1, 0, 0, &fdset, &zerotm);
			    break;
			}
		    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

		    if (ret == -1 && errno == EBADF) {
			// disable the invalid socket notifier
			static const char *t[] = { "Read", "Write", "Exception" };
			qWarning("QSocketNotifier: invalid socket %d and type '%s', disabling...",
				 sn->fd, t[type]);
			sn->obj->setEnabled(false);
		    }

		    sn = list->next();
		}
	    }
	} else {
	    // EINVAL... shouldn't happen, so let's complain to stderr
	    // and hope someone sends us a bug report
	    perror( "select" );
	}
    }

    // some other thread woke us up... consume the data on the thread pipe so that
    // select doesn't immediately return next time
    if ( nsel > 0 && FD_ISSET( d->thread_pipe[0], &d->sn_vec[0].select_fds ) ) {
	char c;
	::read( d->thread_pipe[0], &c, 1 );
    }

    if ( qt_postselect_handler ) {
	QVFuncList::Iterator it, end = qt_postselect_handler->end();
	for ( it = qt_postselect_handler->begin(); it != end; ++it )
	    (**it)();
    }

    // activate socket notifiers
    if ( ! ( flags & ExcludeSocketNotifiers ) && nsel > 0 && d->sn_highest >= 0 ) {
	// if select says data is ready on any socket, then set the socket notifier
	// to pending
	int i;
	for ( i=0; i<3; i++ ) {
	    if ( ! d->sn_vec[i].list )
		continue;

	    QPtrList<QSockNot> *list = d->sn_vec[i].list;
	    QSockNot *sn = list->first();
	    while ( sn ) {
		if ( FD_ISSET( sn->fd, &d->sn_vec[i].select_fds ) )
		    setSocketNotifierPending( sn->obj );
		sn = list->next();
	    }
	}

	nevents += activateSocketNotifiers();
    }

    // activate timers
    nevents += activateTimers();

    // return true if we handled events, false otherwise
    return (nevents > 0);
}

bool QEventLoop::hasPendingEvents() const
{
    extern uint qGlobalPostedEventsCount(); // from qapplication.cpp
    return qGlobalPostedEventsCount() || qt_fbdpy->eventPending();
}
