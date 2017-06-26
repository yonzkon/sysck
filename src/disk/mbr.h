#ifndef MBR_H
#define MBR_H

#ifdef __cplusplus
extern "C" {
#endif

#define FS_LINUX 0x83
#define FS_WIN95_FAT32_LBA 0xc
#define DISK_SIGNATURE_MAGIC 0xcf4746fe
#define MBR_TAG 0xaa55
#define MBR_RESERVED 2048

// cylinder start from 1 to 1024
// head start from 1 to 255
// sector start from 1 to 63
#define CYLINDER_SIZE (255 * 63)

struct chs {
	unsigned char head;
	unsigned short sector:6;
	unsigned short cylinder:10;
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

inline
void fba_to_chs(struct chs *chs, unsigned int fba)
{
	chs->head = (fba % CYLINDER_SIZE) / 63;
	chs->sector = (fba % CYLINDER_SIZE) % 63 + 1;
	chs->cylinder = fba / CYLINDER_SIZE;
}

#ifdef __cplusplus
}
#endif

#endif
