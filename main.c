/***************************************
 *     EOS32 File System Checker	   *
 *	   @version 1.0.0				   *
 *	   @author Pavlo Rozbytskyi		   *
 *	           David Omran             *
 ***************************************/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include "main.h"

/* ToDo-Group:
 * David: Exit-Code 13, 15-19
 * Pavlo: 
*/

/* Exit-Codes ToDo:
    Die Groesse einer Datei ist nicht konsistent mit den im Inode vermerkten Bloecken: Exit-Code 14.
    Alle anderen Dateisystem-Fehler: Exit-Code 99.
    Alle anderen Fehler: Exit-Code 9.
*/

/* Exit-Codes DONE
    Falscher Aufruf des Programms: Exit-Code 1.
    Image-Datei nicht gefunden: Exit-Code 2.
    Datei Ein/Ausgabefehler: Exit-Code 3.
    Illegale Partitionsnummer: Exit-Code 4.
    Partition enthaelt kein EOS32-Dateisystem: Exit-Code 5.
    Erfolgloser Aufruf von malloc(): Exit-Code 6.
*/

/* Tested commands:
 * 	Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
	Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
    Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
	Ein Block ist mehr als einmal in einer Datei oder in mehr als einer Datei: Exit-Code 13.
	Ein Inode mit Linkcount n != 0 erscheint nicht in exakt n Verzeichnissen: Exit-Code 17.
	Ein Inode mit Linkcount 0 ist nicht frei: Exit-Code 16.
	Ein Inode mit Linkcount 0 erscheint in einem Verzeichnis: Exit-Code 15.
	Ein Inode erscheint in einem Verzeichnis, ist aber frei: Exit-Code 19.
	Ein Verzeichnis kann von der Wurzel aus nicht erreicht werden: Exit-Code 21.
	Der Root-Inode ist kein Verzeichnis: Exit-Code 20.
	Ein Inode hat ein Typfeld mit illegalem Wert: Exit-Code 18. 
 */

static unsigned int fsStart;
// information about all blocks in file system
static Block_Info *blockInfos; 
// information about all inodes in file system
static Inode_Info *inodeInfos;
// file system start position
static unsigned int fsStart;

#define DEBUG

