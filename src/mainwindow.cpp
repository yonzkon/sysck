#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <unistd.h>
#include <sys/reboot.h>

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

void MainWindow::on_state_msg(QString msg, msg_level level)
{
	switch (level) {
	case MSG_INFO:
		handle_msg_info(msg);
		break;

	case MSG_ERROR:
		handle_msg_error(msg);
		break;

	case MSG_FATAL:
		handle_msg_fatal(msg);
		break;

	default:
		break;
	}
}

void MainWindow::handle_msg_info(QString msg)
{
	ui->textBrowser->append(msg);
}

void MainWindow::handle_msg_error(QString msg)
{
	QMessageBox msgbox(QMessageBox::Question, NULL, msg + "\npress No to reboot the system",
					   QMessageBox::Yes | QMessageBox::No);

	if (msgbox.exec() == QMessageBox::Yes) {
		emit check_return(true);
	} else {
		emit check_return(false);
		reboot(0x1234567);
		close();
	}
}

void MainWindow::handle_msg_fatal(QString msg)
{
	QMessageBox msgbox;
	msgbox.setText(msg + "\npress OK to reboot the system");
	msgbox.exec();

	reboot(0x1234567);
	close();
}
