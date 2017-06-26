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
	, tagname("[" + QString(name.c_str()) + "]")
	, format_type(format_type)
	, fsck_timeout(fsck_timeout)
	, blk(new mmcblk(name))
	, part_permission(false)
{
	units.push_back({"check_exist", &mmcblk_checker::check_exist, false, false});
	units.push_back({"check_part", &mmcblk_checker::check_part, false, false});
	units.push_back({"check_volume", &mmcblk_checker::check_volume, false, false});
	units.push_back({"check_available", &mmcblk_checker::check_available, false, false});
	units.push_back({"check_fsck", &mmcblk_checker::check_fsck, false, false});
	units.push_back({"check_finished", &mmcblk_checker::check_finished, false, false});
}

mmcblk_checker::~mmcblk_checker()
{
	delete blk;
}

void mmcblk_checker::execute()
{
	run_check();
}

void mmcblk_checker::carryon(bool part_permission)
{
	this->part_permission = part_permission;
	execute();
}

void mmcblk_checker::check_exist(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " exists", MSG_INFO);

	if (!is_exist()) {
		emit state_msg(tagname + " does not exist", MSG_REBOOT);
		unit->has_passed = false;
		unit->has_completed = true;
		stop_check();
	} else {
		unit->has_passed = true;
		unit->has_completed = true;
	}
}

void mmcblk_checker::check_part(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " is parted", MSG_INFO);

	if (is_parted()) {
		unit->has_passed = true;
		unit->has_completed = true;
		return;
	}

	if (!part_permission) {
		emit state_msg(tagname + " isn't parted", MSG_PERMIT);
		unit->has_passed = false;
		unit->has_completed = false;
		stop_check();
		return;
	}

	emit state_msg("do partition on " + tagname, MSG_INFO);
	if (do_part(format_type) == -1) {
		emit state_msg("do partition on" + tagname + " failed", MSG_REBOOT);
		unit->has_passed = false;
		unit->has_completed = true;
		stop_check();
		return;
	}
	print_partitions();

	unit->has_completed = true;
}

void mmcblk_checker::check_volume(mmcblk_check_unit *unit)
{
	auto partitions = blk->current_partitions();
	// The reason why we use size rather than size64 is that
	// size64 maybe incorrect in arm with kernel version 2.6.30
	double first_partition_size = (double)partitions[1].size / 1024 / 1024 / 2;
	if (first_partition_size < 6) {
		emit state_msg(tagname + "'s first partition is "
					   + QString("%1").arg(first_partition_size)
					   + "G, not meet the lower limit 6G", MSG_REBOOT);
		unit->has_passed = false;
		unit->has_completed = true;
		stop_check();
	} else {
		unit->has_passed = true;
		unit->has_completed = true;
	}
}

void mmcblk_checker::check_available(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " is available", MSG_INFO);

	if (!is_available()) {
		emit state_msg(tagname + " isn't available(readonly or can't readwrite)", MSG_REBOOT);
		unit->has_passed = false;
		unit->has_completed = true;
		stop_check();
	}
}

void mmcblk_checker::check_fsck(mmcblk_check_unit *unit)
{
	emit state_msg("do fsck on " + tagname, MSG_INFO);
	blk->fsck_partitions(fsck_timeout);
	for (auto &item : blk->current_partitions()) {
		if (item.is_disk)
			continue;

		if (!item.is_fscked) {
			emit state_msg(tagname + ": fsck timeout or failed", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if ((item.fsck_status | FSCK_OK) == 0) {
			emit state_msg(tagname + ": No errors", MSG_INFO);
			continue;
		}

		if (item.fsck_status & FSCK_NONDESTRUCT) {
			emit state_msg(tagname + ": File system errors corrected", MSG_INFO);
			continue;
		}

		if (item.fsck_status & FSCK_DESTRUCT) {
			emit state_msg(tagname  + ": System should be rebooted", MSG_REBOOT);
			unit->has_passed = true;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (item.fsck_status & FSCK_UNCORRECTED) {
			emit state_msg(tagname + ": File system errors left uncorrected", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (item.fsck_status & FSCK_ERROR) {
			emit state_msg(tagname + ": Operational error", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (item.fsck_status & FSCK_USAGE) {
			emit state_msg(tagname + ": Usage or syntax error", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (item.fsck_status & FSCK_CANCELED) {
			emit state_msg(tagname + ": Fsck canceled by user request", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}
	}

	unit->has_passed = true;
	unit->has_completed = true;
}

void mmcblk_checker::check_finished(mmcblk_check_unit *unit)
{
	unit->has_passed = true;
	unit->has_completed = true;
	stop_check();

	emit state_msg("check finished", MSG_INFO);
	emit finished();
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
	auto &partitions = blk->current_partitions();

	if (!is_parted() || !partitions[0].is_available || !partitions[1].is_available)
		return false;
	else
		return true;
}

int mmcblk_checker::do_part(std::string format_type)
{
	auto &partitions = blk->current_partitions();

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

		sleep(1); // wait os to make device node file
		for (auto &item : partitions) {
			if (item.is_disk || !item.is_available || item.is_mounted)
				continue;
			if (blk->format(item, format_type) != 0) {
				cout << "[FATAL] " << "format " << item.name << " failed." << endl;
				return -1;
			}
		}
	}

	return 0;
}

void mmcblk_checker::print_partitions()
{
	for (auto &item : blk->current_partitions()) {
		cout << item.major << " ";
		cout << item.minor << " ";
		cout << item.name << " ";
		cout << item.devfile<< " ";
		cout << item.sysdir<< " ";
		cout << item.is_available << " ";

		cout << item.is_disk << " ";
		cout << item.is_mounted << " ";

		cout << item.blocks << " ";
		cout << item.readonly << " ";
		cout << item.size << " ";
		cout << item.size64 << " ";
		cout << item.sector_size << " ";
		cout << item.block_size << " ";
		cout << endl;
	}
}
