#include "backend.h"
#include "checker/mmcblk_checker.h"
using namespace sysck;

backend::backend(int argc, char *argv[], QObject *parent)
	: QThread(parent)
	, conf(new sysck::config(argc, argv))
{
}

void backend::run()
{
	for (auto &item : conf->disks) {
		mmcblk_checker *checker = new mmcblk_checker(item.c_str(),
													 conf->format_type,
													 conf->fsck_timeout);
		QThread *thread = new QThread;
		checker->moveToThread(thread);
		thread->moveToThread(thread);
		thread->start();

		//qRegisterMetaType<msg_level>("msg_level");
		QObject::connect(this, SIGNAL(start_check()),
						 checker, SLOT(execute()));
		QObject::connect(parent(), SIGNAL(repartition()),
						 checker, SLOT(rebuild_partition()));
		QObject::connect(parent(), SIGNAL(stop_check()),
						 thread, SLOT(quit()));
		QObject::connect(checker, SIGNAL(finished()),
						 thread, SLOT(quit()));
		QObject::connect(checker, SIGNAL(state_msg(QString, int)),
						 parent(), SLOT(on_state_msg(QString, int)));

		emit start_check();
		thread->wait();

		QObject::disconnect(this, SIGNAL(start_check()));
		QObject::disconnect(parent(), SIGNAL(repartition()));
		QObject::disconnect(parent(), SIGNAL(stop_check()));
		checker->disconnect();
		delete thread;
		delete checker;
	}

	QObject::connect(this, SIGNAL(finished()), parent(), SLOT(close()));
}
