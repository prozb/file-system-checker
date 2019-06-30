#include <stdio.h>

#define INCORRECT_START 1
#define IMAGE_NOT_FOUND 2
#define IO_ERROR 3
#define ILLEGAL_PARTITION 4
#define NO_FILE_SYSTEM 5
#define MEMORY_ALLOC_ERROR 6
#define RANDOM_ERROR 9
// Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
// Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
// Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
#define NEITHER_IN_FILE_OR_FREELIST 10
#define IN_FILE_IN_FREELIST 11
#define MULTIPLE_TIMES_FREELIST 12
#define BLOCK_DUPLICATE_DATA 13
#define DATA_SIZE_INCONSISTENT 14
#define INODE_LINK_COUNT_NULL_IN_DIR 15
#define INODE_LINK_COUNT_NULL_NOT_EMPTY 16
#define INODE_LINK_COUNT_APPEARANCE_FALSE 17
#define INODE_TYPE_FIELD_INVALID 18
#define INODE_FREE_IN_DIR 19
#define ROOT_INODE_NOT_DIR 20
#define DIR_CANNOT_BE_REACHED_FROM_ROOT 21
#define UNDEFINED_FILE_SYSTEM_ERROR 99

/* Exit-Codes ToDo:
    Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
    Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
    Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
    Ein Block ist mehr als einmal in einer Datei oder in mehr als einer Datei: Exit-Code 13.
    Die Groesse einer Datei ist nicht konsistent mit den im Inode vermerkten Bloecken: Exit-Code 14.
    Ein Inode mit Linkcount 0 erscheint in einem Verzeichnis: Exit-Code 15.
    Ein Inode mit Linkcount 0 ist nicht frei: Exit-Code 16.
    Ein Inode mit Linkcount n != 0 erscheint nicht in exakt n Verzeichnissen: Exit-Code 17.
    Ein Inode hat ein Typfeld mit illegalem Wert: Exit-Code 18.
    Ein Inode erscheint in einem Verzeichnis, ist aber frei: Exit-Code 19.
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

    */

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

#define   IFDIR		030000	/* directory */
#define   IFCHR		020000	/* character special */
#define   IFBLK		010000	/* block special */


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
    unsigned int link_actual;     // inode occurrence actual information
} Inode_Info;

typedef struct Inode {
    unsigned int mode;            // inode mode 
  	unsigned int nlink;           // num of links to node 

  	EOS32_off_t size;             // size of inode  
  	// EOS32_daddr_t addr;           // 
} Inode;

typedef struct SuperBlock_Info {
    EOS32_daddr_t fsize;          // file system size in blocks
	EOS32_daddr_t isize;          // inode list size
	EOS32_daddr_t freeblks;       // free blocks size
	EOS32_ino_t freeinos;         // free inodes size
    EOS32_daddr_t *free_blocks;   // list with free blocks 

    unsigned int nfree;           // number of entries in free block list
} SuperBlock_Info;

// checking inode directory
int isDir(Inode *);
unsigned int get4Bytes(unsigned char *);
void readInode(unsigned char *, Inode *);
void readSystemFiles(FILE *, SuperBlock_Info *);
void visitNode(FILE *, EOS32_daddr_t , EOS32_daddr_t );
void checkBLockInfos(SuperBlock_Info *, Block_Info *);
void indirectBlock(FILE *, EOS32_daddr_t, unsigned char);
void freeBlock(FILE *, EOS32_daddr_t *, EOS32_daddr_t);
void readFreeBlocks(FILE *, SuperBlock_Info *);
void readSuperBlock(unsigned char *, SuperBlock_Info *);
void readInodeTable(FILE *, SuperBlock_Info *);
void readInodeBlock(FILE *disk, EOS32_daddr_t, unsigned char *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);