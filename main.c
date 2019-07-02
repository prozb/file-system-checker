#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include "main.h"

/* ToDo-Group:
 * David: set mode and nlink for inodes
 * Pavlo: recursive traversal over file system 
*/

/* Exit-Codes ToDo:
    Die Groesse einer Datei ist nicht konsistent mit den im Inode vermerkten Bloecken: Exit-Code 14.
    Der Root-Inode ist kein Verzeichnis: Exit-Code 20.
    Ein Verzeichnis kann von der Wurzel aus nicht erreicht werden: Exit-Code 21.
    Alle anderen Dateisystem-Fehler: Exit-Code 99.
    Alle anderen Fehler: Exit-Code 9.
*/

/* Exit-Codes pending
    Erfolgloser Aufruf von malloc(): Exit-Code 6.
    */


/* Exit-Codes DONE
    Falscher Aufruf des Programms: Exit-Code 1.
    Image-Datei nicht gefunden: Exit-Code 2.
    Datei Ein/Ausgabefehler: Exit-Code 3.
    Illegale Partitionsnummer: Exit-Code 4.
    Partition enthaelt kein EOS32-Dateisystem: Exit-Code 5.
	Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
	Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
    Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
	Ein Block ist mehr als einmal in einer Datei oder in mehr als einer Datei: Exit-Code 13.
    Ein Inode mit Linkcount 0 erscheint in einem Verzeichnis: Exit-Code 15.
    Ein Inode mit Linkcount 0 ist nicht frei: Exit-Code 16.
    Ein Inode mit Linkcount n != 0 erscheint nicht in exakt n Verzeichnissen: Exit-Code 17.
    Ein Inode hat ein Typfeld mit illegalem Wert: Exit-Code 18.
    Ein Inode erscheint in einem Verzeichnis, ist aber frei: Exit-Code 19.
*/

static unsigned int fsStart;
// information about all blocks in file system
static Block_Info *blockInfos;
static Inode *inodeInfos;
unsigned char partTable[SECTOR_SIZE];
unsigned char blockBuffer[BLOCK_SIZE];

#define DEBUG

