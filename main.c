#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"

unsigned int fsStart;

#define DEBUG

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

	SuperBlock_Info *superBlock;
	superBlock = malloc(sizeof(EOS32_daddr_t) * 3 + 
						sizeof(EOS32_ino_t) + 
						sizeof(unsigned int) + 
						sizeof(unsigned int *));

	if(superBlock == NULL){
		fprintf(stderr, "cannot allocate memory for superblock\n");
		exit(MEMORY_ALLOC_ERROR);
	}

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
    	fprintf(stderr, "cannot open disk image file '%s', %d", argv[1], errno);
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
	readSuperBlock(blockBuffer, superBlock);

	#ifdef DEBUG
	for(int i = 0; i < superBlock->nfee; i++){
		fprintf(stdout, "free block[%d] = %d\n", i, superBlock->free_blocks[i]);
	}
	#endif
	
	return 0;
}

void readSuperBlock(unsigned char *p, SuperBlock_Info *superBlock_Info) {
	EOS32_daddr_t fsize;
	EOS32_daddr_t isize;
	EOS32_daddr_t freeblks;
	EOS32_ino_t freeinos;
	EOS32_daddr_t free;
	unsigned int nfree; 
	int i;

	p += 4;
	fsize = get4Bytes(p);
	p += 4;
	isize = get4Bytes(p);
	p += 4;
	freeblks = get4Bytes(p);
	p += 4;
	freeinos = get4Bytes(p);

	p += 8;
  	for (i = 0; i < NICINOD; i++) {
    	p += 4;
  	}
	
	nfree = get4Bytes(p);
	p += 4;
  	
	unsigned int *freeBlocks;
	freeBlocks = malloc(sizeof(unsigned int) * nfree);
	if(freeBlocks == NULL){
		fprintf(stderr, "cannot allocate memory for free block list\n");
		exit(MEMORY_ALLOC_ERROR);
	}
	
  	for (i = 0; i < NICFREE; i++) {
		free = get4Bytes(p);
		p += 4;
		if (i < nfree) {
			*(freeBlocks + i) = free;  
		}
  	}

	superBlock_Info->nfee = nfree;
	superBlock_Info->freeblks = freeblks;
	superBlock_Info->freeinos = freeinos;
	superBlock_Info->fsize = fsize;
	superBlock_Info->isize = isize;
	superBlock_Info->free_blocks = freeBlocks;
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

void traversalTree(unsigned char *p, unsigned int blockNum) {
  unsigned int mode;
  unsigned int nlink;
  EOS32_off_t size;
  EOS32_daddr_t addr;
  int i, j;

  for (i = 0; i < INOPB; i++) {
    mode = get4Bytes(p);
    p += 4;
    
    nlink = get4Bytes(p);
    p += 4;

	size = get4Bytes(p);
    p += 4;
    
	for (j = 0; j < 6; j++) {
      addr = get4Bytes(p);
      p += 4;
    }
	// indirect address
    addr = get4Bytes(p);
    p += 4;
	// double indirect address
    addr = get4Bytes(p);
  }
}
