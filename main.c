#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// a) Falscher Aufruf des Programms: Exit-Code 1. 
// b) Image-Datei nicht gefunden: Exit-Code 2. 
// c) Datei Ein/Ausgabefehler: Exit-Code 3. 
// d) Illegale Partitionsnummer: Exit-Code 4. 
// e) Partition enthaelt kein EOS32-Dateisystem: Exit-Code 5. 
// f) Erfolgloser Aufruf von malloc(): Exit-Code 6. 
// g) Alle anderen Fehler: Exit-Code 9

#define INCORRECT_START 1
#define IMAGE_NOT_FOUND 2
#define FILE_IO_ERROR 3
#define ILLEGAL_PARTITION 4
#define NO_FILE_SYSTEM 5
#define MEMORY_ALLOC_ERROR 6
#define RANDOM_ERROR 9

int main(int argc, char *argv[]){
	if(argc < 3){
		exit(INCORRECT_START);
	}

	if(access(argv[1], F_OK) == -1){
		exit(IMAGE_NOT_FOUND);
	}

	
	return 0;
}
