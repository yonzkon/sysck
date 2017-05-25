#ifndef PARTITION_H
#define PARTITION_H

#include "device.h"
#include "mbr.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>
#include <unistd.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>

namespace sysck {

struct partition : public device {
	bool is_disk;
	bool is_mounted;

	int blocks;   /* not used */
	int readonly; /* 0 = read-write */
	long size;
	size_t size64;
	int sector_size;
	int block_size;
};

template<class T>
struct detect_partition_with_devfile {
	typedef std::vector<T> partition_container;

	static void detect(std::string &name, partition_container &partitions)
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
					  makedev(pt.major, pt.minor)) == -1) {
				perror("mknod");
				return;
			}
		}
		pt.devfile = "/dev/" + pt.name;

		// check sysdir
		// TODO

		// check if is available & read low level state of disk
		int fd = open(pt.devfile.c_str(), O_RDWR);
		if (fd == -1) {
			perror("open");
			return;
		}

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

	static int rebuild_table(std::string name, struct mbr *mbr)
	{
		std::string devfile = "/dev/" + name;
		if (access(devfile.c_str(), F_OK) == -1) {
			perror("access");
			return -1;
		}

		int fd = open(devfile.c_str(), O_RDWR);
		if (fd == -1) {
			perror("open");
			return -1;
		}

		if (sizeof(struct mbr) != write(fd, (char*)mbr, sizeof(struct mbr)))
			return -1;
		else
			return 0;
	}

	static int reread_table(std::string name)
	{
		std::string devfile = "/dev/" + name;
		if (access(devfile.c_str(), F_OK) == -1) {
			perror("access");
			return -1;
		}

		int fd = open(devfile.c_str(), O_RDWR);
		if (fd == -1) {
			perror("open");
			return -1;
		}

		if (ioctl(fd, BLKRRPART, NULL) == -1) {
			perror("ioctl");
			return -1;
		}

		return 0;
	}

	static int fsck(T &pt)
	{
		if (!pt.is_available || pt.is_disk || pt.devfile.empty())
			return -1;

		int pid = fork();
		if (pid < 0) {
			perror("fork");
			return -1;
		}

		if (!pid) {
			close(0);//close(1);close(2);
			exit(execlp("fsck", "fsck", "-y", pt.devfile.c_str(), NULL));
		}

		// waitpid, to timeout in 5 minutes
		int status;
		int sleep_interval = 1;
		int times = 0, max_times = 60 * 5 / sleep_interval;
		while (times < max_times) {
			sleep(sleep_interval);
			if (waitpid(pid, &status, WNOHANG) == -1) {
				perror("waitpid");
				return -1;
			}

			if (WIFSIGNALED(status))
				return -1;

			if (WIFEXITED(status))
				return WEXITSTATUS(status);

			times++;
		}

		kill(pid, SIGTERM);
		waitpid(pid, &status, 0);
		return -1;
	}

	static int format(T &pt, std::string type)
	{
		if (!pt.is_available || pt.is_disk || pt.devfile.empty())
			return -1;

		// FIXME: mkfs may prompt 'Proceed anyway? (y,n)'
		// while the disk already contains a file system.
		// I'm not sure if 'echo y' will be suitable in any circumstance.
		std::stringstream cmd;
		cmd << "echo y| mkfs." << type << " " << pt.devfile;
		return system(cmd.str().c_str());
	}
};

}

#endif
