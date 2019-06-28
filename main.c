
// ----- INPUT -----
/* Zwei Shell-Parameter:
 * - Dateiname der EOS32 IMG Datei
 * - Partitionsnummer
 */

// ----- Logik -----
/* - Ausführen eines Filesytem-Check auf der Partitionsnummer
 * - Erkennen von Fehlersituationen im Dateisystem
 * - Ausgabe von Fehlermeldung bei auftreten einer Fehlersituation und abbruch mit entsprechendem EXIT-Code
 * - Prinzipiell sind zu ueberpruefen: Bloecke, Dateien (d.h. Inodes), Verzeichnisse.
 */

// ----- EXIT-Codes ------
/*
 *  a) Ein Block ist weder in einer Datei noch auf der Freiliste: Exit-Code 10.
    b) Ein Block ist sowohl in einer Datei als auch auf der Freiliste: Exit-Code 11.
    c) Ein Block ist mehr als einmal in der Freiliste: Exit-Code 12.
    d) Ein Block ist mehr als einmal in einer Datei oder in mehr als einer Datei: Exit-Code 13.
    e) Die Groesse einer Datei ist nicht konsistent mit den im Inode vermerkten Bloecken: Exit-Code 14.
    f) Ein Inode mit Linkcount 0 erscheint in einem Verzeichnis: Exit-Code 15.
    g) Ein Inode mit Linkcount 0 ist nicht frei: Exit-Code 16.
    h) Ein Inode mit Linkcount n != 0 erscheint nicht in exakt n Verzeichnissen: Exit-Code 17.
    i) Ein Inode hat ein Typfeld mit illegalem Wert: Exit-Code 18.
    j) Ein Inode erscheint in einem Verzeichnis, ist aber frei: Exit-Code 19.
    k) Der Root-Inode ist kein Verzeichnis: Exit-Code 20.
    l) Ein Verzeichnis kann von der Wurzel aus nicht erreicht werden: Exit-Code 21.
    m) Alle anderen Dateisystem-Fehler: Exit-Code 99.

    Andere moegliche Fehler, die geprueft werden muessen:
    a) Falscher Aufruf des Programms: Exit-Code 1. // DONE
    b) Image-Datei nicht gefunden: Exit-Code 2. // DONE
    c) Datei Ein/Ausgabefehler: Exit-Code 3.
    d) Illegale Partitionsnummer: Exit-Code 4. // DONE
    e) Partition enthaelt kein EOS32-Dateisystem: Exit-Code 5.
    f) Erfolgloser Aufruf von malloc(): Exit-Code 6.
    g) Alle anderen Fehler: Exit-Code 9.
 */

// --------- HINWEISE --------
/*
    1. Sie duerfen die Hilfsfunktionen zum Lesen des Dateisystems aus den Programmen "shpart" und "EOS32-shfs" verwenden, ohne sich dem Vorwurf eines Plagiats auszusetzen.
    2. Den Startsektor der in der Kommandozeile angegebenen Partition entnehmen Sie der Partitionstabelle der Platte. Pruefen Sie, ob eine Partition angegeben wurde, auf der sich auch tatsaechlich ein EOS32-Dateisystem befindet!
    3. Ueberlegen Sie sich Datenstrukturen, um moeglichst effizient viele der o.a. Fehlersituationen in moeglichst wenigen Durchgaengen zu entdecken. Vorschlag (den Sie unbedingt aufgreifen sollten):
    a) (fuer Bloecke) Tabelle mit zwei Zaehlern pro Block; einer zaehlt Vorkommen in Dateien, der andere Vorkommen in der Freiliste. Es wird ein Durchgang durch alle Inodes gemacht, anschliessend die Freiliste inspiziert. Am Ende muss jeder Block genau eine "1" in einem der beiden Zaehlern haben.
    b) (fuer Verzeichnisse) Tabelle mit einem Zaehler pro Inode. Es wird ein rekursiver Durchgang durch alle Verzeichnisse gemacht und dabei die Anzahl der Referenzen auf jede Datei gezaehlt. Am Ende wird mit einem Durchgang durch die Inodes geprueft, ob der Linkcount in den Inodes stimmt.
    4. Die Idee, die gesamte Platte in Datenstrukturen in den Hauptspeicher zu laden und danach den Dateisystem-Check dort durchzufuehren, ist angesichts der Groesse moderner Platten undurchfuehrbar. Denken Sie daran, dass die Dateisysteme, mit denen die Akzeptanztests fuer Ihre Hausuebung durchgefuehrt werden, ganz andere Groessen haben koennen als das im Praktikum benutzte Dateisystem. Das gilt sowohl fuer die Gesamtanzahl der Bloecke als auch fuer die Anzahl der Inode-Bloecke (und natuerlich auch fuer die auf der Platte gespeicherten Dateien).
    5. Achten Sie auf Character- und Block-Special-Files! Deren "Blocknummer" fuer den ersten direkten Block ist keine reale Blocknummer, sondern die Haupt/Neben-Geraetenummer, die den Treiber fuer dieses Geraet auswaehlt.
    6. Testen Sie Ihr Programm gruendlich, indem Sie gezielt Bytes (bzw. Halbworte oder Worte) im Disk-Image "umschiessen". Sie koennen dazu dieses Programm benutzen.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct {

} BlockSize;

typedef struct {

} InodeSize;


/*
 * Format-Function for Error-Messages
 */
void error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    printf("Error: ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    exit(1);
}



int main(int argc, char *argv[]) {
    char *filename;
    char *endptr;
    char eos[] = "PLACEHOLDER";
    //char partition[] = "PLACEHOLDER";
    FILE *disk = NULL;
    int partition;

    /* Stops the Shell if there is no input */
    if (argc == 1) {
        printf("Usage: ./hu2 <EOS-IMG-FILE> [partition-number]\n");
        exit(1); // Exit-Code Number 1

    }
    /* Checks if the input is a valid disk image */
    disk = fopen(argv[0], "rb");
    if (disk == NULL) {
        error("cannot open disk image file '%s'", argv[1]);
    }
    /* Interprets the disk image and partition number */
    else if (argc == 2) {
        filename = argv[1];
        partition = (strtoul(argv[2], &endptr, 10));
        if (*endptr != '\0' || partition < 0 || partition > 15) {
            error("illegal partition number '%s'", argv[2]);
            exit(4); // Exit-Code Number 4
        }
        disk = fopen(filename, "rb");
        if (disk == NULL) {
            error("cannot find disk image %s\n", argv[1]);
            exit(2); // Exit-Code Number 2
        }
    }

}