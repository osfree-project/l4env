extern "C" {
#include <l4/log/l4log.h>
#include <l4/util/util.h>
}

#include <qapplication.h>
#include <qmainwindow.h>

int
main(int argc, char **argv)
{
  LOG("QWS-Server: starting up ...");

  QApplication app(argc, argv);

  LOG("QWS-Server: Setting up main window");

  QMainWindow *janela = new QMainWindow();
  janela->setCaption("Server");
  janela->show();
  app.setMainWidget(janela);

  LOG("QWS-Server: starting main loop ...");
  return app.exec();
}

