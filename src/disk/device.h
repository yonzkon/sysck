#ifndef DEVICE_H
#define DEVICE_H

struct device {
	int major;
	int minor;
	char name[32];
	char devfile[256];
	char sysdir[256];
	bool is_available;
};

#endif
