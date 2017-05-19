#include "device/mmcblk.hpp"
#include "ui_MainWindow.h"
#include <QApplication>
#include <iostream>
#include <memory>

using namespace std;

static void print_partitions(typename sysck::mmcblk::container &partitions)
{
	for (auto &item : partitions) {
		std::cout << item.major << " ";
		std::cout << item.minor << " ";
		std::cout << item.name << " ";
		std::cout << item.devfile<< " ";
		std::cout << item.sysdir<< " ";
		std::cout << item.is_available << " ";

		std::cout << item.is_disk << " ";
		std::cout << item.is_mounted << " ";

		std::cout << item.blocks << " ";
		std::cout << item.readonly << " ";
		std::cout << item.size << " ";
		std::cout << item.size64 << " ";
		std::cout << item.sector_size << " ";
		std::cout << item.block_size << " ";
		std::cout << std::endl;
	}
}

static int check_sdcard(const char *name)
{
	unique_ptr<sysck::mmcblk> sdcard(new sysck::mmcblk(name));
	auto partitions = sdcard->current_partitions();
	print_partitions(partitions);

	// check disk state
	if (partitions.size() == 0) {
		cout << "[FATAL] " << name << " is not available." << endl;
		return -1;
	}

	if (partitions.size() == 1) {
		cout << "[WARNING] " << name << " is not partitioned." << endl;

		if (sdcard->rebuild_partition_table() == -1) {
			cout << "[FATAL] " << "rebuild_partition_table failed." << endl;
			return -1;
		}

		if (sdcard->reread_partition_table() == -1) {
			cout << "[FATAL] " << "reread_partition_table failed." << endl;
			return -1;
		}

		if (partitions.size() == 1) {
			cout << "[FATAL] "
				 << name
				 << " is still not partitioned after rebuild_partition_table."
				 << endl;
			return -1;
		}

		partitions = sdcard->current_partitions();
		for (auto &item : partitions) {
			// TODO: not format disk
			sdcard->format(item, sysck::FORMAT_FAT32);
		}
	}

	// check partition state
	if (partitions[0].devfile.empty()) {
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

	return sdcard->fsck(partitions[1]);
}

int main(int argc, char *argv[])
{
	check_sdcard("sda");
	return 0;

	QApplication app(argc, argv);

	QMainWindow *window = new QMainWindow();
	Ui_MainWindow ui;// = new Ui_MainWindow();
	ui.setupUi(window);
	window->show();

	return app.exec();
}
