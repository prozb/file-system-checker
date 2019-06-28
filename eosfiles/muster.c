#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define SECTOR_SIZE	512
#define BLOCK_SIZE	4096	/* disk block size in bytes */
#define SPB		(BLOCK_SIZE / SECTOR_SIZE)
#define INOPB		64	/* number of inodes per block */
#define DIRPB		64	/* number of directory entries per block */
#define DIRSIZ		60	/* max length of path name component */
#define NICINOD		500	/* number of free inodes in superblock */
#define NICFREE		500	/* number of free blocks in superblock */
#define IFMT		070000	/* type of file */
#define   IFREG		040000	/* regular file */
#define   IFDIR		030000	/* directory */
#define   IFCHR		020000	/* character special */
#define   IFBLK		010000	/* block special */
#define   IFFREE	000000	/* reserved (indicates free inode) */



// Typdeklaration
typedef struct {
  unsigned int cnodes;
  unsigned int cfree;
} BlockCount;

typedef struct {
  unsigned int cref;
  unsigned int root;
} InodeCount;

typedef unsigned int EOS32_daddr_t;
typedef unsigned int EOS32_ino_t;



// Methodenprototypen
void readBlock(EOS32_daddr_t blockNum, unsigned char *blockBuffer);
unsigned int inspectIndirectBlock(EOS32_daddr_t blockNum, int dindirect);
void inspectDirectoryBlock(EOS32_daddr_t blockNum);
void inspectFreelistLink(EOS32_daddr_t blockNum);
void inspectInode(EOS32_ino_t ino);


// Hilfsmethoden
unsigned int get4Bytes(unsigned char *p) {
  return (unsigned int) *(p + 0) << 24 |
         (unsigned int) *(p + 1) << 16 |
         (unsigned int) *(p + 2) <<  8 |
         (unsigned int) *(p + 3) <<  0;
}


// Variablen
BlockCount *blockTable;
InodeCount *inodeTable;
unsigned int fsStart;		/* file system start sector */
FILE *disk;





