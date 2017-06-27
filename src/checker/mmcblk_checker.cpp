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
	unit_type unit;
	unit.has_passed = false;
	unit.has_completed = false;

	unit.name = "check_exist";
	unit.func = &mmcblk_checker::check_exist;
	units.push_back(unit);

	unit.name = "check_readonly";
	unit.func = &mmcblk_checker::check_readonly;
	units.push_back(unit);

	unit.name = "check_part";
	unit.func = &mmcblk_checker::check_part;
	units.push_back(unit);

	unit.name = "check_volume";
	unit.func = &mmcblk_checker::check_volume;
	units.push_back(unit);

	unit.name = "check_available";
	unit.func = &mmcblk_checker::check_available;
	units.push_back(unit);

	unit.name = "check_fsck";
	unit.func = &mmcblk_checker::check_fsck;
	units.push_back(unit);

	unit.name = "check_finished";
	unit.func = &mmcblk_checker::check_finished;
	units.push_back(unit);
}

mmcblk_checker::~mmcblk_checker()
{
	delete blk;
}

void mmcblk_checker::execute()
{
	run_check();
}

void mmcblk_checker::rebuild_partition()
{
	emit state_msg("do partition on " + tagname, MSG_INFO);
	if (do_part(format_type) == -1) {
		emit state_msg("do partition on " + tagname + " failed", MSG_REBOOT);
		stop_check();
		return;
	
	print_partitions();

	// FIXME: restart checking, and it's still a ugly implementation.
	run_check();
}

void mmcblk_checker::check_exist(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " exists", MSG_INFO);

	if (!is_exist()) {
		emit state_msg(tagname + " does not exist", MSG_REBOOT);
		unit->has_passed = false;
		stop_check();
	} else {
		unit->has_passed = true;
	}

	unit->has_completed = true;
}

void mmcblk_checker::check_readonly(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " is readonly", MSG_INFO);

	if (is_readonly()) {
		emit state_msg(tagname + " is readonly", MSG_REBOOT);
		unit->has_passed = false;
		stop_check();
	} else {
		unit->has_passed = true;
	}

	unit->has_completed = true;
}

void mmcblk_checker::check_part(mmcblk_check_unit *unit)
{
	emit state_msg("checking if " + tagname + " is parted", MSG_INFO);

	if (is_parted()) {
		unit->has_passed = true;
		unit->has_completed = true;
	} else {
		emit state_msg(tagname + " isn't parted", MSG_REPART);
		unit->has_passed = false;
		unit->has_completed = false;
		stop_check();
	}

	print_partitions();
}

void mmcblk_checker::check_volume(mmcblk_check_unit *unit)
{
	mmcblk::partition_collection_type partitions = blk->current_partitions();
	// The reason why we use size rather than size64 is that
	// size64 maybe incorrect in arm with kernel version 2.6.30
	double first_partition_size = (double)partitions[1].size / 1024 / 1024 / 2;
	if (first_partition_size < 6) {
		emit state_msg(tagname + "'s first partition is "
					   + QString("%1").arg(first_partition_size)
					   + "G, not meet the lower limit 6G", MSG_REPART);
		unit->has_passed = false;
		unit->has_completed = false;
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
		stop_check();
	} else {
		unit->has_passed = true;
	}

	unit->has_completed = true;
}

void mmcblk_checker::check_fsck(mmcblk_check_unit *unit)
{
	emit state_msg("do fsck on " + tagname, MSG_INFO);
	blk->fsck_partitions(fsck_timeout);
	const mmcblk::partition_collection_type &partitions = blk->current_partitions();
	for (size_t i = 0; i < partitions.size(); i++) {
		if (partitions[i].is_disk)
			continue;

		if (!partitions[i].is_fscked) {
			emit state_msg(tagname + ": fsck timeout or failed", MSG_REPART);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
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
			unit->has_passed = true;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (partitions[i].fsck_status & FSCK_UNCORRECTED) {
			emit state_msg(tagname + ": File system errors left uncorrected", MSG_REPART);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (partitions[i].fsck_status & FSCK_ERROR) {
			emit state_msg(tagname + ": Operational error", MSG_REPART);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (partitions[i].fsck_status & FSCK_USAGE) {
			emit state_msg(tagname + ": Usage or syntax error", MSG_REBOOT);
			unit->has_passed = false;
			unit->has_completed = true;
			stop_check();
			return;
		}

		if (partitions[i].fsck_status & FSCK_CANCELED) {
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

bool mmcblk_checker::is_readonly()
{
	if (blk->current_partitions()[0].readonly == 0)
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
	const mmcblk::partition_collection_type &partitions = blk->current_partitions();

	if (!is_parted() || !partitions[0].is_available || !partitions[1].is_available)
		return false;
	else
		return true;
}

int mmcblk_checker::do_part(std::string format_type)
{
	const mmcblk::partition_collection_type &partitions = blk->current_partitions();

	if (partitions.size() == 0)
		return -1;

	struct mbr mbr;
	memset(&mbr, 0, sizeof(struct mbr));
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

	return 0;
}

void mmcblk_checker::print_partitions()
{
	const mmcblk::partition_collection_type &partitions = blk->current_partitions();

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
