#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "main.h"

unsigned char *partition = "/home/przb86/Desktop/bs-task2/build/run/disk.img";
unsigned int fsStart;

int main(int argc, char *argv[]){
	FILE *disk;
	
	// free blocks list (need to check that files are not in this list) 
	EOS32_daddr_t freeList[NICFREE];
	unsigned char partTable[SECTOR_SIZE];
	unsigned char blockBuffer[BLOCK_SIZE];
	unsigned char *ptptr;
	unsigned int fsSize;
	unsigned int partType;
	unsigned int freeListSize;
	int part;

	if(argc < 3){
		fprintf(stderr, "incorrect program: \"%s\" start\n", argv[1]);
		exit(INCORRECT_START);
	}

	if(access(argv[1], F_OK) == -1){
		fprintf(stderr, "file \"%s\" does not exist", argv[1]);
		exit(IMAGE_NOT_FOUND);
	}

	disk = fopen(argv[1], "rb");
  	if (disk == NULL) {
    	fprintf(stderr, "cannot open disk image file '%s', %d", partition, errno);
		exit(IO_ERROR);
  	}

	char *endptr;
	/* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10); // parsing number 
    if (*endptr != '\0' || part < 0 || part > 15) {
      	fprintf(stderr, "illegal partition number '%s'", argv[2]);
		exit(ILLEGAL_PARTITION);
    }
  	// setting file pointer to partition table
	fseek(disk, 1 * SECTOR_SIZE, SEEK_SET); 
    if (fread(partTable, 1, SECTOR_SIZE, disk) != SECTOR_SIZE) {
        fprintf(stderr, "cannot read partition table of disk '%s'", argv[1]);
	    exit(IO_ERROR);
    }
	// pointer to needed partition
	ptptr = partTable + part * 32;
    partType = get4Bytes(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
        fprintf(stderr, "partition %d of disk '%s' does not contain an EOS32 file system",
            part, argv[1]);
		exit(NO_FILE_SYSTEM);
    }
    fsStart = get4Bytes(ptptr + 4);
    fsSize = get4Bytes(ptptr + 8);
	
	// reading superblock and allocating free list 
	readBlock(disk, 1, blockBuffer);
	allocateFreeBlock(blockBuffer, &freeList);

	// for(int i = 0; i < NICFREE; i++){
	// 	printf("free block [%d] = %d\n", i, freeList[i]);
	// }

	return 0;
}

void readBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char *blockBuffer) {
  fseek(disk, fsStart * SECTOR_SIZE + blockNum * BLOCK_SIZE, SEEK_SET);

  if (fread(blockBuffer, BLOCK_SIZE, 1, disk) != 1) {
    fprintf(stderr, "cannot read block %lu (0x%lX)", blockNum, blockNum);
	exit(IO_ERROR);
  }
}

unsigned int get4Bytes(unsigned char *addr) {
  return (unsigned int) addr[0] << 24 |
         (unsigned int) addr[1] << 16 |
         (unsigned int) addr[2] <<  8 |
         (unsigned int) addr[3] <<  0;
}

void allocateFreeBlock(unsigned char *p, EOS32_daddr_t *freeList) {
	unsigned int nfree;
	EOS32_daddr_t addr;
	int i;

	nfree = get4Bytes(p); 

	p += 4;
	for (i = 0; i < NICFREE; i++) {
		addr = get4Bytes(p);
		p += 4;
		// adding block address to free list 
		if (i < nfree) {
			*(freeList + i) = addr; 
		}
	}
}
