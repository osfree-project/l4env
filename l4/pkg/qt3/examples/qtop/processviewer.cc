#include "processviewer.h"
#include "processviewer.moc"

#include <qlistview.h>
#include <qpopupmenu.h>
#include <qmessagebox.h>

#include <l4/sys/kdebug.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

class L4Thread
{
	public:
		L4Thread(){m_id = L4_NIL_ID;}
		L4Thread(l4_threadid_t id){m_id = id;}
		l4_threadid_t id(){return m_id;}
		bool operator<(L4Thread t1) const
		{
			L4Thread t2 = *this;
			return ((t1.id().id.task < t2.id().id.task)
				|| (t1.id().id.task == t2.id().id.task
					&& t1.id().id.lthread < t2.id().id.lthread));
		}
	private:
		l4_threadid_t m_id;
};

ProcessViewer::ProcessViewer()
: QMainWindow()
{
	threadmenu = NULL;

	view = new QListView(this);
	view->addColumn("Thread");
	view->addColumn("Name");
	view->addColumn("CPU Time");
	view->addColumn("Priority");
	view->setShowSortIndicator(true);
	view->setRootIsDecorated(true);

	setCentralWidget(view);

	connect(view,
		SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
		SLOT(displayPopup(QListViewItem*, const QPoint&, int)));

	setCaption("L4 Process Viewer");
	show();

	names_register("qtop");

	nested = true;

	startTimer(1000);
}

void ProcessViewer::showThreads()
{
	l4_threadid_t current, next;
	l4_uint64_t us;
	l4_umword_t prio;
	QMap<L4Thread, QString> names;
	QMap<L4Thread, int> crunchlist;
	QMap<L4Thread, int> priolist;
	char cname[128];
	int ret;
	int totaltime, diff;
	int percent;
	QPtrList<QListViewItem> items;

	for(int i = 0; i < NAMES_MAX_ENTRIES; i++)
	{
		ret = names_query_nr(i, cname, sizeof(cname), &current);
		if(ret)
		{
			L4Thread t(current);
			names[t] = QString(cname);
		}
	}

	totaltime = 0;
	current = L4_NIL_ID;

	do
	{
		fiasco_get_cputime(current, &next, &us, &prio);
		L4Thread t(current);

		diff = us - times[t];
		times[t] = us;
		totaltime += diff;

		crunchlist[t] = diff;
		priolist[t] = prio;

		current = next;
	}
	while(!l4_is_nil_id(next));

	QListViewItemIterator ii(view);
	while(ii.current())
	{
		items.append(ii.current());
		ii++;
	}

	QMap<L4Thread, int>::Iterator it;
	for(it = crunchlist.begin(); it != crunchlist.end(); it++)
	{
		L4Thread t = it.key();
		current = t.id();
		percent = (100 * crunchlist[t] / totaltime);
		prio = priolist[t];
		QString tid = QString("%1.%2").arg(current.id.task).arg(current.id.lthread);
		QString time = QString("%1%").arg(percent);
		QString priority = QString("%1").arg(prio);
		QString name = "-";
		if(names[t]) name = names[t];

		QListViewItem *item = view->findItem(tid, 0);
		if(item)
		{
			if(item->text(1) != name)
				item->setText(1, name);
			if(item->text(2) != time)
				item->setText(2, time);
			if(item->text(3) != priority)
				item->setText(3, priority);

			items.remove(item);
			if(item->parent())
				items.remove(item->parent());
		}
		else
		{
			if(nested)
			{
				QString task = name.section(".", 0, 0);
				QListViewItem *parent = view->findItem(task, 0);
				if(!parent) parent = new QListViewItem(view, task);

				(void)new QListViewItem(parent, tid, name, time, priority);
			}
			else
			{
				(void)new QListViewItem(view, tid, name, time, priority);
			}
		}
	}

	items.setAutoDelete(true);
	items.clear();
	items.setAutoDelete(false);
}

void ProcessViewer::timerEvent(QTimerEvent *e)
{
	showThreads();
}

void ProcessViewer::displayPopup(QListViewItem *item, const QPoint& pos, int col)
{
	delete threadmenu;
	threadmenu = NULL;
	if(!item->parent()) return;

	threadmenu = new QPopupMenu(view);
	threadmenu->insertItem("Kill me :-)");

	connect(threadmenu, SIGNAL(activated(int)), SLOT(queryPopup(int)));

	threadmenu->popup(pos);
}

void ProcessViewer::queryPopup(int id)
{
	l4_threadid_t threadid;
	l4thread_t thread;

	delete threadmenu;
	threadmenu = NULL;
	if(!view->currentItem()) return;

	QStringList l = QStringList::split(".", view->currentItem()->text(0));
	threadid.id.task = l[0].toInt();
	threadid.id.lthread = l[1].toInt();
	thread = l4thread_setup(threadid, NULL, 0, 0);
	int ret = l4thread_shutdown(thread);
	if(ret)
	{
		QMessageBox::critical(this, "Error", "Thread could not be killed.",
			QMessageBox::Ok, QMessageBox::NoButton);
	}
}

