#include "mainwindow.h"
#include <QApplication>
#include "backend.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	w.showFullScreen();

	backend bk(argc, argv);
	bk.connect(&bk, SIGNAL(check_state(QString)), &w, SLOT(on_check_state(QString)));
	bk.connect(&bk, SIGNAL(check_error(QString)), &w, SLOT(on_check_error(QString)));
	bk.connect(&bk, SIGNAL(check_fatal(QString)), &w, SLOT(on_check_fatal(QString)));
	bk.connect(&bk, SIGNAL(check_finish()), &w, SLOT(close()));
	bk.connect(&w, SIGNAL(check_return(bool)), &bk, SLOT(on_check_return(bool)));
	bk.start();

	return a.exec();
}
