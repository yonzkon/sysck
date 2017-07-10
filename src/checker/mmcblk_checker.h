#ifndef MMCBLK_CHECKER_H
#define MMCBLK_CHECKER_H

#include "disk/disk.h"
#include "disk/partition.h"
#include "base_checker.h"
#include <string>
#include <QString>

namespace sysck {

typedef disk<
	partition,
	std::vector<partition>,
	detect_partition_with_devfile,
	recover_partition_by_utils>
	mmcblk;

class mmcblk_checker : public base_checker<mmcblk_checker> {
	Q_OBJECT
public:
	typedef check_unit<mmcblk_checker> mmcblk_check_unit;

private:
	mmcblk_checker(mmcblk_checker &rhs);
	mmcblk_checker& operator=(mmcblk_checker &rhs);

public:
	mmcblk_checker(std::string name, std::string format_type, int fsck_timeout);
	virtual ~mmcblk_checker();

signals:
	void state_msg(QString msg, int type);
	void finished();

public slots:
	void execute();
	void rebuild_partition();

private:
	void check_exist(mmcblk_check_unit *unit);
	void check_readonly(mmcblk_check_unit *unit);
	void check_part(mmcblk_check_unit *unit);
	void check_volume(mmcblk_check_unit *unit);
	void check_available(mmcblk_check_unit *unit);
	void check_fsck(mmcblk_check_unit *unit);
	void check_finished(mmcblk_check_unit *unit);

	bool is_exist();
	bool is_readonly();
	bool is_parted();
	bool is_available();

	int do_part(std::string format_type);
	void print_partitions();

private:
	std::string name;
	QString tagname;
	std::string format_type;
	int fsck_timeout;
	mmcblk* blk;
	bool part_permission;
};

}

#endif
