#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define SET_C 0x03
#define UA_C 0x07
#define UA_BCC (A^UA_C)
#define SET_BCC (A^SET_C)
#define NUMMAX 3
#define C10 0x00
#define C11 0x40
#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D
#define C2Start 0x02
#define C2End 0x03
#define CRR0 0x05
#define CRR1 0x85
#define CREJ0 0x01
#define CREJ1 0x81
#define T1 0x00
#define T2 0x01
#define L1 0x04
#define L2 0x0B
#define DISC 0x0B
#define headerC 0x01
#define sizePacketConst 53

/*
* Manda trama SET para o recetor, para "avisar" que vai comecar a mandar dados
* Data link layer
*/
int LLOPEN(int fd, int x);

/*
* Faz stuffing de uma mensagem e manda-a para o recetor
* Data link layer
*/
int LLWRITE(int fd, unsigned char* mensagem, int size);

/*
* Calcula o valor do BCC2 de uma mensagem
* Data link Layer
*/
unsigned char calculoBCC2(unsigned char* mensagem, int size);

/*
* Manda uma qualquer trama de controlo, sendo o C recebido a diferenca
* Application layer
*/
void sendControlMessage(int fd, unsigned char C);

/*
* Stuffing do BCC2
* Data link layer
*/
unsigned char* stuffingBCC2(unsigned char BCC2,int* sizeBCC2);

/*
* Manda os pacotes de controlo I START e END
* Application layer
*/
unsigned char* controlPackageI(unsigned char state, off_t sizeFile, unsigned char* fileName, int sizeOfFilename, int* sizeControlPackageI);

/*
* Le uma qualquer trama de controlo
* Data link layer
*/
unsigned char readControlMessage(int fd);

/*
* Abre um ficheiro e le o seu conteudo
* Application layer
*/
unsigned char* openReadFile(unsigned char* fileName, off_t* sizeFile);

/*
* Acrescenta cabecalho das tramas I
* Application layer
*/
unsigned char* headerAL(unsigned char* mensagem, off_t sizeFile, int * sizePacket);

/*
* Divide uma mensagem proveniente do ficheiro em packets
* Application layer
*/
unsigned char* splitMessage(unsigned char* mensagem,off_t* indice,int* sizePacket, off_t sizeFile);

/*
* Verifica se o UA foi recebido (com alarme)
* Data link layer
*/
void stateMachineUA(int* state, unsigned char* c);

/*
* Mecanismo de terminacao
* Data link layer
*/
void LLCLOSE(int fd);


unsigned char * messUpBCC2(unsigned char* packet, int sizePacket);

unsigned char * messUpBCC1(unsigned char* packet, int sizePacket);