int main(int argc, char *argv[]) {
    FILE *disk;

    // free blocks list (need to check that files are not in this list)
    EOS32_daddr_t freeList[NICFREE];
    unsigned char *blockPointer;
    unsigned char *ptptr;
    unsigned int fsSize;
    unsigned int partType;
    unsigned int freeListSize;
    unsigned int numBlocks;
    int part;

    SuperBlock_Info *superBlock;
    superBlock = malloc(sizeof(EOS32_daddr_t) * 3 +
                        sizeof(EOS32_ino_t) +
                        sizeof(unsigned int) +
                        sizeof(unsigned int *));

    if (superBlock == NULL) {
        fprintf(stderr, "cannot allocate memory for superblock\n");
        exit(MEMORY_ALLOC_ERROR);
    }

    if (argc < 3) {
        fprintf(stderr, "incorrect program: \"%s\" start\n", argv[1]);
        exit(INCORRECT_START);
    }

    if (access(argv[1], F_OK) == -1) {
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

    if (fsSize % SPB != 0) {
        fprintf(stderr, "The File system size is not a multiple of block size.\n");
        exit(UNDEFINED_FILE_SYSTEM_ERROR); // ToDo, is Exit-Code 99 correct?
    }
    numBlocks = fsSize / SPB;
    printf("This equals %u (0x%X) blocks of %d bytes each.\n",
           numBlocks, numBlocks, BLOCK_SIZE);
    if (numBlocks < 2) {
        fprintf(stderr, "The File system has less than 2 blocks");
        exit(UNDEFINED_FILE_SYSTEM_ERROR); // Exit-Code Number 99
    }

    // reading superblock and allocating free list
    readBlock(disk, 1, blockBuffer);
    readSuperBlock(blockBuffer, superBlock);

    // allocating memory for list with information about each
    // memory block in the system
    blockInfos = malloc(superBlock->fsize * sizeof(unsigned int) * 2);
    if (blockInfos == NULL) {
        fprintf(stderr, "cannot allocate memory for list with information about each system block\n");
        exit(MEMORY_ALLOC_ERROR);
    }
    ptptr += 8;
    inodeInfos->size = get4Bytes(ptptr);
    // allocating memory for inode blocks in the system
    inodeInfos = malloc(inodeInfos->size * sizeof(Inode) * INOPB);
    if(inodeInfos == NULL){
        fprintf(stderr, "cannot allocate memory for list with information about each system block\n");
        exit(MEMORY_ALLOC_ERROR);
    }

#ifdef DEBUG
    // for(int i = 0; i < superBlock->nfree; i++){
    // 	fprintf(stdout, "free block[%d] = %d\n", i, superBlock->free_blocks[i]);
    // }
#endif
    // reading inode table (inode per block is 64)
    readInodeTable(disk, superBlock);
    readFreeBlocks(disk, superBlock);
    checkBlockInfos(superBlock, blockInfos);
    checkInodeTable(disk, superBlock, inodeInfos);

#ifdef DEBUG
    // for(int i = 4900; i < 4999 ; i++){
    // 	fprintf(stdout, "block [%d] = [%d , %d]\n", i, (blockInfos + i)->file_occur,
    // 	(blockInfos + i)->free_list_occur);
    // }
#endif
    printf("File check completed successfully");
    fclose(disk);
    return 0;
}

void checkInodeTable(FILE* disk, SuperBlock_Info *superBlock, Inode *pInode) {
    unsigned char buffer[BLOCK_SIZE];
    unsigned char *buff = buffer;
    // skipping boot block and superblock
    for (int pointer = superBlock->isize + 2; pointer < superBlock->fsize; pointer++) {
        readInodeBlock(disk, pointer, buff);
        buff = blockBuffer;
        // iterate trough all the Inodes inside the Block (64)
        for (int i = 0; i < INOPB; i++) {
            // set the inodeID to the current Inode inside the Block
            pInode->inodeID = (i + (pointer * INOPB));
            unsigned int mode;
            unsigned int nlink;

            // check if the current InodeID is not zero (free)
            if (pInode[i].inodeID != 0) {
                // allocate the Inode mode
                mode = get4Bytes(buff);
                buff += 4;
                if (mode != 0) {
                    // allocates the inode Type of the current Inode
                    unsigned int inodeType = mode & IFMT;
                    if (inodeType != IFREG && inodeType != IFDIR && inodeType != IFCHR && inodeType != IFBLK) {
                        fprintf(stderr, "Illegal inodeType value 0x%08X \n", i);
                        exit(INODE_TYPE_FIELD_INVALID);
                    }
                    // checks if the inode Type of the inode is a directory
                    if (inodeType == IFDIR && pInode[i].mode == 0) {
                        fprintf(stderr, "The Inode %d is a directory, but cannot be reached from the root \n", pInode->inodeID);
                        exit(DIR_CANNOT_BE_REACHED_FROM_ROOT);
                    }
                    // ToDo: fix condition, why is this always true???
                } else if (mode == 0 && pInode[i].mode == 1) {
                    fprintf(stderr, "The Inode %d appears in a directory but is free\n", pInode->inodeID);
                    exit(INODE_FREE_IN_DIR);
                }
                // allocate the Inode-Link counter
                nlink = get4Bytes(buff);
                buff += 4;

                if (nlink == 0) {
                    if (mode != 0) {
                        fprintf(stderr, "The Inode %d has a Linkcount of 0 but it is not free \n", pInode->inodeID);
                        exit(INODE_LINK_COUNT_NULL_NOT_EMPTY);
                    }
                    if (pInode[i].nlink != 0) {
                        fprintf(stderr, "The Inode %u has Linkcount of 0 but it appears in a directory %u times \n", pInode->inodeID, pInode[i].nlink);
                        exit(INODE_LINK_COUNT_NULL_IN_DIR);
                    }
                }

                if (nlink != pInode[i].nlink) {
                    fprintf(stderr, "The Linkcount is %d but the Inode %d appears %d times in directory \n", nlink, pInode->inodeID, pInode[i].nlink);
                    exit(INODE_LINK_COUNT_APPEARANCE_FALSE);
                }
                // set the pointer to 64 which is the end of a block (4+4+56)
                buff += 56;
                // else: the InodeID is zero, or all 64 Inodes have been inspected.
            } else {
                buff += 64;
            }
        }
    }


}

void checkBlockInfos(SuperBlock_Info *superBlock, Block_Info *blockInfo) {
    // skipping boot block, superblock and inode blocks
    for (int i = superBlock->isize + 2; i < superBlock->fsize; i++) {
        // checking block is neither in file or in free list
        if (blockInfo[i].file_occur == 0 &&
            blockInfo[i].free_list_occur == 0) {
            fprintf(stderr, "block [%d] is neither in file or in free list\n", i);
            exit(NEITHER_IN_FILE_OR_FREELIST);
        }

        if (blockInfo[i].file_occur > 0 &&
            blockInfo[i].free_list_occur > 0) {
            fprintf(stderr, "block [%d] is in file and in free list\n", i);
            exit(IN_FILE_IN_FREELIST);
        }

        if (blockInfo[i].free_list_occur > 1) {
            fprintf(stderr, "block [%d] has multiple occurrences in free list\n", i);
            exit(NEITHER_IN_FILE_OR_FREELIST);
        }

        if (blockInfo[i].file_occur > 1) {
            fprintf(stderr, "block [%d] has multiple occurrences files\n", i);
            exit(BLOCK_DUPLICATE_DATA);
        }
    }
}

void readInodeTable(FILE *disk, SuperBlock_Info *superBlock) {
    // iterating over inode blocks
    unsigned char buffer[BLOCK_SIZE];
    unsigned char *p = buffer;
    // todo: implement check bootable partition!!!
    // not bootable partition doesnt have bootblock
    for(int i = 2; i < superBlock->isize + 2; i++){
        readInodeBlock(disk, i, p);
    }
}

void readInodeBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char *p) {
    readBlock(disk, blockNum, p);

    unsigned int mode;
    unsigned int nlink;
    EOS32_off_t size;
    EOS32_daddr_t addr;

    int i, j;

    for (i = 0; i < INOPB; i++) {
        unsigned char *backup = p;
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

                // incrementing occurrence in files if block address not zero
                if (addr > 0) {
                    blockInfos[addr].file_occur++;
                }
            }

            // getting single indirect
            addr = get4Bytes(p);
            p += 4;

            if (addr > 0) {
                blockInfos[addr].file_occur++;
                // reading indirect block
                indirectBlock(disk, addr, SINGLE_INDIRECT);
            }
            // getting double indirect
            addr = get4Bytes(p);
            p += 4;

            if (addr > 0) {
                blockInfos[addr].file_occur++;
                // reading double indirect block
                indirectBlock(disk, addr, DOUBLE_INDIRECT);
            }
        } else {
            p += 32;
        }
    }
}

