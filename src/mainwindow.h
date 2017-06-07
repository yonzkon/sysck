#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

signals:
	void continue_check(bool);
	void stop_check();

public slots:
	void on_state_msg(QString msg, int type);

private:
	void handle_msg_info(QString msg);
	void handle_msg_permit(QString msg);
	void handle_msg_reboot(QString msg);

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
