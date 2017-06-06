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

		QObject::connect(this, SIGNAL(start_process_check()),
						 checker, SLOT(process_check()));
		QObject::connect(parent(), SIGNAL(check_return(bool)),
						 checker, SLOT(continue_or_exit(bool)));
		QObject::connect(checker, SIGNAL(state_msg(QString, msg_level)),
						 parent(), SLOT(on_state_msg(QString, msg_level)));

		QThread *thread = new QThread;
		checker->moveToThread(thread);
		thread->start();
		emit start_process_check();
		thread->wait();

		checker->disconnect();
		QObject::disconnect(this, SIGNAL(start_process_check()));
		QObject::disconnect(parent(), SIGNAL(check_return(bool)));
		delete thread;
		delete checker;
	}
}
