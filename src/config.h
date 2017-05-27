#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

namespace sysck {

class config {
public:
	config(int argc, char *argv[]);

public:
	int config_from_arg(int argc, char *argv[]);
	void usage(int show_detail, char *argv0) const;

public:
	std::string format_type;
	std::vector<std::string> disks;
	int fsck_timeout;
};

}

#endif
