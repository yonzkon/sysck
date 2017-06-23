#ifndef PARTITION_H
#define PARTITION_H

#include "device.h"
#include "mbr.h"
#include <cstdlib>
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
	bool is_fscked;
	int fsck_status;

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
			pt.readonly = 1;
			return;
		}

		if (ioctl(fd, BLKROGET, &pt.readonly) == -1) {
			perror("ioctl");
			close(fd);
			return;
		}
		if (ioctl(fd, BLKGETSIZE, &pt.size) == -1) {
			perror("ioctl");
			close(fd);
			return;
		}
		if (ioctl(fd, BLKGETSIZE64, &pt.size64) == -1) {
			perror("ioctl");
			close(fd);
			return;
		}
		if (ioctl(fd, BLKSSZGET, &pt.sector_size) == -1) {
			perror("ioctl");
			close(fd);
			return;
		}
		if (ioctl(fd, BLKBSZGET, &pt.block_size) == -1) {
			perror("ioctl");
			close(fd);
			return;
		}
		close(fd);
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

#ifdef MBR_CLEAR
		std::stringstream cmd;
		cmd << "dd if=/dev/zero of=/dev/" << name << " bs=4096 count=2560";
		if (system(cmd.str().c_str()) != 0)
			return -1;
#endif

		int fd = open(devfile.c_str(), O_RDWR);
		if (fd == -1) {
			perror("open");
			return -1;
		}

		if (sizeof(struct mbr) != write(fd, (char*)mbr, sizeof(struct mbr))) {
			close(fd);
			return -1;
		}

		close(fd);
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
			close(fd);
			return -1;
		}

		close(fd);
		return 0;
	}

	// timeout: second
	static int fsck(T &pt, int timeout)
	{
		if (pt.is_disk || !pt.is_available || pt.is_mounted || pt.devfile.empty())
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

		int status, rc;
		int sleep_interval = 1;
		int times = 0, max_times = timeout / sleep_interval;
		while (times < max_times) {
			rc = waitpid(pid, &status, WNOHANG);
			if (rc == -1) {
				perror("waitpid");
				return -1;
			} else if (rc == 0) {
				sleep(sleep_interval);
				times++;
			} else if (rc == pid) {
				if (WIFSIGNALED(status))
					return -1;

				if (WIFEXITED(status)) {
					pt.is_fscked = true;
					pt.fsck_status =  WEXITSTATUS(status);
					return 0;
				}
			}
		}

		kill(pid, SIGTERM);
		waitpid(pid, &status, 0);
		return -1;
	}

	static int format(const T &pt, std::string type)
	{
		if (pt.is_disk || !pt.is_available || pt.is_mounted || pt.devfile.empty())
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
