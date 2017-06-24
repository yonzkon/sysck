#ifndef CHECK_UNIT_H
#define CHECK_UNIT_H

#include <string>

namespace sysck {

template <class T>
struct check_unit {
	std::string name;
	void (T::*func)(check_unit*);
	bool has_completed;
	bool has_passed;
};

}

#endif
