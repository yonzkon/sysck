#include "config.h"
#include "device/mmcblk.hpp"
#include "device/mbr.h"
#include <iostream>
#include <memory>

using namespace std;

shared_ptr<sysck::config> conf;

static void print_partitions(typename sysck::mmcblk::partition_container &partitions)
{
	for (auto &item : partitions) {
		std::cout << item.major << " ";
		std::cout << item.minor << " ";
		std::cout << item.name << " ";
		std::cout << item.devfile<< " ";
		std::cout << item.sysdir<< " ";
		std::cout << item.is_available << " ";

		std::cout << item.is_disk << " ";
		std::cout << item.is_mounted << " ";

		std::cout << item.blocks << " ";
		std::cout << item.readonly << " ";
		std::cout << item.size << " ";
		std::cout << item.size64 << " ";
		std::cout << item.sector_size << " ";
		std::cout << item.block_size << " ";
		std::cout << std::endl;
	}
}

static void make_sdcard_mbr(struct mbr *mbr, int total_sectors)
{
	memset(mbr, 0, sizeof(struct mbr));
	mbr->disk_signature = DISK_SIGNATURE_MAGIC;
	mbr->dpt[0].active = 0;
	fba_to_chs(&mbr->dpt[0].start_sector, MBR_RESERVED);
	mbr->dpt[0].fs_type = FS_LINUX;
	fba_to_chs(&mbr->dpt[0].end_sector, total_sectors - 1);
	mbr->dpt[0].first_lba = MBR_RESERVED;
	mbr->dpt[0].sectors = total_sectors - MBR_RESERVED;
	mbr->tag = MBR_TAG;
}

static int check_sdcard(const char *name)
{
	unique_ptr<sysck::mmcblk> sdcard(new sysck::mmcblk(name));
	auto partitions = sdcard->current_partitions();
	print_partitions(partitions);

	// check if disk is exist
	if (partitions.size() == 0) {
		cout << "[FATAL] " << name << " is not available." << endl;
		return -1;
	}

	// check if disk is parted
	if (partitions.size() == 1) {
		cout << "[WARNING] " << name << " is not partitioned." << endl;

		struct mbr mbr;
		make_sdcard_mbr(&mbr, partitions[0].size);
		if (sdcard->rebuild_table(name, &mbr) != 0) {
			cout << "[FATAL] " << "rebuild partition table " << name << " failed." << endl;
			return -1;
		}
		cout << "[INFO] rebuild partition table on " << name << " success." << endl;

		if (sdcard->reread_partition_table() != 0) {
			cout << "[FATAL] " << "reread partition table failed." << endl;
			return -1;
		}
		partitions = sdcard->current_partitions();

		if (partitions.size() == 1) {
			cout << "[FATAL] "
				 << name
				 << " is still not partitioned after rebuild partition table."
				 << endl;
			return -1;
		}

		partitions = sdcard->current_partitions();
		for (auto &item : partitions) {
			if (item.is_disk || !item.is_available)
				continue;
			if (sdcard->format(item, conf->format_type) != 0) {
				cout << "[FATAL] " << "format " << item.name << " failed." << endl;
				return -1;
			}
		}
	}

	// check if disk and its first partition is available
	if (!partitions[0].is_available || !partitions[1].is_available) {
		cout << "[FATAL] " << name << " is physical broken, please change the SD card." << endl;
		return -1;
	}

	// now we run fsck on every partition
	for (auto &item : partitions) {
		if (!item.is_disk && item.is_available && !item.is_mounted)
			sdcard->fsck(item);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	conf = make_shared<sysck::config>(argc, argv);

	for (auto &item : conf->disks)
		check_sdcard(item.c_str());

	return 0;
}
