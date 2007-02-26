#include <unistd.h>
#include <stdlib.h>
#include <qapplication.h>

#include "qrunwindow.h"

extern bool init();

int main(int argc, char **argv) {

  // wait for file systems to be mounted by 'fstab'
  sleep(5);

  if (!init())
    exit(1);

  QApplication app(argc, argv);

  QRunWindow *w = new QRunWindow();
  app.setMainWidget(w);
  w->show();

  return app.exec();
}