int main(int argc, char const *argv[]) {
  const char *diskName;
  char *endptr;
  unsigned char *ptptr;
  unsigned int partitionNumber;
  unsigned long fsSize;
  unsigned int numBlocks;
  unsigned int type;
  unsigned int nfree;
  unsigned int b;
  unsigned int i;
  unsigned char *p;
  unsigned char blockBuffer[BLOCK_SIZE];
  unsigned char partTable[SECTOR_SIZE];

  // EOS32_daddr_t fsize;
  EOS32_daddr_t isize;



  if (argc != 3) {
    printf("Usage: %s <disk image file> <partition number>\n", argv[0]);
    exit(1);
  }

  diskName = argv[1];
  // Test ob Partionsnummer valid ist
  partitionNumber = strtoul(argv[2], &endptr, 10);
  if(*endptr != '\0' || partitionNumber  < 0 || partitionNumber > 15){
    printf("Illegal Partionsnummer %s\n", argv[2]);
    exit(4);
  }

  // Open Disk
  disk = fopen(diskName, "rb");
  if (disk == NULL) {
    printf("cannot find disk image %s\n", diskName);
    exit(2);
  }

  // Partitionstabelle ansehen
  fseek(disk, 1 * SECTOR_SIZE, SEEK_SET);
  if (fread(partTable, 1, SECTOR_SIZE, disk) != SECTOR_SIZE) {
    printf("cannot read partition table of disk '%s'\n", argv[1]);
    exit(3);
  }
  ptptr = partTable + partitionNumber * 32;
  type = (get4Bytes(ptptr + 0) & 0x7FFFFFFF);
  if (type != 0x00000058) {
    printf("partition %d of disk '%s' does not contain an EOS32 file system\n", partitionNumber, argv[1]);
    exit(5);
  }

  fsStart = get4Bytes(ptptr + 4);
  fsSize = get4Bytes(ptptr + 8);

  if (fsSize % SPB != 0) {
    printf("File system size is not a multiple of block size.\n");
  }
  numBlocks = fsSize / SPB;

  if (numBlocks < 2) {
    printf("file system has less than 2 blocks\n");
    exit(9);
  }

  // printf("BlockCount: %d\n", numBlocks);

  blockTable = (BlockCount*) malloc(sizeof(BlockCount) * numBlocks);
  if(blockTable == NULL){
    printf("Malloc failed of BlockTable\n");
    exit(6);
  }

  //Superblock inpizieren
  readBlock(1, blockBuffer);
  p = blockBuffer;
  // MagicNumber überspringen
  p += 4;
  // fsize = get4Bytes(p);
  p += 4;
  // printf("file system size = %u (0x%X) blocks\n", fsize, fsize);
  isize = get4Bytes(p);
  // printf("inode list size  = %u (0x%X) blocks\n", isize, isize);

  inodeTable = (InodeCount*) malloc(sizeof(InodeCount) * INOPB * isize);
  if(inodeTable == NULL){
    printf("Malloc failed of BlockTable\n");
    exit(6);
  }






  // Alle Inodes Durchgehen
  int count = 0;
  for(b = 0; b < isize; b++){
    unsigned int size;
    readBlock(b + 2, blockBuffer);
    p = blockBuffer;
    for(i = 0; i < INOPB; i++){
      unsigned int mode;
      unsigned int type;
      unsigned int blocks = 0;
      EOS32_daddr_t addr;

      mode = get4Bytes(p);
      type = mode & IFMT;
      if(mode != 0){
        int j;

        count++;
        // printf("inode %d (%d) in block[%d]\n",i , (i + (b*INOPB)), b+2);
        p += 28;

        size = get4Bytes(p);
        p += 4;

        for (j = 0; j < 6; j++) {
          addr = get4Bytes(p);
          p += 4;
          // Special Files blocks ignore
          if (type != IFCHR && type != IFBLK) {
            if(addr > numBlocks){
              printf("XX  Inode %d direct block[%1d] is over the size blocks (%d) => %u (0x%X)\n", (i + (b*INOPB)), j, numBlocks, addr, addr);
              exit(99);
            }
            else if (addr != 0) {
              blockTable[addr].cnodes += 1;
              blocks++;
            }
          }
        }

        // Single Indirekt Block
        addr = get4Bytes(p);
        if(addr != 0){
          blockTable[addr].cnodes += 1;
          // printf("  single indirect = %u (0x%X)\n", addr, addr);
          blocks += inspectIndirectBlock(addr, 0);
        }

        // Double Indirekt Block
        p += 4;
        addr = get4Bytes(p);
        if(addr != 0){
          blockTable[addr].cnodes += 1;
          // printf("  double indirect = %u (0x%X)\n", addr, addr);
          blocks += inspectIndirectBlock(addr, 1);
        }
        p += 4;

        if((size !=0 && blocks!=0) && (size < ((blocks-1)*BLOCK_SIZE) || size > (blocks*BLOCK_SIZE))){
          printf("bottom: %d top:%d\n", ((blocks-1)*BLOCK_SIZE), (blocks*BLOCK_SIZE));
          printf("inode %d (%d) size(%d) are incosistent with number of blocks %d\n",i , (i + (b*INOPB)), size, blocks);
          exit(14);
        }


      }
      else {
        p += 64;
      }
    }
  }
  // printf("Number of Inodes %d\n", count);


  // Durchgang durch die Freiliste
  readBlock(1, blockBuffer);
  p = blockBuffer;

  p += 24; // Vor bis Cache für Inodes;
  p += NICINOD * 4; // Cache überspringen;

  nfree = get4Bytes(p);
  p += 4; // Vor bis Freiliste

  for (i = 0; i < NICFREE; i++) {
    EOS32_daddr_t free;
    free = get4Bytes(p);
    p += 4;
    // printf("  %s block[%3d] = %u (0x%X)\n", i == 0 ? "link" : "free", i, free, free);
    if (i < nfree) {
      if(i == 0 && free != 0){
        inspectFreelistLink(free);
      }
      blockTable[free].cfree += 1;

    }
  }

  for(i = 2 + isize; i < numBlocks; i++){
    unsigned int cnodes = blockTable[i].cnodes;
    unsigned int cfree = blockTable[i].cfree;

    if(cnodes == 0 && cfree == 0){
      printf("Block[%d] is not in inode or freelist;  cinode: %d;  cfree: %d\n", i, cnodes, cfree);
      exit(10);
    }
    else if (cnodes == 1 && cfree == 1){
      printf("Block[%d] is in inode and freelist;  cinode: %d;  cfree: %d\n", i, cnodes, cfree);
      exit(11);
    }
    else if(cfree > 1){
      printf("Block[%d] is multiple times in freelist;  cinode: %d;  cfree: %d\n", i, cnodes, cfree);
      exit(12);
    }
    else if(cnodes > 1){
      printf("Block[%d] is multiple times in inodes;  cinode: %d;  cfree: %d\n", i, cnodes, cfree);
      exit(13);
    }

  }

  // Root Inode betreten und recursiv alle Verzeichnissse besuchen
  inspectInode(1);



  for(b = 0; b < isize; b++){
    readBlock(b + 2, blockBuffer);
    p = blockBuffer;
    for(i = 0; i < INOPB; i++){
      unsigned int mode;
      unsigned int nlink;
      EOS32_ino_t ino = (i + (b*INOPB));
      InodeCount ic = inodeTable[ino];

      if(ino != 0){
        mode = get4Bytes(p);
        p += 4;
        if (mode != 0) {
          unsigned int type = mode & IFMT;
          if (type != IFREG && type != IFDIR && type != IFCHR && type != IFBLK) {
            printf("inode %d has illegal type value 0x%08X \n", i, mode);
            exit(18);
          }

          if (type == IFDIR && ic.root==0){
            printf("Inode %d is dir, but it is not reachable of root \n", ino);
            exit(21);
          }
        }
        else if (mode == 0 && ic.root==1){
          printf("Inode %d is free but it appear in a dir\n", ino);
          exit(19);
        }

        // if(ino == 692){
        //    printf("root = %d and cref %d\n",ic.root, ic.cref );
        // }

        nlink = get4Bytes(p);
        p += 4;
        // printf("  nlnk = %u (0x%08X)\n", nlink, nlink);

        if(nlink == 0){
          if(ic.cref != 0){
            printf("Inode %u has linkcount 0 but appears in a dir %u\n", ino, ic.cref);
            exit(15);
          }

          if(mode != 0){
            printf("Inode %d has linkcount 0 but is not free \n", ino);
            exit(16);
          }
        }

        if(nlink != ic.cref){
          printf("Inode %d has linkcount %d but it appears %d times in directories\n", ino, nlink, ic.cref);
          exit(17);
        }

        p += 56;
      }
      else {
        p += 64;
      }

    }
  }

  printf("Successful finished Filecheck\n");
  return 0;
}

