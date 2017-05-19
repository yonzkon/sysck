#ifndef MBR_SECTOR_HPP
#define MBR_SECTOR_HPP

struct dpt {
	char flag;
	char phy_sec_start[3];
	char type;
	char phy_sec_end[3];
	int logic_sec;
	int sec_cnt;
};

struct mbr_sector {
	char code[446];
	struct dpt dpt[4];
	short tag;
};

inline void logic2physic(char *physic_sec, int logic_sec)
{
	physic_sec[0] = (logic_sec % 16065) / 63;
	physic_sec[1] = ((logic_sec % 16065) % 63 + 1) & 0x3F;
	physic_sec[1] |= ((logic_sec / 16065) & 0x300) >> 2;
	physic_sec[2] = (logic_sec / 16065) & 0xFF;
}

#endif
