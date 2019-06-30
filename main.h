#include <stdio.h>

#define INCORRECT_START 1
#define IMAGE_NOT_FOUND 2
#define IO_ERROR 3
#define ILLEGAL_PARTITION 4
#define NO_FILE_SYSTEM 5
#define MEMORY_ALLOC_ERROR 6
#define RANDOM_ERROR 9

// a) Falscher Aufruf des Programms: Exit-Code 1. 
// b) Image-Datei nicht gefunden: Exit-Code 2. 
// c) Datei Ein/Ausgabefehler: Exit-Code 3. 
// d) Illegale Partitionsnummer: Exit-Code 4. 
// e) Partition enthaelt kein EOS32-Dateisystem: Exit-Code 5. 
// f) Erfolgloser Aufruf von malloc(): Exit-Code 6. 
// g) Alle anderen Fehler: Exit-Code 9

// Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
// Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
// Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
#define NEITHER_IN_FILE_OR_FREELIST 10
#define IN_FILE_IN_FREELIST 11
#define MULTIPLE_TIMES_FREELIST 12

#define SINGLE_INDIRECT 0
#define DOUBLE_INDIRECT 1

#define SECTOR_SIZE	512	/* disk sector size in bytes */
#define BLOCK_SIZE	4096	/* disk block size in bytes */
#define NICFREE		500	/* maximum number of free blocks in superblock */
#define NICINOD		500	/* number of free inodes in superblock */
#define IFMT		070000	/* type of file */
#define INOPB		64	/* number of inodes per block */
#define DIRPB		64	/* number of directory entries per block */
#define DIRSIZ		60	/* max length of path name component */

#define   IFDIR		030000	/* directory */
#define   IFCHR		020000	/* character special */
#define   IFBLK		010000	/* block special */


typedef unsigned int EOS32_ino_t;
typedef unsigned int EOS32_daddr_t;
typedef unsigned int EOS32_off_t;
typedef int EOS32_time_t;

typedef struct Block_Info {
    unsigned int file_occur;
    unsigned int free_list_occur;
} Block_Info;

typedef struct SuperBlock_Info {
    EOS32_daddr_t fsize; // file system size in blocks
	EOS32_daddr_t isize; // inode list size
	EOS32_daddr_t freeblks; // free blocks size
	EOS32_ino_t freeinos; // free inodes size

    unsigned int nfree; // number of entries in free block
    EOS32_daddr_t *free_blocks;
} SuperBlock_Info;

typedef struct Inode {
    unsigned int mode;
  	unsigned int nlink;

  	EOS32_off_t size;
  	EOS32_daddr_t addr;
} Inode;

void checkBLockInfos(SuperBlock_Info *, Block_Info *);
void indirectBlock(FILE *, EOS32_daddr_t, unsigned char);
void freeBlock(FILE *, EOS32_daddr_t *, EOS32_daddr_t);
void readFreeBlocks(FILE *, SuperBlock_Info *);
void readSuperBlock(unsigned char *, SuperBlock_Info *);
void readInodeTable(FILE *, SuperBlock_Info *);
void readInodeBlock(FILE *disk, EOS32_daddr_t, unsigned char *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);
unsigned int get4Bytes(unsigned char *);