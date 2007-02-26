#include "qgfxdrops_qws.h"

#if defined(Q_OS_DROPS)
#include "qgfxraster_qws.h"
#include "qmemorymanager_qws.h"
#include "qwsdisplay_qws.h"
#include "qwindowsystem_qws.h"

extern long  drops_qws_set_screen(long width, long height, long depth);
extern long  drops_qws_get_scr_width(void);
extern long  drops_qws_get_scr_height(void);
extern long  drops_qws_get_scr_depth(void);
extern long  drops_qws_get_scr_line(void);
extern void *drops_qws_get_scr_adr(void);

QDropsScreen::QDropsScreen( int display_id ) : QScreen( display_id )
{
    drops_qws_set_screen(w, h, d);

    dw = drops_qws_get_scr_width();
    dh = drops_qws_get_scr_height();
    d = drops_qws_get_scr_depth();
    lstep = drops_qws_get_scr_line();
    data = (uchar*)drops_qws_get_scr_adr();

    mapsize = size = h * lstep;
    screencols = 2 ^ d;
    w = dw;
    h = dh;
}

QDropsScreen::~QDropsScreen()
{
}

bool QDropsScreen::initDevice()
{
    return TRUE;
}

bool QDropsScreen::connect( const QString &displaySpec )
{
    return TRUE;
}

void QDropsScreen::disconnect()
{
}

void QDropsScreen::setMode(int,int,int)
{
}

#endif // Q_OS_DROPS
