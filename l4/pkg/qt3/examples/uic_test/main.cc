#include <unistd.h>
#include <qapplication.h>
#include "mymainwindow.h"

int main( int argc, char ** argv )
{
sleep(2);

    QApplication a( argc, argv );
    MyMainWindow w;
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}
