#include "disk_checker.h"
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace sysck;

static void partition_read_extra_state(struct partition *pt)
{
	// check if has devfile
	char buffer[256];
	sprintf(buffer, "/dev/%s", pt->name);
	if (access(buffer, F_OK) == -1) return;
	pt->has_devfile = true;

	// check if is available & read low level state of disk
	int fd = open(buffer, O_RDWR);
	if (fd == -1) return;

	if (ioctl(fd, BLKROGET, &pt->readonly) == -1) return;
	if (ioctl(fd, BLKGETSIZE, &pt->size) == -1) return;
	if (ioctl(fd, BLKGETSIZE64, &pt->size64) == -1) return;
	if (ioctl(fd, BLKSSZGET, &pt->sector_size) == -1) return;
	if (ioctl(fd, BLKBSZGET, &pt->block_size) == -1) return;
	pt->is_available = true;

	// check if is mounted
	// TODO
	pt->is_mounted = true;
}

disk_checker::disk_checker(std::string name)
	: device_name(name)
{
	read_current_partitions();
}

disk_checker::~disk_checker()
{}

void disk_checker::read_current_partitions()
{
	std::regex pattern(device_name);
	std::ifstream ifpart("/proc/partitions");

	partitions.clear();

	while (!ifpart.eof()) {
		char buffer[256];
		ifpart.getline(buffer, 256);
		if (regex_search(buffer, pattern)) {
			struct partition part = {};
			sscanf(buffer, " %d %d %d %31s", &part.major, &part.minor, &part.blocks, part.name);
			partition_read_extra_state(&part);
			partitions.push_back(part);
		}
	}

	ifpart.close();
}

const std::vector<struct partition>& disk_checker::current_partitions() const
{
	return partitions;
}

bool disk_checker::is_exist() const
{
	if (partitions.size() == 0)
		return false;
	else
		return true;
}

bool disk_checker::is_parted() const
{
	if (partitions.size() > 1)
		return true;
	else
		return false;
}

int disk_checker::check_partition(struct partition &pt) const
{
	if (device_name.compare(pt.name) && pt.has_devfile && pt.is_available) {
		std::stringstream cmd;
		cmd << "fsck -y /dev/" << pt.name;
		return system(cmd.str().c_str());
	}

	return -1;
}

int disk_checker::format_partition(struct partition &pt, format_type type) const
{
	std::stringstream cmd;

	switch (type) {
	case FORMAT_EXT2:
		cmd << "mkfs -t ext2 /dev/" << pt.name;
		break;

	case FORMAT_FAT32:
		cmd << "mkfs -t vfat /dev/" << pt.name;
		break;

	default:
		return -1;
	}

	return system(cmd.str().c_str());
}


void disk_checker::print_partition_table() const
{
	for (auto &item : partitions) {
		std::cout << item.major << " ";
		std::cout << item.minor << " ";
		std::cout << item.blocks << " ";
		std::cout << item.name << " ";
		std::cout << item.has_devfile << " ";
		std::cout << item.is_available << " ";
		std::cout << item.is_mounted << " ";
		std::cout << item.readonly << " ";
		std::cout << item.size << " ";
		std::cout << item.size64 << " ";
		std::cout << item.sector_size << " ";
		std::cout << item.block_size << " ";
		std::cout << std::endl;
	}
}

int disk_checker::reread_partition_table()
{
	return 0;
}

int disk_checker::rebuild_partition_table()
{
	return 0;
}
