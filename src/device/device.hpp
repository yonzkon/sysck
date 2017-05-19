#ifndef DEVICE_HPP
#define DEVICE_HPP

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
