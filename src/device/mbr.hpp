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
	unsigned char start_head;
	unsigned short start_sector:6;
	unsigned short start_cylinder:10;
} __attribute__ ((packed));

struct dpt {
	unsigned char active; /* 00 -> inactive; 80 -> active */
	struct chs start_sector;
	unsigned char fs_type;
	struct chs end_sector;
	unsigned int first_lba;
	unsigned int sectors;
} __attribute__ ((packed));

struct mbr {
	unsigned char code[440];
	unsigned int disk_signature;
	unsigned short unused;
	struct dpt dpt[4];
	unsigned short tag;
} __attribute__ ((packed));

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
