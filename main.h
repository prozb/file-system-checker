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

#define SECTOR_SIZE	512	/* disk sector size in bytes */
#define BLOCK_SIZE	4096	/* disk block size in bytes */
#define NICFREE		500	/* maximum number of free blocks in superblock */
#define NICINOD		500	/* number of free inodes in superblock */

#define INOPB		64	/* number of inodes per block */
#define DIRPB		64	/* number of directory entries per block */
#define DIRSIZ		60	/* max length of path name component */

#define IFMT		070000	/* type of file */
#define   IFREG		040000	/* regular file */
#define   IFDIR		030000	/* directory */
#define   IFCHR		020000	/* character special */
#define   IFBLK		010000	/* block special */
#define   IFFREE	000000	/* reserved (indicates free inode) */
#define ISUID		004000	/* set user id on execution */
#define ISGID		002000	/* set group id on execution */
#define ISVTX		001000	/* save swapped text even after use */
#define IUREAD		000400	/* user's read permission */
#define IUWRITE		000200	/* user's write permission */
#define IUEXEC		000100	/* user's execute permission */
#define IGREAD		000040	/* group's read permission */
#define IGWRITE		000020	/* group's write permission */
#define IGEXEC		000010	/* group's execute permission */
#define IOREAD		000004	/* other's read permission */
#define IOWRITE		000002	/* other's write permission */
#define IOEXEC		000001	/* other's execute permission */

typedef unsigned int EOS32_ino_t;
typedef unsigned int EOS32_daddr_t;
typedef unsigned int EOS32_off_t;
typedef int EOS32_time_t;

typedef struct Block_Info {
    unsigned int file_occur;
    unsigned int free_list_occur;
    EOS32_daddr_t address;
} Block_Info;

typedef struct SuperBlock_Info {
    EOS32_daddr_t fsize; // file system size
	EOS32_daddr_t isize; // inode list size
	EOS32_daddr_t freeblks; // free blocks size
	EOS32_ino_t freeinos; // free inodes size

    unsigned int nfee; // number of entries in free block
    unsigned int *free_blocks;
} SuperBlock_Info;

void readSuperBlock(unsigned char *, SuperBlock_Info *);
void traversalTree(unsigned char *, unsigned int);
void allocateFreeBlock(unsigned char *, EOS32_daddr_t *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);
unsigned int get4Bytes(unsigned char *);
