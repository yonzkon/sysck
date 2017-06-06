#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include "msg_level.h"

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
	void check_return(bool);

public slots:
	void on_state_msg(QString msg, msg_level level);

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
