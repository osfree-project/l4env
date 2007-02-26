extern "C" {
#include <l4/log/l4log.h>
#include <l4/util/util.h>
}

#include <qobject.h>

class C: public QObject {
  
  Q_OBJECT

 public:
  ~C() {};

 signals:
  void signal(int);

 private slots:
  void slot(int) { LOG_Enter(""); };

 public:
  void work(int n) { emit signal(n); };
};