void inspectInode(EOS32_ino_t ino){
  EOS32_daddr_t block =  ino / INOPB + 2;
  EOS32_ino_t inoBlock = ino % INOPB;
  unsigned char buffer[BLOCK_SIZE];
  unsigned char *p;

  readBlock(block, buffer);
  p = buffer;

  // skip to Inode in block
  p += 64 * inoBlock;
  unsigned int mode = get4Bytes(p);
  p += 32;

  if(ino == 1 && (mode == 0 || (mode & IFMT) != IFDIR)){
    printf("Root Inode is not a dir\n");
    exit(20);
  }

  if(mode != 0){
    if ((mode & IFMT) == IFDIR) {
      EOS32_daddr_t addr;
      int j;
      // TODO ROOT Verzeichnis durchgehen
      for (j = 0; j < 6; j++) {
        addr = get4Bytes(p);
        p += 4;

        if (addr != 0) {
          inspectDirectoryBlock(addr);
          // blockTable[addr].cnodes += 1;
        }
      }

      // Single Indirekt Block
      addr = get4Bytes(p);
      if(addr != 0){
        // printf("  single indirect = %u (0x%X)\n", addr, addr);
        inspectIndirectBlock(addr, 0);
      }

      // Double Indirekt Block
      p += 4;
      addr = get4Bytes(p);
      if(addr != 0){
        // printf("  double indirect = %u (0x%X)\n", addr, addr);
        inspectIndirectBlock(addr, 1);
      }
      p += 4;
    }

  }

  else {
    printf("Inode %d is empty but inside a dir\n", ino);
    exit(19);
  }
}

