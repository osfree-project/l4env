#include <qglobal.h>

bool init() {

  return true;
}

bool loaderRun(const char *binary) {

    qDebug("running %s", binary);
    return true;
}
