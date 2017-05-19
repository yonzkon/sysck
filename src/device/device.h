#ifndef DEVICE_H
#define DEVICE_H

#include <string>

struct device {
	int major;
	int minor;
	std::string name;
	std::string devfile;
	std::string sysdir;
	bool is_available;
};

#endif