int main(int argc, char *argv[]){
	FILE *disk;
	
	unsigned char partTable[SECTOR_SIZE];
	unsigned char blockBuffer[BLOCK_SIZE];
	unsigned char *ptptr;
	unsigned int fsSize;
	unsigned int partType;
    unsigned int numBlocks;
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
		fprintf(stderr, "incorrect program: \"%s\" start\n", argv[0]);
		exit(INCORRECT_START);
	}

	if(access(argv[1], F_OK) == -1){
		fprintf(stderr, "file \"%s\" does not exist\n", argv[1]);
		exit(IMAGE_NOT_FOUND);
	}

	disk = fopen(argv[1], "rb");

  	if (disk == NULL) {
    	fprintf(stderr, "cannot open disk image file '%s', %d\n", argv[1], errno);
		exit(IO_ERROR);
  	}

	char *endptr;
	/* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10); // parsing number 
    if (*endptr != '\0' || part < 0 || part > 15) {
      	fprintf(stderr, "illegal partition number '%s'\n", argv[2]);
		exit(ILLEGAL_PARTITION);
    }
  	// setting file pointer to partition table
	fseek(disk, 1 * SECTOR_SIZE, SEEK_SET); 
    if (fread(partTable, 1, SECTOR_SIZE, disk) != SECTOR_SIZE) {
        fprintf(stderr, "cannot read partition table of disk '%s'\n", argv[1]);
	    exit(IO_ERROR);
    }
	// pointer to needed partition
	ptptr = partTable + part * 32;
    partType = get4Bytes(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
        fprintf(stderr, "partition %d of disk '%s' does not contain an EOS32 file system\n",
            part, argv[1]);
		exit(NO_FILE_SYSTEM);
    }

    fsStart = get4Bytes(ptptr + 4);
    fsSize = get4Bytes(ptptr + 8);

    if (fsSize % SPB != 0) {
        fprintf(stderr, "The File system size is not a multiple of block size\n");
        exit(UNDEFINED_FILE_SYSTEM_ERROR); // ToDo, is Exit-Code 99 correct?
    }
    numBlocks = fsSize / SPB;
    // printf("This equals %u (0x%X) blocks of %d bytes each.\n",
    //        numBlocks, numBlocks, BLOCK_SIZE);
    if (numBlocks < 2) {
        fprintf(stderr, "The File system has less than 2 blocks\n");
        exit(UNDEFINED_FILE_SYSTEM_ERROR); // Exit-Code Number 99
    }

	// reading superblock and allocating free list 
	readBlock(disk, 1, blockBuffer);
	readSuperBlock(blockBuffer, superBlock);

	// allocating memory for list with information about each 
	// memory block in the system
	blockInfos = malloc(superBlock->fsize * sizeof(unsigned int) * 2);
	if(blockInfos == NULL){
		fprintf(stderr, "cannot allocate memory for a list with information about each system block\n");
		exit(MEMORY_ALLOC_ERROR);
	}

	// allocating memory for inode information
	inodeInfos = malloc(INOPB * superBlock->isize * sizeof(Inode_Info));
	if(inodeInfos == NULL){
		fprintf(stderr, "cannot allocate memory for a list with information about each inode\n");
		exit(MEMORY_ALLOC_ERROR);
	}

	// reading inode table (inoded per block is 64)
	readInodeTable(disk, superBlock);
	readFreeBlocks(disk, superBlock);
	checkBlockInfos(superBlock, blockInfos);
	// recursive reading system files
	readSystemFiles(disk, superBlock);
	// handling inode errors
	checkInodeErrors(disk, inodeInfos, superBlock);

	fprintf(stdout, "completed\n");
	fclose(disk);
	return 0;
}

void checkInodeErrors(FILE *disk, Inode_Info *inodeInfos, SuperBlock_Info *superBlock){
	Inode *inode;
	inode = malloc(sizeof(Inode));
	if(inode == NULL){
		fprintf(stderr, "memory allocation error\n");
		exit(MEMORY_ALLOC_ERROR);
	}
	for(int i = 1; i < superBlock->isize * INOPB; i++){
		readInode2(disk, inode, i);
		// Ein Inode mit Linkcount 0 ist nicht frei: Exit-Code 16.
		if(inodeInfos[i].link_count == 0 && inode->mode != 0){
			fprintf(stderr, "inode [%d] with link count 0 is not free\n", i);
			exit(INODE_LINK_COUNT_NULL_NOT_FREE);
		}
		// Ein Inode mit Linkcount n != 0 erscheint nicht in exakt n Verzeichnissen: Exit-Code 17.
		if(inodeInfos[i].link_count != 0 && (inodeInfos[i].link_count != inode->nlink)){
			fprintf(stderr, "inode [%d] is in %d directories, but has link count of %d\n", i,
			inodeInfos[i].link_count, inode->nlink);
			exit(INODE_LINK_COUNT_APPEARANCE_FALSE);
		}
		// Ein Inode mit Linkcount 0 erscheint in einem Verzeichnis: Exit-Code 15.
		if(inodeInfos[i].parent && inodeInfos[i].link_count == 0){
			fprintf(stderr, "inode [%d] with link count 0 is in %d directories\n", i, inode->nlink);
			exit(INODE_LINK_COUNT_NULL_IN_DIR);
		}

		// Ein Inode erscheint in einem Verzeichnis, ist aber frei: Exit-Code 19.
		if(inodeInfos[i].parent && !inode->mode){
			fprintf(stderr, "inode [%d] should be free but is in a directory\n", i);
			exit(INODE_FREE_IN_DIR);			
		}
		// Ein Verzeichnis kann von der Wurzel aus nicht erreicht werden: Exit-Code 21.
		if(isDir(inode) && inodeInfos[i].parent == 0){
			fprintf(stderr, "inode [%d] cannot be reached from root\n", i);
			exit(DIR_CANNOT_BE_REACHED_FROM_ROOT);
		}

		if(inode->mode != 0 && !checkIllegalType(inode->mode)){
			fprintf(stderr, "inode [%d] inode has an illegal type\n", i);
			exit(INODE_TYPE_FIELD_INVALID);			
		}
	}
}

void checkBlockInfos(SuperBlock_Info *superBlock, Block_Info *blockInfo){
	// skipping boot block, superblock and inode blocks
	for(int i = superBlock->isize + 2; i < superBlock->fsize; i++){
		// checking block is neither in file or in free list
		if(blockInfo[i].file_occur == 0 &&
		   blockInfo[i].free_list_occur == 0){
			fprintf(stderr, "block [%d] is neither in file or in free list\n", i);
			exit(NEITHER_IN_FILE_OR_FREELIST);
		}

		if(blockInfo[i].file_occur > 0 &&
		   blockInfo[i].free_list_occur > 0){
			fprintf(stderr, "block [%d] is in file and in free list\n", i);
			exit(IN_FILE_IN_FREELIST);
		}

		if(blockInfo[i].free_list_occur > 1){
			fprintf(stderr, "block [%d] has multiple occurrences in free list\n", i);
			exit(MULTIPLE_TIMES_FREELIST);
		}

		if(blockInfo[i].file_occur > 1){
			fprintf(stderr, "block [%d] has multiple occurrences in files\n", i);
			exit(BLOCK_DUPLICATE_DATA);
		}
	}
}

void readInodeTable(FILE *disk, SuperBlock_Info *superBlock){
	// iterating over inode blocks
	unsigned char buffer[BLOCK_SIZE];
	unsigned char *p = buffer;

	for(int i = 2; i < superBlock->isize + 2; i++){
		readInodeBlock(disk, i, p);
	}
}

void readSystemFiles(FILE *disk, SuperBlock_Info *superBlock){
	Inode *inode;
	inode = malloc(sizeof(Inode));
	// iterating over file system blocks
	if(inode == NULL){
		fprintf(stderr, "cannot allocate memory for inode\n");
		exit(MEMORY_ALLOC_ERROR);
	}
	// start from root inode
	readInode2(disk, inode, 1);
	if(!isDir(inode)){
		// throw exception if root inode is not dir
		fprintf(stderr, "root inode is not a directory\n");
		exit(ROOT_INODE_NOT_DIR);
	}
	// parent of root is root
	inodeInfos[1].parent = 1;
	free(inode);
	// steping into root inode
	stepIntoInode(disk, 1, 0);
}

unsigned char stepIntoInode(FILE *disk, EOS32_daddr_t inodeNum, EOS32_daddr_t parentInode){
	EOS32_daddr_t inodeBlockSize = 0;
	EOS32_daddr_t *refs;
	Inode *inode;
	inode = malloc(sizeof(Inode));
	// iterating over file system blocks
	if(inode == NULL){
		fprintf(stderr, "cannot allocate memory for inode\n");
		exit(MEMORY_ALLOC_ERROR);
	}

	refs = malloc(sizeof(EOS32_daddr_t) * 8);
	if(refs == NULL){
		fprintf(stderr, "cannot allocate memory for inode\n");
		exit(MEMORY_ALLOC_ERROR);
	}
	readInode2(disk, inode, inodeNum);
	
	// todo: size
	if(isFile(inode)){
		calculateInodeSize(disk, inode, &inodeBlockSize);

		if(inodeBlockSize > 0){
			if(inodeBlockSize * BLOCK_SIZE < inode->size || 
			   (inodeBlockSize - 1) * BLOCK_SIZE > inode->size){
				fprintf(stdout, "inconsistent size of inode[%d] = %d\n should be beween %d and %d\n", inodeNum, inode->size, inodeBlockSize * BLOCK_SIZE,  (inodeBlockSize - 1) * BLOCK_SIZE);
				exit(DATA_SIZE_INCONSISTENT);
			}
		}else if(inode->size != 0){
			fprintf(stdout, "inconsistent size of inode[%d] = %d\n", inodeNum, inode->size);
			exit(DATA_SIZE_INCONSISTENT);
		}

	}

	if(!isDir(inode) && inodeNum == 1){
		// throw exception if root inode is not dir
		fprintf(stderr, "root inode is not a directory\n");
		exit(ROOT_INODE_NOT_DIR);
	}
	// checking invalid type
	if(!checkIllegalType(inode->mode)){
		fprintf(stderr, "inode [%d] type is invalid\n", inodeNum);
		exit(INODE_TYPE_FIELD_INVALID);
	}

	// traversal inode if directory
	if(isDir(inode)){	
		// todo: copy pointers
		for(int i = 0; i < 8; i++){
			refs[i] = inode->refs[i];
		}
		free(inode);
		// recursive descent into all directory blocks of current directory
		for(int i = 0; i < INODE_BLOCKS_COUNT; i++){
			if(refs[i] == 0){
				// directory does not contain more directories 
				break;
			}
			// handling simple cases with direct blocks
			if(i < 6){
				stepIntoDirectoryBlock(disk, parentInode, inodeNum, refs[i]);
			}else {
				//todo: handling single and double indirection
			}
		}
	free(refs);
	}else{
		//calculating file size, step into file block
		
		free(refs);
		// is not directory
		inodeInfos[inodeNum].link_count = 1;
		inodeInfos[inodeNum].parent = 1;

		return 0;
	}
	return 1; // directory
}

int isFile(Inode *inode){
	if ((inode->mode & IFMT) == IFREG) {
        return 1;
	} 
	return 0;
}

void calculateInodeSize(FILE *disk, Inode *inode, EOS32_daddr_t *size){
	for(int i = 0; i < 8; i++){
		if(i == 6 && inode->refs[i] != 0){
			visitBlock(disk, inode->refs[i], size, SINGLE_INDIRECT);
		}else if(i == 7 && inode->refs[i] != 0){
			visitBlock(disk, inode->refs[i], size, DOUBLE_INDIRECT);
		}else if(inode->refs[i] == 0){
			break;
		}else{
			*size += 1;
		}
	}
}

void visitBlock(FILE *disk, EOS32_daddr_t blockNum, EOS32_daddr_t *size, unsigned char doubleIndirect){
	unsigned char buffer [BLOCK_SIZE];
	unsigned char *p;
	EOS32_daddr_t addr;
  	int i;

	p = buffer;

	readBlock(disk, blockNum, buffer);
	for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
		addr = get4Bytes(p);
		p += 4;
		
		if(addr > 0){
			if(doubleIndirect == DOUBLE_INDIRECT){
				indirectBlock(disk, addr, SINGLE_INDIRECT);
			}else{
				*size += 1;	
			}
		}else{
			break;
		}
	}
}

void indirectBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char doubleIndirect) {
  	unsigned char buffer [BLOCK_SIZE];
	unsigned char *p;
	EOS32_daddr_t addr;
  	int i;

	p = buffer;

	readBlock(disk, blockNum, buffer);
	for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
		addr = get4Bytes(p);
		p += 4;
		
		if(addr > 0){
			blockInfos[addr].file_occur++;
			if(doubleIndirect == DOUBLE_INDIRECT){
				indirectBlock(disk, addr, SINGLE_INDIRECT);
			}
		}else{
			break;
		}
	}
}

void stepIntoDirectoryBlock(FILE *disk, EOS32_daddr_t parentInode,
				 EOS32_daddr_t inodeNum, EOS32_daddr_t currentDirBlock){
	// todo: count num of files
	EOS32_ino_t ino;
  	int i, j;
	unsigned int linksCount = 0;
	unsigned char buffer[BLOCK_SIZE];
	unsigned char *p = buffer;
	unsigned char dirFlag;

	readBlock(disk, currentDirBlock, p);
	for (i = 0; i < DIRPB; i++) {
		// getting inode number 
		ino = get4Bytes(p);
		
		// step into inode if its not current inode, zero inode or parent
		if(ino != inodeNum && ino != 0 && ino != parentInode){
			dirFlag = stepIntoInode(disk, ino, inodeNum);
			// count directories
			if(dirFlag){
				linksCount++;
			}
		}else if((ino == inodeNum || ino == parentInode) && ino != 0){
			// count links to parent and current inode as well
			linksCount++;
		}
		
		if(ino == 0){
			// no more inodes in this directory
			break;
		}
		// move pointer to next dir
		p += DIRSIZ + 4;
	}
	// calculated links = i - 1 (".")  
	inodeInfos[inodeNum].link_count += linksCount;
	inodeInfos[inodeNum].parent = 1;
}

int isDir(Inode *inode){
	if((inode->mode & IFMT) == IFDIR){
		return 1;
	}
	return 0;
}

int checkIllegalType(unsigned int mode){
	if ((mode & IFMT) == IFREG ||
	    (mode & IFMT) == IFDIR ||
		(mode & IFMT) == IFCHR || 
		(mode & IFMT) == IFBLK){
		
		return 1;
	}
	return 0;
}

void readInode2(FILE *disk, Inode *node, unsigned int inoNum){
	unsigned char buffer[BLOCK_SIZE];
	unsigned char *p = buffer;
	
	unsigned int blockNum = inoNum / INOPB + 2;
	unsigned int position = inoNum % INOPB;
	// reading block with current inode 
	readBlock(disk, blockNum, p);
	// move pointer to inode
	p += INOPB * position;
	// reading inode
	readInode(p, node);	
}

void readInode(unsigned char *p, Inode *node){
	unsigned int _mode;
  	unsigned int _nlink;
  	EOS32_off_t _size;
	EOS32_daddr_t _addr;
	EOS32_daddr_t *_refs = malloc(sizeof(EOS32_daddr_t) * 8);

	if(_refs == NULL){
		exit(MEMORY_ALLOC_ERROR);
	} 

	_mode = get4Bytes(p);
	p += 4;
		
	_nlink = get4Bytes(p);
	p += 24;

	_size = get4Bytes(p);
	p += 4;

	node->mode 	= _mode;
	node->nlink = _nlink;
	node->size  = _size;
	node->refs  = _refs;
	// reading addresses
	// skipping handling blocks of character and block devices
	if (((_mode & IFMT) != IFCHR) && ((_mode & IFMT) != IFBLK)) {
		for (int j = 0; j < 6; j++) {
			// iterating over direct blocks
			_addr = get4Bytes(p);
			p += 4;

			_refs[j] = _addr;
		}
		
		// getting single indirect
		_addr	 = get4Bytes(p);
		_refs[6] = _addr;

		p += 4;
		
		// getting double indirect
		_addr = get4Bytes(p);
		_refs[7] = _addr;
	}
}

void readInodeBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char *p){
	unsigned int mode;
  	unsigned int nlink;
  	EOS32_off_t size;
  	EOS32_daddr_t addr;

  	int i, j;

	readBlock(disk, blockNum, p);
  	for (i = 0; i < INOPB; i++) {
    	mode = get4Bytes(p);
    	p += 4;
		
		nlink = get4Bytes(p);
		p += 24;

		size = get4Bytes(p);
		p += 4;
		// skipping handling blocks of character and block devices
		if (((mode & IFMT) != IFCHR) && ((mode & IFMT) != IFBLK)) {
			for (j = 0; j < 6; j++) {
				// iterating over direct blocks
				addr = get4Bytes(p);
				p += 4;

				// incrementing occurrece in files if block address not zero
				if(addr > 0){
					blockInfos[addr].file_occur++;
				}
			}
			
			// getting single indirect
			addr = get4Bytes(p);
			p += 4;

			if(addr > 0){
				blockInfos[addr].file_occur++;
				// reading indirect block
				indirectBlock(disk, addr, SINGLE_INDIRECT);
			}
			// getting double indirect
			addr = get4Bytes(p);
			p += 4;

			if(addr > 0){
				blockInfos[addr].file_occur++;
				// reading double indirect block
				indirectBlock(disk, addr, DOUBLE_INDIRECT);
			}
		}else{
			p += 32;
		}
	}
}

void readFreeBlocks(FILE *disk, SuperBlock_Info *superBlock_Info){
	EOS32_daddr_t freeBlksSize = superBlock_Info->nfree;
	EOS32_daddr_t *freeList     = superBlock_Info->free_blocks;
	// recursive traversal free blocks
	freeBlock(disk, freeList, freeBlksSize);
}

void freeBlock(FILE *disk, EOS32_daddr_t *freeList, EOS32_daddr_t freeBlksSize){
	unsigned char buffer[BLOCK_SIZE];
	unsigned char *p = buffer;
	EOS32_daddr_t addr;
	unsigned int nfree;
	EOS32_daddr_t *newFreeList;

	for(int i = 0; i < NICFREE; i++){
		if(i < freeBlksSize){
			if(freeList[i] > 0 && i == 0){
				readBlock(disk, freeList[i], p);
				
				nfree = get4Bytes(p);
				p += 4;

				newFreeList = malloc(sizeof(unsigned int) * nfree);
				if(newFreeList == NULL){
					fprintf(stderr, "memory allocation error\n");
					exit(MEMORY_ALLOC_ERROR);
				}

				// recursive descent into (freeList[i])
				for(int j = 0; j < NICFREE; j++){
					addr = get4Bytes(p);
					p += 4;
					if(j < nfree){
						*(newFreeList + j) = addr;
					}  
				}
				freeBlock(disk, newFreeList, nfree);
				free(newFreeList);
			}
			blockInfos[freeList[i]].free_list_occur++;
		}
	}	
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
  	
	EOS32_daddr_t *freeBlocks;
	freeBlocks = malloc(sizeof(unsigned int) * nfree);
	if(freeBlocks == NULL){
		fprintf(stderr, "cannot allocate memory for free block list\n");
		exit(MEMORY_ALLOC_ERROR);
	}
	
  	for (i = 0; i < NICFREE; i++) {
		free = get4Bytes(p);
		p += 4;
		if(i < nfree){
			*(freeBlocks + i) = free;  
		}
	}

	superBlock_Info->nfree = nfree;
	superBlock_Info->freeblks = freeblks;
	superBlock_Info->freeinos = freeinos;
	superBlock_Info->fsize = fsize;
	superBlock_Info->isize = isize;
	superBlock_Info->free_blocks = freeBlocks;
}


void readBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char *blockBuffer) {
	fseek(disk, fsStart * SECTOR_SIZE + blockNum * BLOCK_SIZE, SEEK_SET);

  	if (fread(blockBuffer, BLOCK_SIZE, 1, disk) != 1) {
    	fprintf(stderr, "cannot read block %i (0x%i)\n", blockNum, blockNum);
		exit(IO_ERROR);
  	}
}

unsigned int get4Bytes(unsigned char *addr) {
  return (unsigned int) addr[0] << 24 |
         (unsigned int) addr[1] << 16 |
         (unsigned int) addr[2] <<  8 |
         (unsigned int) addr[3] <<  0;
}