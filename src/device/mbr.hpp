#ifndef MBR_H
#define MBR_H

#ifdef __cplusplus
extern "C" {
#endif

// cylinder start from 1 to 1024
// head start from 1 to 255
// sector start from 1 to 63
#define CYLINDER_SIZE (255 * 63)

struct chs {
	char start_head;
	short start_sector:6;
	short start_cylinder:10;
} __attribute__ ((aligned(1)));

struct dpt {
	char active; /* 00 -> inactive; 80 -> active */
	struct chs start_sector;
	char fs_type;
	struct chs end_sector;
	int first_lba;
	int sectors;
} __attribute__ ((aligned(1)));

struct mbr {
	char code[440];
	int disk_signature;
	int unused;
	struct dpt dpt[4];
	short tag;
} __attribute__ ((aligned(1)));

extern inline
void fba_to_chs(struct chs *chs, int fba)
{
	chs->start_head = (fba % CYLINDER_SIZE) / 63;
	chs->start_sector = (fba % CYLINDER_SIZE) % 63 + 1;
	chs->start_cylinder = fba / CYLINDER_SIZE;
}

#ifdef __cplusplus
}
#endif

#endif
