#include <stdio.h>

#define INCORRECT_START 1
#define IMAGE_NOT_FOUND 2
#define IO_ERROR 3
#define ILLEGAL_PARTITION 4
#define NO_FILE_SYSTEM 5
#define MEMORY_ALLOC_ERROR 6
#define RANDOM_ERROR 9
#define BLOCK_NOT_FOUND 10
#define DATA_AND_FREELIST_BLOCK 11
#define BLOCK_DUPLICATE_FREELIST 12
#define BLOCK_DUPLICATE_DATA 13
#define DATA_SIZE_INCONSISTENT 14
#define INODE_LINK_COUNT_NULL_IN_DIR 15
#define INODE_LINK_COUNT_NULL_NOT_EMPTY 16
#define INODE_LINK_COUNT_APPEARANCE_FALSE 17
#define INODE_TYPE_FIELD_INVALID 18
#define INODE_FREE_IN_DIR 19
#define ROOT_INODE_NO_DIR 20
#define DIR_CANNOT_BE_REACHED_FROM_ROOT 21
#define UNDEFINED_FILE_SYSTEM_ERROR 99
#define UNDEFINED_ERROR 9


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

    unsigned int nfree; // number of entries in free block
    unsigned int *free_blocks;
} SuperBlock_Info;

typedef struct Inode {
    unsigned int mode;
  	unsigned int nlink;
  	EOS32_off_t size;
  	EOS32_daddr_t *addr;
} Inode;

void readSuperBlock(unsigned char *, SuperBlock_Info *);
void traversalTree(unsigned char *, unsigned int);
void readInodeTable(FILE *, unsigned char *, SuperBlock_Info *);
void readInodeBlock(FILE *disk, EOS32_daddr_t, unsigned int, unsigned char *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);
unsigned int get4Bytes(unsigned char *);