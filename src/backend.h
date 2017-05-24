#ifndef BACKEND_H
#define BACKEND_H

#include <QThread>
#include "config.h"

class backend : public QThread
{
	Q_OBJECT
public:
	explicit backend(int argc, char *argv[], QObject *parent = 0);

public:
	void run();
	bool wait_check_result();

signals:
	void check_state(QString state);
	void check_error(QString errmsg);
	void check_fatal(QString errmsg);
	void check_finish();

public slots:
	void on_check_return(bool);

private:
	sysck::config *conf;
	volatile bool blocked;
	bool check_result;
};

#endif // BACKEND_H
