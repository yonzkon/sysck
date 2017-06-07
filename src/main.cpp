#include "mainwindow.h"
#include <QApplication>
#ifdef QWS
#include <QWSServer>
#endif
#include "backend.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

#ifdef QWS
	QWSServer::setBackground(QColor(0,0,0,0));
	QWSServer::setCursorVisible(false);
#endif

	MainWindow w;
	w.show();

	backend bk(argc, argv, &w);
	bk.start();

	return a.exec();
}
