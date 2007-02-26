extern "C" {
#include <l4/log/l4log.h>
#include <l4/util/util.h>
}

#include <qapplication.h>
#include <qmainwindow.h>

int
main(int argc, char **argv)
{
  LOG("QWS-Client: starting up ...");

  QApplication app(argc, argv);

  LOG("QWS-Client: Setting up main window");

  QMainWindow *janela = new QMainWindow();
  janela->setCaption("Client");
  janela->show();
  app.setMainWidget(janela);

  LOG("QWS-Client: starting main loop ...");
  return app.exec();
}

