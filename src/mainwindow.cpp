#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <unistd.h>
#include <sys/reboot.h>
#include "msg_type.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->setAttribute(Qt::WA_TranslucentBackground, true);
	this->setWindowFlags(Qt::FramelessWindowHint);
	this->setGeometry(0, 0, 300, 480);

	QPalette pal = ui->textBrowser->palette();
	//pal.setBrush(QPalette::Base, QBrush(Qt::NoBrush));
	pal.setBrush(QPalette::Base, Qt::black);
	pal.setColor(QPalette::Text, Qt::white);
	ui->textBrowser->setPalette(pal);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_state_msg(QString msg, int type)
{
	if (type & MSG_INFO)
		handle_msg_info(msg);

	if (type & MSG_PERMISSION)
		handle_msg_permission(msg);

	if (type & MSG_REBOOT)
		handle_msg_reboot(msg);
}

void MainWindow::handle_msg_info(QString msg)
{
	ui->textBrowser->append(msg);
}

void MainWindow::handle_msg_permission(QString msg)
{
	QMessageBox msgbox(QMessageBox::Question, NULL, msg + "\npress No to reboot the system",
					   QMessageBox::Yes | QMessageBox::No);

	if (msgbox.exec() == QMessageBox::Yes) {
		emit return_permission(true);
	} else {
		emit return_permission(false);
		reboot(0x1234567);
		close();
	}
}

void MainWindow::handle_msg_reboot(QString msg)
{
	QMessageBox msgbox;
	msgbox.setText(msg + "\npress OK to reboot the system");
	msgbox.exec();

	reboot(0x1234567);
	close();
}
