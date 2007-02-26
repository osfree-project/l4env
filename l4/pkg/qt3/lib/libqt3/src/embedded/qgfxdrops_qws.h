#ifndef QGFXDROPS_QWS_H
#define QGFXDROPS_QWS_H

#if defined(Q_OS_DROPS)
#include "qgfx_qws.h"

class QDropsScreen : public QScreen
{
public:
    QDropsScreen( int display_id );
    virtual ~QDropsScreen();

    virtual bool initDevice();
    virtual bool connect( const QString &displaySpec );

    virtual bool useOffscreen() { return FALSE; }

    virtual void disconnect();
//    virtual void shutdownDevice();
    virtual void setMode(int,int,int);
//    virtual void save();
//    virtual void restore();
//    virtual void blank(bool on);
//    virtual void set(unsigned int,unsigned int,unsigned int,unsigned int);
//    virtual uchar * cache(int,int);
//    virtual void uncache(uchar *);
//    virtual int sharedRamSize(void *);

};

#endif

#endif // QGFXDROPS_QWS_H
