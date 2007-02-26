#ifndef PROCESSVIEWER_H
#define PROCESSVIEWER_H

#include <qmainwindow.h>

class QListView;
class QListViewItem;
class QPopupMenu;
class L4Thread;

class ProcessViewer : public QMainWindow
{
	Q_OBJECT

	public:
		ProcessViewer();

	protected slots:
		void timerEvent(QTimerEvent *e);

	private slots:
		void displayPopup(QListViewItem *item, const QPoint& pos, int col);
		void queryPopup(int id);

	private:
		void showThreads();

		QListView *view;
		QMap<L4Thread, int> times;
		bool nested;
		QPopupMenu *threadmenu;
};

#endif

