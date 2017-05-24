#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <stdlib.h>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_check_state(QString state)
{
	ui->textBrowser->append(state);
}

void MainWindow::on_check_error(QString errmsg)
{
	QMessageBox msgbox(QMessageBox::Question, NULL, errmsg + "\n press No reboot the system",
					   QMessageBox::Yes | QMessageBox::No);

	if (msgbox.exec() == QMessageBox::Yes) {
		emit check_return(true);
	} else {
		emit check_return(false);
		system("reboot -f");
		close();
	}
}

void MainWindow::on_check_fatal(QString errmsg)
{
	QMessageBox msgbox;
	msgbox.setText(errmsg + "\n press OK to reboot the system");
	msgbox.exec();

	system("reboot -f");
	close();
}
