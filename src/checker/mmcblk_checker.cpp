#include "mmcblk_checker.h"
#include "msg_type.h"
#include "disk/mbr.h"
#include "disk/fsck.h"
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

mmcblk_checker::mmcblk_checker(std::string name,
							   std::string format_type,
							   int fsck_timeout)
	: name(name)
	, format_type(format_type)
	, fsck_timeout(fsck_timeout)
	, blk(new mmcblk(name))
	, part_permission(false)
	, stage(STAGE_EXIT)
{
}

mmcblk_checker::~mmcblk_checker()
{
	delete blk;
}

void mmcblk_checker::execute()
{
	QString tagname = "[" + QString(name.c_str()) + "]";
	print_partitions();

	// FIXME: ugly implemention which drives me crazy...
	if (stage == STAGE_EXIT) {
		emit state_msg("checking if " + tagname + " exists", MSG_INFO);
		if (!is_exist()) {
			emit state_msg(tagname + " does not exist", MSG_REBOOT);
			return;
		}
		stage = STAGE_PART;
	}

	if (stage == STAGE_PART) {
		emit state_msg("checking if " + tagname + " is parted", MSG_INFO);
		if (!is_parted()) {
			if (!part_permission) {
				emit state_msg(tagname + " isn't parted", MSG_PERMIT);
				return;
			}

			emit state_msg("do partition on " + tagname, MSG_INFO);
			if (do_part(format_type) == -1) {
				emit state_msg("do partition on" + tagname + " failed", MSG_REBOOT);
				return;
			}
			print_partitions();
		}
		stage = STAGE_AVAI;
	}

	if (stage == STAGE_AVAI) {
		emit state_msg("checking if " + tagname + " is available", MSG_INFO);
		if (!is_available()) {
			emit state_msg(tagname + " isn't available", MSG_REBOOT);
			return;
		}
		stage = STAGE_FSCK;
	}

	if (stage != STAGE_FSCK) return;
	emit state_msg("do fsck on " + tagname, MSG_INFO);
	blk->fsck_partitions(fsck_timeout);
	const mmcblk::partition_container &partitions = blk->current_partitions();
	for (size_t i = 0; i < partitions.size(); i++) {
		if (partitions[i].is_disk)
			continue;

		if (!partitions[i].is_fscked) {
			emit state_msg(tagname + ": Fsck timeout or failed", MSG_REBOOT);
			continue;
		}

		if ((partitions[i].fsck_status | FSCK_OK) == 0) {
			emit state_msg(tagname + ": No errors", MSG_INFO);
			continue;
		}

		if (partitions[i].fsck_status & FSCK_NONDESTRUCT) {
			emit state_msg(tagname + ": File system errors corrected", MSG_INFO);
			continue;
		}

		if (partitions[i].fsck_status & FSCK_DESTRUCT) {
			emit state_msg(tagname  + ": System should be rebooted", MSG_REBOOT);
			return;
		}

		if (partitions[i].fsck_status & FSCK_UNCORRECTED) {
			emit state_msg(tagname + ": File system errors left uncorrected", MSG_REBOOT);
			return;
		}

		if (partitions[i].fsck_status & FSCK_ERROR) {
			emit state_msg(tagname + ": Operational error", MSG_REBOOT);
			return;
		}

		if (partitions[i].fsck_status & FSCK_USAGE) {
			emit state_msg(tagname + ": Usage or syntax error", MSG_REBOOT);
			return;
		}

		if (partitions[i].fsck_status & FSCK_CANCELED) {
			emit state_msg(tagname + ": Fsck canceled by user request", MSG_REBOOT);
			return;
		}
	}

	emit state_msg("check finished", MSG_INFO);
	emit finished();
}

void mmcblk_checker::carryon(bool part_permission)
{
	this->part_permission = part_permission;
	execute();
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

	if (!is_parted() || !partitions[0].is_available || !partitions[1].is_available)
		return false;
	else
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
			if (partitions[i].is_disk || !partitions[i].is_available
				|| partitions[i].is_mounted)
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
