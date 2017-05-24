#include "config.h"
#include "checker/mmcblk_checker.h"
#include <memory>

using namespace std;

shared_ptr<sysck::config> conf;

int main(int argc, char *argv[])
{
	conf = make_shared<sysck::config>(argc, argv);

	for (auto &item : conf->disks) {
		sysck::mmcblk_checker checker(item.c_str());
		checker.check_exist();
		checker.check_parted(conf->format_type);
		checker.check_available();
		checker.check_fsck();
	}

	return 0;
}
