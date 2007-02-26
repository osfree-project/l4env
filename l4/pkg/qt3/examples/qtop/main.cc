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
  QApplication app(argc, argv);
  app.setStyle(new QCDEStyle());
  app.qwsSetDecoration(new QWSHydroDecoration());

  ProcessViewer *viewer = new ProcessViewer();
  app.setMainWidget(viewer);

  return app.exec();
}

