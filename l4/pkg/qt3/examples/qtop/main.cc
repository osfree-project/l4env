extern "C" {
#include <l4/log/l4log.h>
#include <l4/util/util.h>
}

#include <qapplication.h>
// make it sexy :)
#include <qcdestyle.h>
#include <qwshydrodecoration_qws.h>

#include "processviewer.h"

int
main(int argc, char **argv)
{
  // wait for file systems to be mounted by 'fstab'
  l4_sleep(2000);

  LOG("QWS-Server: starting up ...");

  QApplication app(argc, argv);
  app.setStyle(new QCDEStyle());
  app.qwsSetDecoration(new QWSHydroDecoration());

  LOG("QWS-Server: Setting up main window");

  ProcessViewer *viewer = new ProcessViewer();
  app.setMainWidget(viewer);

  LOG("QWS-Server: starting main loop ...");
  return app.exec();
}

