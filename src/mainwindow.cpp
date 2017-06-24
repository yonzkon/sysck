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

	if (type & MSG_PERMIT)
		handle_msg_permit(msg);

	if (type & MSG_REBOOT)
		handle_msg_reboot(msg);
}

void MainWindow::handle_msg_info(QString msg)
{
	ui->textBrowser->append(msg);
}

void MainWindow::handle_msg_permit(QString msg)
{
	QMessageBox msgbox(QMessageBox::Question, NULL, msg,
					   QMessageBox::Yes | QMessageBox::No);
	msgbox.setButtonText(QMessageBox::Yes, "continue");
	msgbox.setButtonText(QMessageBox::No, "reboot");

	if (msgbox.exec() == QMessageBox::Yes) {
		emit continue_check(true);
	} else {
		emit stop_check();
		reboot(0x1234567);
		close();
	}
}

void MainWindow::handle_msg_reboot(QString msg)
{
	QMessageBox msgbox;
	msgbox.setText(msg);
	msgbox.setButtonText(QMessageBox::Ok, "reboot");
	msgbox.exec();

	reboot(0x1234567);
	close();
}
