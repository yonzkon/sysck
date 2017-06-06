#ifndef MMCBLK_CHECKER_H
#define MMCBLK_CHECKER_H

#include "disk/disk.h"
#include "disk/partition.h"
#include "msg_level.h"
#include <string>
#include <QObject>
#include <QString>

namespace sysck {

typedef disk<
	partition,
	detect_partition_with_devfile,
	recover_partition_by_utils>
	mmcblk;

class mmcblk_checker : public QObject {
	Q_OBJECT
private:
	mmcblk_checker(mmcblk_checker &rhs);
	mmcblk_checker& operator=(mmcblk_checker &rhs);

public:
	mmcblk_checker(std::string name, std::string format_type, int fsck_timeout);
	virtual ~mmcblk_checker();

signals:
	void state_msg(QString msg, msg_level level);

public slots:
	void process_check();
	void continue_or_exit(bool continue_or_exit);

private:
	bool wait_for_continue_or_exit();
	bool is_exist();
	bool is_parted();
	bool is_available();

	int do_part(std::string format_type);
	void do_fsck(int timeout);

	void print_partitions();

private:
	std::string name;
	std::string format_type;
	int fsck_timeout;
	mmcblk* blk;
	bool wait_is_block;
	bool wait_result_is_continue;
};

}

#endif
