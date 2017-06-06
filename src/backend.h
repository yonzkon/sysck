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

signals:
	void start_process_check();

private:
	sysck::config *conf;
};

#endif // BACKEND_H
