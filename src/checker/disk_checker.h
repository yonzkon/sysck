#ifndef DISK_CHECKER_H
#define DISK_CHECKER_H

#include <string>
#include <vector>

namespace sysck {

struct partition {
	int major;
	int minor;
	int blocks;
	char name[32];

	bool has_devfile;
	bool is_available;
	bool is_mounted;

	int readonly; /* 0 = read-write */
	long size;
	long size64;
	int sector_size;
	int block_size;
};

class disk_checker {
public:
	enum format_type {
		FORMAT_FAT32,
		FORMAT_EXT2,
	};

private:
	disk_checker(disk_checker &rhs);
	disk_checker& operator=(disk_checker &rhs);

public:
	disk_checker(std::string name);
	virtual ~disk_checker();

private:
	void read_current_partitions();

public:
	const std::vector<struct partition>& current_partitions() const;

	bool is_exist() const;
	bool is_parted() const;

	int check_partition(struct partition &pt) const;
	int format_partition(struct partition &pt, format_type type) const;

	void print_partition_table() const;
	int reread_partition_table();
	int rebuild_partition_table();

private:
	std::string device_name;
	std::vector<struct partition> partitions;
};

}

#endif
