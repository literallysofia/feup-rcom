#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define SET_C 0x03
#define SET_BCC A ^ SET_C
#define UA_C 0x07
#define UA_BCC A ^ UA_C
#define C10 0x00
#define C11 0x40
#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D
#define RR_C0 0x05
#define RR_C1 0x85
#define REJ_C0 0x01
#define REJ_C1 0x81
#define DISC_C 0x0B
#define C2End 0x03

/*
* Lê trama de controlo SET e, de seguida, manda trama UA
* Data link layer
*/
void LLOPEN(int fd);

/*
* Lê uma mensagem e faz destuffing
* Data link layer
*/
unsigned char *LLREAD(int fd, int *sizeMessage);

/*
* Obtem o nome do ficheiro a ser passado
* Application layer
*/
unsigned char *nameOfFileFromStart(unsigned char *start);

/*
* Obtem o tamanho do ficheiro
* Application layer
*/
off_t sizeOfFileFromStart(unsigned char *start);

/*
* Verifica se o BCC2 recebido na mensagem esta certo, ou seja, se o xor de todos os bytes da mensagem
* e igual ao BCC2 recebido
* Data link layer
*/
int checkBCC2(unsigned char *message, int sizeMessage);

/*
* Ciclo de leitura que fica a espera de ler uma trama de controlo em que C seja igual ao c recebido
* Application layer
*/
int readControlMessage(int fd, unsigned char C);

/*
* Manda uma qualquer trama de controlo, sendo o C recebido a diferenca
* Application layer
*/
void sendControlMessage(int fd, unsigned char C);

/*
* Remove o cabecalho colocado pela application layer nas tramas I
* Application layer
*/
unsigned char *removeHeader(unsigned char *toRemove, int sizeToRemove, int *sizeRemoved);

/*
* Verifica se a trama recebida e a trama END(igual a trama START mas com valor de C2 diferente)
* Application layer
*/
int isEndMessage(unsigned char *start, int sizeStart, unsigned char *end, int sizeEnd);

/*
* Cria um ficheiro com os dados recebidos do ficheiro original e com o mesmo nome
* Application layer
*/
void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char *filename);

/*
* Mecanismo de terminacao
* Data link Layer
*/
void LLCLOSE(int fd);
