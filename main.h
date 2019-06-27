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

typedef unsigned int EOS32_ino_t;
typedef unsigned int EOS32_daddr_t;
typedef unsigned int EOS32_off_t;
typedef int EOS32_time_t;

void allocateFreeBlock(unsigned char *, EOS32_daddr_t *);
void readBlock(FILE *, EOS32_daddr_t, unsigned char *);
unsigned int get4Bytes(unsigned char *);
