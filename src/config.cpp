#include "config.h"
#include <cstring>
#include <iostream>

static const char options[] =
"   -d                   specify disks to check\n"
"   -t                   specify format type while mkfs [default: vfat]\n"
"   -h                   show this message\n"
;

sysck::config::config(int argc, char *argv[])
	: format_type("fat32")
{
	config_from_arg(argc, argv);
}

int sysck::config::config_from_arg(int argc, char *argv[])
{
	//int index_args = 0;

	for(int i = 1; i < argc; i++) {
		char *z = argv[i];

		//// dbfiles: test_db first, result_db second
		//if (z[0] != '-') {
		//	if (index_args == 0)
		//		snprintf(conf->test_db, sizeof(conf->test_db), "%s", z);
		//	else if (index_args == 1)
		//		snprintf(conf->result_db, sizeof(conf->result_db), "%s", z);
		//	else {
		//		dzlog_warn("unknown argument: %s", z);
		//		dzlog_warn("use -h for a list of options.");
		//	}

		//	index_args++;
		//	continue;
		//}

		// options which start with '-'
		if(strcmp(z, "-h") == 0) {
			usage(1, argv[0]);
		} else if (strcmp(z, "-t") == 0) {
			i++;
			if (i >= argc || argv[i][0] == '-') {
				std::cout << "[WARNING] missing argument for option " << z << std::endl;
				return 1;
			}
			this->format_type = argv[i];
		} else if (strcmp(z, "-d") == 0) {
			i++;
			if (i >= argc || argv[i][0] == '-') {
				std::cout << "[WARNING] missing argument for option " << z << std::endl;
				return 1;
			}
			this->disks.push_back(std::string(argv[i]));
		} else {
			std::cout << "[WARNING] unknown option " << z
					  << ", use -h for a list of options."
					  << std::endl;
			return 1;
		}
	}

	return 0;
}

void sysck::config::usage(int show_detail, char *argv0) const
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n", argv0);
	if (show_detail) {
		fprintf(stderr, "OPTIONS include:\n%s", options);
	} else {
		fprintf(stderr, "Use the -h option for additional information\n");
	}
	exit(1);
}
