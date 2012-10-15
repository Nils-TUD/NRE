#include <iostream>
#include <stdint.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* offset of partition-table in MBR */
#define PART_TABLE_OFFSET	0x1BE

/* a partition on the disk */
struct DiskPart {
	/* Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active") */
	uint8_t bootable;
	/* start: Cylinder, Head, Sector */
	uint8_t startHead;
	uint16_t startSector : 6,
		startCylinder: 10;
	uint8_t systemId;
	/* end: Cylinder, Head, Sector */
	uint8_t endHead;
	uint16_t endSector : 6,
		endCylinder : 10;
	/* Relative Sector (to start of partition -- also equals the partition's starting LBA value) */
	uint32_t start;
	/* Total Sectors in partition */
	uint32_t size;
} __attribute__((packed));

static void print_parts(char *mbr) {
	DiskPart *src = reinterpret_cast<DiskPart*>(mbr + PART_TABLE_OFFSET);
	for(size_t i = 0; src->size > 0; i++) {
		std::cout << "Partition " << i << ":\n";
		std::cout << "  bootable=" << (src->bootable & 0x80 ? "yes" : "no") << "\n";
		std::cout << "  startHead=" << (unsigned)src->startHead << "\n";
		std::cout << "  startSector=" << src->startSector << "\n";
		std::cout << "  startCylinder=" << src->startCylinder << "\n";
		std::cout << "  endHead=" << (unsigned)src->endHead << "\n";
		std::cout << "  endSector=" << src->endSector << "\n";
		std::cout << "  endCylinder=" << src->endCylinder << "\n";
		std::cout << "  start=" << src->start << "\n";
		std::cout << "  size=" << src->size << "\n";
		src++;
	}
}

static char buffer[512];

int main(int argc,char **argv) {
	if(argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <disk>" << std::endl;
		return 1;
	}
	
	int disk = open(argv[1],O_RDONLY);
	if(disk < 0)
		perror("open");
	if(read(disk,buffer,512) != 512)
		perror("read");
	print_parts(buffer);
	close(disk);
	return 0;
}

