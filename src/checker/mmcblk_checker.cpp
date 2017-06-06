#include "mmcblk_checker.h"
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
	, wait_is_block(true)
	, wait_result_is_continue(false)
{
}

mmcblk_checker::~mmcblk_checker()
{
	delete blk;
}

void mmcblk_checker::process_check()
{
	QString tagname = "[" + QString(name.c_str()) + "]";
	print_partitions();

	emit state_msg("checking if " + tagname + " exists", MSG_INFO);
	if (!is_exist()) {
		emit state_msg(tagname + " does not exist", MSG_FATAL);
		return;
	}

	emit state_msg("checking if " + tagname + " is parted", MSG_INFO);
	if (!is_parted()) {
		emit state_msg(tagname + " isn't parted", MSG_PAUSE);

		if (!wait_for_continue_or_exit())
			return;

		emit state_msg("do partition on " + tagname, MSG_INFO);
		if (do_part(format_type) == -1) {
			emit state_msg("do partition on" + tagname + " failed", MSG_FATAL);
			return;
		}
		print_partitions();
	}

	emit state_msg("checking if " + tagname + " is available", MSG_INFO);
	if (!is_available()) {
		emit state_msg(tagname + " isn't available", MSG_FATAL);
		return;
	}

	// TODO: check fsck_status after do_fsck
	emit state_msg("do fsck on " + tagname, MSG_INFO);
	do_fsck(fsck_timeout);
	for (auto &item : blk->current_partitions()) {
		if (item.is_disk)
			continue;

		if ((item.fsck_status | FSCK_OK) == 0) {
			emit state_msg(tagname + ": No errors", MSG_INFO);
			continue;
		}

		if (item.fsck_status & FSCK_NONDESTRUCT)
			emit state_msg(tagname + ": File system errors corrected", MSG_INFO);

		if (item.fsck_status & FSCK_DESTRUCT) {
			emit state_msg(tagname  + ": System should be rebooted", MSG_INFO);
		}

		if (item.fsck_status & FSCK_UNCORRECTED) {

		}
	}

	emit state_msg("check finished", MSG_INFO);
}

void mmcblk_checker::continue_or_exit(bool continue_or_exit)
{
	wait_result_is_continue = continue_or_exit;
	wait_is_block = false;
}

bool mmcblk_checker::wait_for_continue_or_exit()
{
	while (wait_is_block)
		sleep(1);

	wait_is_block = true;
	return wait_result_is_continue;
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

		for (auto item : partitions) {
			if (item.is_disk || !item.is_available)
				continue;
			if (blk->format(item, format_type) != 0) {
				cout << "[FATAL] " << "format " << item.name << " failed." << endl;
				return -1;
			}
		}
	}

	return 0;
}

void mmcblk_checker::do_fsck(int timeout)
{
	for (auto item : blk->current_partitions()) {
		if (!item.is_disk && item.is_available && !item.is_mounted)
			blk->fsck(item, timeout);
	}
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
