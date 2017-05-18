#include <QApplication>
#include "ui_MainWindow.h"
#include "checker/disk_checker.h"
#include <iostream>
#include <memory>

using namespace std;

static int test_sdcard_check(const char *name)
{
	unique_ptr<sysck::disk_checker> checker(new sysck::disk_checker(name));
	auto partitions = checker->current_partitions();
	checker->print_partition_table();

	// check disk state
	if (!checker->is_exist()) {
		cout << "[FATAL] " << name << " is not exist." << endl;
		return -1;
	}

	if (!checker->is_parted()) {
		cout << "[WARNING] " << name << " is not partitioned." << endl;

		if (checker->rebuild_partition_table() == -1) {
			cout << "[FATAL] " << "rebuild_partition_table failed." << endl;
			return -1;
		}

		if (checker->reread_partition_table() == -1) {
			cout << "[FATAL] " << "reread_partition_table failed." << endl;
			return -1;
		}

		if (!checker->is_parted()) {
			cout << "[FATAL] "
				 << name
				 << " is still not partitioned after rebuild_partition_table."
				 << endl;
			return -1;
		}

		partitions = checker->current_partitions();
		for (auto &item : partitions) {
			// TODO: not format disk
			checker->format_partition(item, sysck::disk_checker::FORMAT_FAT32);
		}
	}

	// check partition state
	if (!partitions[0].has_devfile) {
		cout << "[WARNING] " << name << " doesn't find at /dev. do mknod ..." << endl;
		// TODO: mknod
		if (system("mknod")) {
			cout << "[FATAL] make node file of " << name << " at /dev failed." << endl;
			return -1;
		}
	}

	if (!partitions[0].is_available || !partitions[1].is_available) {
		cout << "[FATAL] " << name << " is physical broken, please change the SD card." << endl;
		return -1;
	}

	return checker->check_partition(partitions[1]);
}

int main(int argc, char *argv[])
{
	test_sdcard_check("sda");

	QApplication app(argc, argv);

	QMainWindow *window = new QMainWindow();
	Ui_MainWindow ui;// = new Ui_MainWindow();
	ui.setupUi(window);
	window->show();

	return app.exec();
}
