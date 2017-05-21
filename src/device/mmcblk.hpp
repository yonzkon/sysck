#ifndef MMCBLK_HPP
#define MMCBLK_HPP

#include "partition.hpp"
#include "storage_disk.hpp"

namespace sysck {

typedef storage_disk<
	partition,
	detect_partition_with_devfile,
	recover_partition_by_utils>
mmcblk;

}

#endif
