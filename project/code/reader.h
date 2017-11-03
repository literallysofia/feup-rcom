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
#define SET_BCC A ^ SET_C
#define UA_BCC A ^ UA_C
#define SET_C 0x03
#define UA_C 0x07
#define C10 0x00
#define C11 0x40
#define RR_C0 0x05
#define RR_C1 0x85
#define REJ_C0 0x01
#define REJ_C1 0x81
#define DISC_C 0x0B
#define C2End 0x03

#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D

/*--------------------------Data Link Layer --------------------------*/

/*
* Lê trama de controlo SET e envia a trama UA.
* Data link layer
*/
void LLOPEN(int fd);

/*
* Lê tramas I e faz destuffing.
* Data link layer
*/
unsigned char *LLREAD(int fd, int *sizeMessage);

/*
* Lê trama de controlo DISC, envia DISC de volta e recebe UA.
* Data link Layer
*/
void LLCLOSE(int fd);

/*
* Ciclo de leitura que quebra após ler uma trama de controlo C 
* que seja igual ao C recebido como argumento da função.
* Data link layer
*/
int readControlMessage(int fd, unsigned char C);

/*
* Envia uma trama de controlo, sendo o C recebido como argumento da função a diferença de cada trama enviada.
* Data link layer layer
*/
void sendControlMessage(int fd, unsigned char C);

/*
* Verifica se o BCC2 recebido na mensagem está correto.
* Data link layer
*/
int checkBCC2(unsigned char *message, int sizeMessage);


/*--------------------------Application Link Layer --------------------------*/

/*
* Base da camada de aplicação pois é esta que controla todo o processo 
* que ocorre nesta camada e que faz as chamadas às funções da camada 
* de ligação.
* Application layer
*/
int main(int argc, char **argv);


/*
* Obtém nome do ficheiro a partir da trama START.
* Application layer
*/
unsigned char *nameOfFileFromStart(unsigned char *start);

/*
* Obtém tamanho do ficheiro a partir da trama START.
* Application layer
*/
off_t sizeOfFileFromStart(unsigned char *start);

/*
* Remove o cabeçalho do nível de aplicação das tramas I.
* Application layer
*/
unsigned char *removeHeader(unsigned char *toRemove, int sizeToRemove, int *sizeRemoved);

/*
* Verifica se a trama recebida e a trama END.
* Application layer
*/
int isEndMessage(unsigned char *start, int sizeStart, unsigned char *end, int sizeEnd);

/*
* Cria ficheiro com os dados recebidos nas tramas I.
* Application layer
*/
void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char *filename);

