#include "mainwindow.h"
#include <QApplication>
#ifdef QWS
#include <QWSServer>
#endif
#include "backend.h"

int main(int argc, char *argv[])
{
#ifdef QWS
	QWSServer::setBackground(QColor(0,0,0,0));
#endif

	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	backend bk(argc, argv);
	bk.connect(&bk, SIGNAL(check_state(QString)), &w, SLOT(on_check_state(QString)));
	bk.connect(&bk, SIGNAL(check_error(QString)), &w, SLOT(on_check_error(QString)));
	bk.connect(&bk, SIGNAL(check_fatal(QString)), &w, SLOT(on_check_fatal(QString)));
	bk.connect(&bk, SIGNAL(check_finish()), &w, SLOT(close()));
	bk.connect(&w, SIGNAL(check_return(bool)), &bk, SLOT(on_check_return(bool)));
	bk.start();

	return a.exec();
}
