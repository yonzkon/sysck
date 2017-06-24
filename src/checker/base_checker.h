#ifndef BASE_CHECKER_H
#define BASE_CHECKER_H

#include "check_unit.h"
#include <vector>
#include <QObject>

namespace sysck {

template <class T>
class base_checker : public QObject {
public:
	typedef check_unit<T> unit_type;
	typedef std::vector<unit_type> unit_collection_type;

private:
	enum {
		ST_CONTINUE,
		ST_BREAK,
	};

public:
	base_checker() : check_loop_status(ST_CONTINUE) {}
	virtual ~base_checker() {}

protected:
	void run_check()
	{
		check_loop_status = ST_CONTINUE;

		for (typename unit_collection_type::iterator iter = units.begin();
			 iter != units.end(); ++iter) {
			if (!iter->has_completed)
				(dynamic_cast<T*>(this)->*(iter->func))(&(*iter));

			if (check_loop_status == ST_BREAK)
				break;
		}
	}

	void stop_check()
	{
		check_loop_status = ST_BREAK;
	}

protected:
	unit_collection_type units;
private:
	int check_loop_status;
};

}

#endif
