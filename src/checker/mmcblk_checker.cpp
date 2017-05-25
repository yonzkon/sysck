#include "mmcblk_checker.h"
#include "disk/mbr.h"
#include <iostream>

using namespace sysck;
using namespace std;

static void make_mbr(struct mbr *mbr, int total_sectors)
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

mmcblk_checker::mmcblk_checker(std::string name)
	: name(name)
	, blk(new mmcblk(name))
{
}

mmcblk_checker::~mmcblk_checker()
{
	delete blk;
}

bool mmcblk_checker::is_exist()
{
	if (blk->current_partitions().size() == 0)
		return false;
	else
		return true;
}

bool mmcblk_checker::is_parted()
{
	if (blk->current_partitions().size() < 2)
		return false;
	else
		return true;
}

bool mmcblk_checker::is_available()
{
	const mmcblk::partition_container &partitions = blk->current_partitions();

	if (partitions.size() < 2)
		return false;

	if (!partitions[0].is_available || !partitions[1].is_available)
		return false;

	return true;
}

int mmcblk_checker::do_part(std::string format_type)
{
	const mmcblk::partition_container &partitions = blk->current_partitions();

	if (partitions.size() == 0)
		return -1;

	if (partitions.size() == 1) {
		cout << "[WARNING] " << name << " is not partitioned." << endl;

		struct mbr mbr = {};
		make_mbr(&mbr, partitions[0].size);
		if (blk->rebuild_table(name, &mbr) != 0) {
			cout << "[FATAL] " << "rebuild partition table " << name << " failed." << endl;
			return -1;
		}
		cout << "[INFO] rebuild partition table on " << name << " success." << endl;

		if (blk->reread_partition_table() != 0) {
			cout << "[FATAL] " << "reread partition table failed." << endl;
			return -1;
		}

		if (partitions.size() == 1) {
			cout << "[FATAL] "
				 << name
				 << " is still not partitioned after rebuild partition table."
				 << endl;
			return -1;
		}

		for (size_t i = 0; i < partitions.size(); i++) {
			if (partitions[i].is_disk || !partitions[i].is_available)
				continue;
			partition p = partitions[i];
			if (blk->format(p, format_type) != 0) {
				cout << "[FATAL] " << "format " << partitions[i].name << " failed." << endl;
				return -1;
			}
		}
	}

	return 0;
}

int mmcblk_checker::do_fsck()
{
	const mmcblk::partition_container &partitions = blk->current_partitions();

	for (size_t i = 0; i < partitions.size(); i++) {
		if (!partitions[i].is_disk && partitions[i].is_available && !partitions[i].is_mounted) {
			int rc;
			partition p = partitions[i];
			if ((rc = blk->fsck(p)) != 0)
				return rc;
		}
	}

	return 0;
}

void mmcblk_checker::print_partitions()
{
	const mmcblk::partition_container &partitions = blk->current_partitions();

	for (size_t i = 0; i < partitions.size(); i++) {
		cout << partitions[i].major << " ";
		cout << partitions[i].minor << " ";
		cout << partitions[i].name << " ";
		cout << partitions[i].devfile<< " ";
		cout << partitions[i].sysdir<< " ";
		cout << partitions[i].is_available << " ";

		cout << partitions[i].is_disk << " ";
		cout << partitions[i].is_mounted << " ";

		cout << partitions[i].blocks << " ";
		cout << partitions[i].readonly << " ";
		cout << partitions[i].size << " ";
		cout << partitions[i].size64 << " ";
		cout << partitions[i].sector_size << " ";
		cout << partitions[i].block_size << " ";
		cout << endl;
	}
}
