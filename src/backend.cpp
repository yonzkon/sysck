#include "backend.h"
#include "checker/mmcblk_checker.h"

backend::backend(int argc, char *argv[], QObject *parent)
	: QThread(parent)
	, conf(new sysck::config(argc, argv))
	, blocked(true)
	, check_result(false)
{
}

void backend::run()
{
	for (auto &item : conf->disks) {
		sysck::mmcblk_checker checker(item.c_str());

		QString name = "disk [" + QString(item.c_str()) + "]";

		emit check_state("check if " + name + " exists");
		if (!checker.is_exist()) {
			emit check_state(name + " does not exist");
			emit check_finish();
			return;
		}

		emit check_state("check if " + name + " is parted");
		if (!checker.is_parted()) {
			emit check_state(name + " isn't parted");
			emit check_error(name + " has no partition table, shall we do partition on it?");

			if (!wait_check_result())
				return;

			emit check_state("do partition on " + name);
			if (checker.do_part(conf->format_type) == -1) {
				emit check_fatal("do partition on" + name + " failed");
				return;
			}
		}

		if (!checker.is_available()) {
			emit check_fatal(name + " isn't available");
			return;
		}

		emit check_state("do fsck on " + name);
		if (checker.do_fsck() != 0) {
			emit check_state("do check on " + name + " failed");
			emit check_error(name + " is damaged, should we re-partition on it?");

			if (!wait_check_result())
				return;

			int rc = system("mount /dev/mmcblk0p1 /mnt/sd");
			if (rc == 0)  {
				system("cp -a /mnt/sd/rtx/terminal.db* /home");
				system("umount /dev/mmcblk0p1");
			}

			emit check_state("do partition on " + name);
			if (checker.do_part(conf->format_type) == -1) {
				emit check_fatal("do partition on" + name + " failed");
				return;
			}

			if (!checker.is_available()) {
				emit check_fatal(name + " isn't available");
				return;
			}

			rc = system("mount /dev/mmcblk0p1 /mnt/sd");
			if (rc == 0)  {
				system("mkdir -p /mnt/sd/rtx");
				system("mv /home/terminal.db* /mnt/sd/rtx");
				system("umount /dev/mmcblk0p1");
			}
		}
		emit check_finish();
	}
}

bool backend::wait_check_result()
{
	while (blocked)
		usleep(500);

	blocked = true;

	return check_result;
}

void backend::on_check_return(bool status)
{
	blocked = false;
	check_result = status;
}
