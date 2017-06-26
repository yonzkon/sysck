#include "../src/disk/mbr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "syntax error.\n");
		return -1;
	}

	struct mbr mbr;
	memset(&mbr, 0, sizeof(struct mbr));
	make_mbr(&mbr, 2 *1024*1024* atoi(argv[2]));

	int fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	int nw = write(fd, (char*)&mbr, sizeof(struct mbr));
	if (nw != sizeof(struct mbr)) {
		fprintf(stderr, "[%s] %d bytes writed, less than 512.\n", argv[1], nw);
		close(fd);
		return -1;
	} else {
		printf("[%s] writed partition table successfully.\n", argv[1]);
		close(fd);
	}

	return 0;
}
