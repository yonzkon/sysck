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

void MainWindow::on_state_msg(QString msg, sysck::msg_level level)
{
	switch (level) {
	case sysck::MSG_INFO:
		ui->textBrowser->append(msg);
		break;

	case sysck::MSG_ERROR: {
		QMessageBox msgbox(QMessageBox::Question, NULL, msg + "\npress No to reboot the system",
						   QMessageBox::Yes | QMessageBox::No);

		if (msgbox.exec() == QMessageBox::Yes) {
			emit check_return(true);
		} else {
			emit check_return(false);
			reboot(0x1234567);
			close();
		}
		break;
	}

	case sysck::MSG_FATAL: {
		QMessageBox msgbox;
		msgbox.setText(msg + "\npress OK to reboot the system");
		msgbox.exec();

		reboot(0x1234567);
		close();
	}
	}
}
