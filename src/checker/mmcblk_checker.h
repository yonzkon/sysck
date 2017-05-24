#ifndef MMCBLK_CHECKER_H
#define MMCBLK_CHECKER_H

#include "disk/disk.h"
#include "disk/partition.h"
#include <string>

namespace sysck {

typedef disk<
	partition,
	detect_partition_with_devfile,
	recover_partition_by_utils>
	mmcblk;

class mmcblk_checker {
private:
	mmcblk_checker(mmcblk_checker &rhs);
	mmcblk_checker& operator=(mmcblk_checker &rhs);

public:
	mmcblk_checker(std::string name);
	virtual ~mmcblk_checker();

public:
	int check_exist();
	int check_parted(std::string format_type);
	int check_available();
	int check_fsck();
	void print_partitions();

private:
	std::string name;
	mmcblk* blk;
};

}

#endif
