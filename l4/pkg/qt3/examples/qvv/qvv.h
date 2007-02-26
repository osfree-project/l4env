/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1999-2002
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id$
 *
 */

#ifndef _QVV_H_
#define _QVV_H_

#include <qwidget.h>
#include <qimage.h>
#include <qlistview.h>
#include <qlineedit.h>
#include <qdir.h>
#include <qaccel.h>
#include <qapplication.h>
#include <qevent.h>
#include <qnetwork.h>
#include <qnetworkprotocol.h>
#include <qurloperator.h>
#include <qmainwindow.h>
#include <qstatusbar.h>
#include <qtoolbar.h>
#include <qapplication.h>
#include <qtextedit.h>

extern int ViewX; // view x pos
extern int ViewY; // view y pos
extern int DeskW; // desktop width
extern int DeskH; // desktop height

#define MINSCALE    10
#define MAXSCALE   500

#ifndef MIN
#define MIN(m,n) ( (m) < (n) ? (m) : (n) )
#endif

extern QApplication *app;

class qvvBrowser;
class qvvMainWindow;

class qvvView : public QWidget
{
    Q_OBJECT
  
    int xpos;
    int ypos;
    qvvMainWindow *mw;

    QPixmap pm;
    
    char fname[1024];
    
    int loaded;
    int scale; // in %'s
    
    int mouse_pan;
    int mouse_pan_x;
    int mouse_pan_y;
    
    int rotation;
    
  public:
    
    qvvView( qvvMainWindow* a_mw );
    ~qvvView();
        
    void load( const char* a_fname );
    void moverel( int dx, int dy );
    void moveabs( int ax, int ay );
    void reView( int a_scale );
  
    void statusMsg( const char* msg );
    
    void setDesktopBackground()
      { app->desktop()->setBackgroundPixmap( pm ); }
    
    qvvMainWindow* getMainWindow() { return (qvvMainWindow*)parent();};
  
  protected:  
    void keyPressEvent ( QKeyEvent * e );
    void mousePressEvent ( QMouseEvent * e );
    void mouseReleaseEvent ( QMouseEvent * e );
    void mouseMoveEvent ( QMouseEvent * e );
    void paintEvent( QPaintEvent * e );
    void closeEvent( QCloseEvent * e ) { ViewX = xpos; ViewY = ypos; QWidget::closeEvent( e ); };
};

class qvvBrowser : public QListView
{
    Q_OBJECT

    friend class qvvView;
    
    qvvMainWindow *mw;
    
  public:
    QUrlOperator   op;

    qvvBrowser( QWidget * parent );
    ~qvvBrowser();

    void find( int c );

    void statusMsg( const char* msg );

  public slots:
    void slotCopyFinished( QNetworkOperation * op );
    qvvMainWindow* getMainWindow() { return (qvvMainWindow*)parent();};
    
  protected:
    virtual void keyPressEvent ( QKeyEvent * e );
    virtual void contentsDragMoveEvent ( QDragMoveEvent * e );
    virtual void contentsDropEvent ( QDropEvent * e );
    virtual void contentsMouseMoveEvent ( QMouseEvent * e );
};

class qvvNote : public QTextEdit
{
    Q_OBJECT

    QString file_name;
  
  public:

    void load( const char* a_file_name );
    void save();
  
  protected:
    virtual void keyPressEvent ( QKeyEvent * e );
};

class qvvMainWindow : public QMainWindow
{
    Q_OBJECT
    
    int optThumbs;
    
  public:
    QDir           cdir;
    QList<qvvView> views;
    
    qvvBrowser     *vb;
    QStatusBar     *sb;
    QToolBar       *tb;
    
    qvvMainWindow( const char *startdir );
    ~qvvMainWindow();

    void loadDir( const char* path );
    void loadThumbs();
    void closeAll();
    void goToDir( int mode );
    void Enter( QListViewItem * );  


    void showImageNote( qvvView *a_view = NULL );

    void statusMsg( const char* msg )
      { if ( sb ) sb->message( msg ); };

    void removeItem( QString name );
  
    void handleKeyPressEvent ( QKeyEvent * e ) 
      { keyPressEvent( e ); e->accept(); };
  
  public slots:
    void slotNewWindow();
    void slotGoUp();
    void slotGoNext();
    void slotGoPrev();
    void slotReload();
    void slotGoHome();
    void slotDelete();
    void slotGoToDir();
    void slotThumbs();
    void slotEnter(  QListViewItem * lvi ) { Enter( lvi ); };
    void slotEnter() { Enter( vb->currentItem() ); };
    void slotRClick( QListViewItem *, const QPoint &, int );
  
  protected:
    virtual void keyPressEvent ( QKeyEvent * e );
};

#endif//_QVV_H_
