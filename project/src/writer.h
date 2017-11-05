#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define NUMMAX 3
#define TIMEOUT 3
#define sizePacketConst 100
#define bcc1ErrorPercentage 0
#define bcc2ErrorPercentage 0

#define FLAG 0x7E
#define A 0x03
#define SET_BCC (A ^ SET_C)
#define UA_BCC (A ^ UA_C)
#define UA_C 0x07
#define SET_C 0x03
#define C10 0x00
#define C11 0x40
#define C2Start 0x02
#define C2End 0x03
#define CRR0 0x05
#define CRR1 0x85
#define CREJ0 0x01
#define CREJ1 0x81
#define DISC 0x0B
#define headerC 0x01

#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D

#define T1 0x00
#define T2 0x01
#define L1 0x04
#define L2 0x0B


/*--------------------------Data Link Layer --------------------------*/

/*
*Envia trama de supervisão SET e recebe trama UA.
*Data link Layer
*/
int LLOPEN(int fd, int x);

/*
* Realiza stuffing das tramas I e envia-as.
* Data link layer
*/
int LLWRITE(int fd, unsigned char *mensagem, int size);

/*
* Envia trama de supervisão DISC, recebe DISC e envia UA.
* Data link layer
*/
void LLCLOSE(int fd);

/*
* Verifica se o UA foi recebido (com alarme).
* Data link layer
*/
void stateMachineUA(int *state, unsigned char *c);

/*
* Espera por uma trama de supervisão e retorna o seu C.
* Data link layer
*/
unsigned char readControlMessageC(int fd);

/*
* Envia uma trama de supervisão, sendo o C recebido como argumento 
* da função a diferença de cada trama enviada.
* Data link layer
*/
void sendControlMessage(int fd, unsigned char C);

/*
* Calcula o valor do BCC2 de uma mensagem.
* Data link Layer
*/
unsigned char calculoBCC2(unsigned char *mensagem, int size);

/*
* realiza o stuffing do BCC2.
* Data link layer
*/
unsigned char *stuffingBCC2(unsigned char BCC2, int *sizeBCC2);

/*
* Geração aleatória de erros no BCC1.
* Data link layer
*/
unsigned char *messUpBCC1(unsigned char *packet, int sizePacket);

/*
* Geração aleatória de erros no BCC2.
* Data link layer
*/
unsigned char *messUpBCC2(unsigned char *packet, int sizePacket);

/*--------------------------Application Link Layer --------------------------*/

/*
* Base da camada de aplicação pois é esta que controla todo o processo 
* que ocorre nesta camada e que faz as chamadas às funções da camada 
* de ligação.
* Application layer
*/
int main(int argc, char **argv);

/*
* Cria os pacotes de controlo START e END.
* Application layer
*/
unsigned char *controlPackageI(unsigned char state, off_t sizeFile, unsigned char *fileName, int sizeOfFilename, int *sizeControlPackageI);


/*
* Abre um ficheiro e le o seu conteúdo.
* Application layer
*/
unsigned char *openReadFile(unsigned char *fileName, off_t *sizeFile);

/*
* Acrescenta o cabeçalho do nível de aplicação às tramas.
* Application layer
*/
unsigned char *headerAL(unsigned char *mensagem, off_t sizeFile, int *sizePacket);

/*
* Divide uma mensagem proveniente do ficheiro em packets.
* Application layer
*/
unsigned char *splitMessage(unsigned char *mensagem, off_t *indice, int *sizePacket, off_t sizeFile);


