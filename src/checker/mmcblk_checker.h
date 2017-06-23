#ifndef MMCBLK_CHECKER_H
#define MMCBLK_CHECKER_H

#include "disk/disk.h"
#include "disk/partition.h"
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
	enum check_stage {
		STAGE_EXIST,
		STAGE_PART,
		STAGE_AVAI,
		STAGE_FSCK,
	};

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
	void carryon(bool part_permission);

private:
	bool is_exist();
	bool is_parted();
	bool is_available();

	int do_part(std::string format_type);

	void print_partitions();

private:
	std::string name;
	std::string format_type;
	int fsck_timeout;
	mmcblk* blk;
	bool part_permission;
	check_stage stage;
};

}

#endif