void indirectBlock(FILE *disk, EOS32_daddr_t blockNum, unsigned char doubleIndirect) {
    unsigned char buffer[BLOCK_SIZE];
    unsigned char *p;
    EOS32_daddr_t addr;
    int i;

    p = buffer;

    readBlock(disk, blockNum, buffer);
    for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
        addr = get4Bytes(p);
        p += 4;

        if (addr > 0) {
            blockInfos[addr].file_occur++;
            if (doubleIndirect == DOUBLE_INDIRECT) {
                indirectBlock(disk, addr, SINGLE_INDIRECT);
            }
        } else {
            break;
        }
    }
}

void readFreeBlocks(FILE *disk, SuperBlock_Info *superBlock_Info) {
    EOS32_daddr_t freeBlksSize = superBlock_Info->nfree;
    EOS32_daddr_t *freeList = superBlock_Info->free_blocks;
    // recursive traversal free blocks
    freeBlock(disk, freeList, freeBlksSize);
}

void freeBlock(FILE *disk, EOS32_daddr_t *freeList, EOS32_daddr_t freeBlksSize) {
    unsigned char buffer[BLOCK_SIZE];
    unsigned char *p = buffer;
    EOS32_daddr_t addr;
    unsigned int nfree;
    EOS32_daddr_t *newFreeList;

    for (int i = 0; i < NICFREE; i++) {
        if (i < freeBlksSize) {
            if (freeList[i] > 0 && i == 0) {
                readBlock(disk, freeList[i], p);

                nfree = get4Bytes(p);
                p += 4;

                newFreeList = malloc(sizeof(unsigned int) * nfree);
                if (newFreeList == NULL) {
                    exit(MEMORY_ALLOC_ERROR);
                }

                // recursive descent into (freeList[i])
                for (int j = 0; j < NICFREE; j++) {
                    addr = get4Bytes(p);
                    p += 4;
                    if (j < nfree) {
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
    if (freeBlocks == NULL) {
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
        fprintf(stderr, "cannot read block %lu (0x%lX)", blockNum, blockNum);
        exit(IO_ERROR);
    }
}

unsigned int get4Bytes(unsigned char *addr) {
    return (unsigned int) addr[0] << 24 |
           (unsigned int) addr[1] << 16 |
           (unsigned int) addr[2] << 8 |
           (unsigned int) addr[3] << 0;
}