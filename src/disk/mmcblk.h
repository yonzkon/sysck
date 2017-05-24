#ifndef MMCBLK_H
#define MMCBLK_H

#include "partition.h"
#include "disk.h"

namespace sysck {

typedef disk<
	partition,
	detect_partition_with_devfile,
	recover_partition_by_utils>
mmcblk;

}

#endif