void inspectDirectoryBlock(EOS32_daddr_t blockNum){
  unsigned char buffer[BLOCK_SIZE];
  unsigned char *p;
  EOS32_ino_t ino;
  int i;
  // printf("inspect directory block %d\n", blockNum);
  readBlock(blockNum, buffer);
  p = buffer;

  for (i = 0; i < DIRPB; i++) {
    // printf("%02d:  ", i);
    ino = get4Bytes(p);
    p += 4;

    inodeTable[ino].cref += 1;
    inodeTable[ino].root = 1;

    if(i > 1){
      inspectInode(ino);
    }

    p += DIRSIZ;
  }
}

void inspectFreelistLink(EOS32_daddr_t blockNum){
  unsigned char buffer[BLOCK_SIZE];
  unsigned int nfree;
  unsigned char *p;
  int i;
  EOS32_daddr_t free;
  // printf("inspectFreelistLink: %d\n", blockNum);
  readBlock(blockNum, buffer);
  p = buffer;

  nfree = get4Bytes(p);
  p += 4;
  for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
    free = get4Bytes(p);
    p += 4;

    if (i < nfree) {
      if(i == 0 && free != 0){
        inspectFreelistLink(free);
      }
      blockTable[free].cfree += 1;
      // printf("  %s block[%3d] = %u (0x%X)\n", i == 0 ? "link" : "free", i, free, free);
    }
  }
}

void inspectInodeIndirect(EOS32_daddr_t blockNum, int dindirect){
  unsigned char buffer[BLOCK_SIZE];
  unsigned char *p;
  EOS32_daddr_t addr;
  int i;

  readBlock(blockNum, buffer);
  p = buffer;
  if(dindirect == 0){
    for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
      addr = get4Bytes(p);
      p += 4;
      if (addr != 0){
        inspectDirectoryBlock(addr);
      }
    }
  }
  else {
    for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
      addr = get4Bytes(p);
      p += 4;
      if (addr != 0){
        inspectInodeIndirect(addr, 0);
      }
    }
  }
}

unsigned int inspectIndirectBlock(EOS32_daddr_t blockNum, int dindirect){
  unsigned char buffer[BLOCK_SIZE];
  unsigned char *p;
  unsigned int blocks;
  EOS32_daddr_t addr;
  int i;

  blocks = 0;
  readBlock(blockNum, buffer);
  p = buffer;
  if(dindirect == 0){
    for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
      addr = get4Bytes(p);
      p += 4;
      if (addr != 0){
        blockTable[addr].cnodes += 1;
        blocks++;
        // printf("block[%4d] = %u (0x%X)\n", i, addr, addr);
      }
    }
  }
  else {
    for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
      addr = get4Bytes(p);
      p += 4;
      if (addr != 0){
        blockTable[addr].cnodes += 1;
        blocks++;
        // printf("block[%4d] = %u (0x%X)\n", i, addr, addr);
        blocks += inspectIndirectBlock(addr, 0);
      }
    }
  }
  return blocks;
}

void readBlock(EOS32_daddr_t blockNum, unsigned char *blockBuffer) {
  fseek(disk, fsStart * SECTOR_SIZE + blockNum * BLOCK_SIZE, SEEK_SET);
  if (fread(blockBuffer, BLOCK_SIZE, 1, disk) != 1) {
    printf("cannot read block %d (0x%x)", blockNum, blockNum);
    exit(3);
  }
}
