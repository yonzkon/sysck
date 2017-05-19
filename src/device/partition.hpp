#ifndef PARTITION_HPP
#define PARTITION_HPP

#include "device.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>

namespace sysck {

enum format_type {
	FORMAT_FAT32,
	FORMAT_EXT2,
};

struct partition : public device {
	bool is_disk;
	bool is_mounted;

	int blocks;   /* not used */
	int readonly; /* 0 = read-write */
	long size;
	long size64;
	int sector_size;
	int block_size;
};

template<class T>
struct detect_partition_with_devfile {
	typedef std::vector<T> Container;

	static void detect(std::string &name, Container &partitions)
	{
		std::regex pattern(name);
		std::ifstream ifpart("/proc/partitions");

		partitions.clear();

		while (!ifpart.eof()) {
			char buffer[256];
			ifpart.getline(buffer, 256);
			if (regex_search(buffer, pattern)) {
				T part = {};
				std::stringstream(buffer) >> part.major
										  >> part.minor
										  >> part.blocks
										  >> part.name;
				if (name.compare(part.name) == 0)
					part.is_disk = true;
				detect_extra(part);
				partitions.push_back(part);
			}
		}

		ifpart.close();
	}

	static void detect_extra(T &pt)
	{
		// check devfile
		std::string devfile = "/dev/" + pt.name;
		if (access(devfile.c_str(), F_OK) == -1) {
			if (mknod(pt.devfile.c_str(),
					  S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
					  makedev(pt.major, pt.minor)) == -1)
				return;
		}
		pt.devfile = "/dev/" + pt.name;

		// check sysdir
		// TODO

		// check if is available & read low level state of disk
		int fd = open(pt.devfile.c_str(), O_RDWR);
		if (fd == -1) return;

		if (ioctl(fd, BLKROGET, &pt.readonly) == -1) return;
		if (ioctl(fd, BLKGETSIZE, &pt.size) == -1) return;
		if (ioctl(fd, BLKGETSIZE64, &pt.size64) == -1) return;
		if (ioctl(fd, BLKSSZGET, &pt.sector_size) == -1) return;
		if (ioctl(fd, BLKBSZGET, &pt.block_size) == -1) return;
		pt.is_available = true;

		// check if is mounted
		if (pt.is_disk) return;
		std::regex pattern(pt.name);
		std::ifstream ifpart("/proc/self/mounts");
		while (!ifpart.eof()) {
			char buffer[256];
			ifpart.getline(buffer, 256);
			if (regex_search(buffer, pattern)) {
				pt.is_mounted = true;
				break;
			}
		}
		ifpart.close();
	}
};

template<class T>
struct recover_partition_by_utils {
	static int recover(T &pt);

	static int rebuild_table(std::string name)
	{
		name = "hello";
		return 0;
	}

	static int reread_table(std::string name)
	{
		std::string devfile = "/dev/" + name;
		if (access(devfile.c_str(), F_OK) == -1)
			return -1;

		int fd = open(devfile.c_str(), O_RDWR);
		if (fd == -1)
			return -1;

		int value;
		if (ioctl(fd, BLKRRPART, &value) == -1)
			return -1;

		return 0;
	}

	static int fsck(T &pt)
	{
		if (!pt.is_available || pt.is_disk || pt.devfile.empty())
			return -1;

		std::stringstream cmd;
		cmd << "fsck -y " << pt.devfile;
		return system(cmd.str().c_str());
	}

	static int format(T &pt, format_type type = FORMAT_FAT32)
	{
		if (!pt.is_available || pt.is_disk || pt.devfile.empty())
			return -1;

		std::stringstream cmd;

		switch (type) {
		case FORMAT_EXT2:
			cmd << "mkfs -t ext2 " << pt.devfile;
			break;

		case FORMAT_FAT32:
			cmd << "mkfs -t vfat " << pt.devfile;
			break;

		default:
			return -1;
		}

		return system(cmd.str().c_str());
	}
};

}

#endif
