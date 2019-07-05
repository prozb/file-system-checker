#include <stdio.h>

#define INCORRECT_START 1
#define IMAGE_NOT_FOUND 2
#define IO_ERROR 3
#define ILLEGAL_PARTITION 4
#define NO_FILE_SYSTEM 5
#define MEMORY_ALLOC_ERROR 6
#define RANDOM_ERROR 9
#define NEITHER_IN_FILE_OR_FREELIST 10
#define IN_FILE_IN_FREELIST 11
#define MULTIPLE_TIMES_FREELIST 12
#define BLOCK_DUPLICATE_DATA 13
#define DATA_SIZE_INCONSISTENT 14
#define INODE_LINK_COUNT_NULL_IN_DIR 15
#define INODE_LINK_COUNT_NULL_NOT_FREE 16
#define INODE_LINK_COUNT_APPEARANCE_FALSE 17
#define INODE_TYPE_FIELD_INVALID 18
#define INODE_FREE_IN_DIR 19
#define ROOT_INODE_NOT_DIR 20
#define DIR_CANNOT_BE_REACHED_FROM_ROOT 21
#define UNDEFINED_FILE_SYSTEM_ERROR 99

#define SINGLE_INDIRECT 0
#define DOUBLE_INDIRECT 1

#define SECTOR_SIZE	512	/* disk sector size in bytes */
#define BLOCK_SIZE	4096	/* disk block size in bytes */
#define NICFREE		500	/* maximum number of free blocks in superblock */
#define NICINOD		500	/* number of free inodes in superblock */
#define IFMT		070000	/* type of file */
#define SPB        (BLOCK_SIZE / SECTOR_SIZE)
#define LINE_SIZE    100    /* input line buffer size in bytes */
#define LINES_PER_BATCH    32    /* number of lines output in one batch */

#define INOPB		64	/* number of inodes per block */
#define DIRPB		64	/* number of directory entries per block */
#define DIRSIZ		60	/* max length of path name component */

#define IFDIR		030000	/* directory */
#define IFCHR		020000	/* character special */
#define IFBLK		010000	/* block special */
#define INODE_BLOCKS_COUNT 8 /* count of block refs in inode */  
#define IFMT		070000	/* type of file */
#define IFREG		040000	/* regular file */
#define IFFREE	000000	/* reserved (indicates free inode) */

typedef unsigned int EOS32_ino_t;
typedef unsigned int EOS32_daddr_t;
typedef unsigned int EOS32_off_t;
typedef int EOS32_time_t;

typedef struct Block_Info {
    unsigned int file_occur;      // block occurrences in files 
    unsigned int free_list_occur; // block occurrences in free list 
} Block_Info;

typedef struct Inode_Info{
    unsigned int link_count;      // inode occurrence calculated 
    unsigned int parent;          // parent flag (for error message 21) 
} Inode_Info;

typedef struct Inode {
    unsigned int mode;                      // inode mode 
  	unsigned int nlink;                     // num of links to node 

  	EOS32_off_t size;                       // size of inode  
  	EOS32_daddr_t *refs;                    // addresses to blocks
} Inode;

typedef struct SuperBlock_Info {
    EOS32_daddr_t fsize;          // file system size in blocks
	EOS32_daddr_t isize;          // inode list size
	EOS32_daddr_t freeblks;       // free blocks size
	EOS32_ino_t freeinos;         // free inodes size
    EOS32_daddr_t *free_blocks;   // list with free blocks 

    unsigned int nfree;           // number of entries in free block list
} SuperBlock_Info;


int isFile(Inode *);
int isDir(Inode *);
int checkIllegalType(unsigned int);
void visitBlock(FILE *, EOS32_daddr_t , EOS32_daddr_t *, unsigned char);
void calculateInodeSize(FILE *, Inode *, EOS32_daddr_t *);
unsigned int get4Bytes(unsigned char *);
void checkInodeErrors(FILE *, Inode_Info *, SuperBlock_Info *);
unsigned char stepIntoInode(FILE *, EOS32_daddr_t, EOS32_daddr_t);
void stepIntoDirectoryBlock(FILE *, EOS32_daddr_t, EOS32_daddr_t, EOS32_daddr_t);
void readInode2(FILE *, Inode *, unsigned int);
void readInode(unsigned char *, Inode *);
void readSystemFiles(FILE *, SuperBlock_Info *);
void visitNode(FILE *, EOS32_daddr_t , EOS32_daddr_t );
void checkBlockInfos(SuperBlock_Info *, Block_Info *);
void indirectBlock(FILE *, EOS32_daddr_t, unsigned char);
void freeBlock(FILE *, EOS32_daddr_t *, EOS32_daddr_t);
void readFreeBlocks(FILE *, SuperBlock_Info *);
void readSuperBlock(unsigned char *, SuperBlock_Info *);
void readInodeTable(FILE *, SuperBlock_Info *);
void readInodeBlock(FILE *disk, EOS32_daddr_t, unsigned char *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);