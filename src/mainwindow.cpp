#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <stdlib.h>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->setAttribute(Qt::WA_TranslucentBackground, true);
	this->setWindowFlags(Qt::FramelessWindowHint);
	this->setGeometry(0, 0, 300, 480);

	//QPalette pal = ui->textBrowser->palette();
	//pal.setBrush(QPalette::Base, QBrush(Qt::NoBrush));
	//pal.setColor(QPalette::Text, Qt::white);
	//ui->textBrowser->setPalette(pal);
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
	QMessageBox msgbox(QMessageBox::Question, NULL, errmsg + "\npress No reboot the system",
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
	msgbox.setText(errmsg + "\npress OK to reboot the system");
	msgbox.exec();

	system("reboot -f");
	close();
}
