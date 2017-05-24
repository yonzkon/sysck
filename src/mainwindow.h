#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
	void check_return(bool status);

public slots:
	void on_check_state(QString state);
	void on_check_error(QString errmsg);
	void on_check_fatal(QString errmsg);

private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
