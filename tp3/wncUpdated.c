/*Non-Canonical Input Processing*/

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
#define NUMMAX 4
#define C10 0x00
#define C11 0x40
#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D
#define C2Start 0x02
#define C2End 0x03


volatile int STOP=FALSE;

void LLOPEN(int fd, int x);

void LLWRITE(int fd, unsigned char* mensagem, int size);

unsigned char calculoBCC2(unsigned char* mensagem, int size);

void sendControlMessage(int fd, unsigned char C);

unsigned char* stuffingBCC2(unsigned char BCC2,int* sizeBCC2);

FILE* openFile(unsigned char* fileName, unsigned int* sizeFile);

unsigned char* controlPackageI(unsigned char state, unsigned int* sizeFile, unsigned char* fileName);

int sumAlarms=0;
int flagAlarm = FALSE;
int trama = 0;
int paragem = FALSE;

//handler do sinal de alarme
void alarmHandler(){
        printf("Alarm=%d\n", sumAlarms);
        flagAlarm=TRUE;
        sumAlarms++;
}

int main(int argc, unsigned char** argv)
{
        int fd, c, res;
        struct termios oldtio,newtio;
        char buf[255];
        int i, sum = 0, speed = 0;
        FILE* f;
        unsigned int sizeFile;

        if ( (argc < 3) ||
             ((strcmp("/dev/ttyS0", argv[1])!=0) &&
              (strcmp("/dev/ttyS1", argv[1])!=0) )) {
                printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
                exit(1);
        }
        /*
           Open serial port device for reading and writing and not as controlling tty
           because we don't want to get killed if linenoise sends CTRL-C.
         */
        fd = open(argv[1], O_RDWR | O_NOCTTY );
        if (fd <0) {perror(argv[1]); exit(-1); }

        f = openFile(argv[2], &sizeFile);



        if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
                perror("tcgetattr");
                exit(-1);
        }

        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        /* set input mode (non-canonical, no echo,...) */
        newtio.c_lflag = 0;

        newtio.c_cc[VTIME]    = 1;/* inter-unsigned character timer unused */
        newtio.c_cc[VMIN]     = 0;/* blocking read until 5 unsigned chars received */

        /*
           VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
           leitura do(s) pr�ximo(s) caracter(es)
         */

        tcflush(fd, TCIOFLUSH);

        if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
                perror("tcsetattr");
                exit(-1);
        }

        printf("New termios structure set\n");

        // instalar handler do alarme
        (void)signal(SIGALRM,alarmHandler);

        //mensagem a enviar
        unsigned char mensagem[] = "Hello World";
        LLOPEN(fd,0);
        LLWRITE(fd,mensagem,sizeof(mensagem));

        sleep(1);
        if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
                perror("tcsetattr");
                exit(-1);
        }

        close(fd);
        return 0;
}

void stateMachineUA(int* state, unsigned char* c){

        switch(*state) {
        //recebe flag
        case 0:
                if(*c==FLAG)
                        *state=1;
                break;
        //recebe A
        case 1:
                if(*c==A)
                        *state=2;
                else
                {
                        if(*c==FLAG)
                                *state=1;
                        else
                                *state = 0;
                }
                break;
        //recebe C
        case 2:
                if(*c==UA_C)
                        *state=3;
                else{
                        if(*c==FLAG)
                                *state=1;
                        else
                                *state = 0;
                }
                break;
        //recebe BCC
        case 3:
                if(*c == UA_BCC)
                        *state = 4;
                else
                        *state=0;
                break;
        //recebe FLAG final
        case 4:
                if(*c==FLAG) {
                        paragem=TRUE;
                        alarm(0);
                        printf("Recebeu UA\n");
                }
                else
                        *state = 0;
                break;
        }

}

//manda a trama SET através da porta série para o recetor, para "avisar" que vai começar a mandar dados
void LLOPEN(int fd, int x){

        unsigned char c;
        do {
                sendControlMessage(fd,SET_C);
                printf("Enviou SET\n");
                alarm(3);
                flagAlarm=0;
                int state=0;

                while(!paragem && !flagAlarm) {
                        read(fd,&c,1);
                        stateMachineUA(&state, &c);
                }
        } while(flagAlarm && sumAlarms < NUMMAX);

}

//faz stuffing de uma mensagem e, de seguida, manda-a através da porta série
void LLWRITE(int fd, unsigned char* mensagem, int size){

        unsigned char BCC2;
        unsigned char* BCC2Stuffed = (unsigned char*)malloc(sizeof(unsigned char));
        unsigned char* mensagemFinal = (unsigned char*)malloc((size+6)*sizeof(unsigned char));
        int sizeMensagemFinal = size + 6;
        int sizeBCC2=1;
        BCC2 = calculoBCC2(mensagem,size);
        BCC2Stuffed = stuffingBCC2(BCC2,&sizeBCC2);

        mensagemFinal[0]= FLAG;
        mensagemFinal[1]=A;
        if(trama == 0) {
                mensagemFinal[2]=C10;
                trama=1;
        } else{
                mensagemFinal[2]=C11;
                trama=0;
        }
        mensagemFinal[3] = (mensagemFinal[1]^mensagemFinal[2]);

        int i=0;
        int j=4;
        for(; i < size; i++) {
                if(mensagem[i] == FLAG) {
                        mensagemFinal = (unsigned char*)realloc(mensagemFinal, ++sizeMensagemFinal);
                        mensagemFinal[j]=Escape;
                        mensagemFinal[j+1]=escapeFlag;
                        j = j+2;
                }
                else{
                        if(mensagem[i]==Escape) {
                                mensagemFinal = (unsigned char*)realloc(mensagemFinal, ++sizeMensagemFinal);
                                mensagemFinal[j]=Escape;
                                mensagemFinal[j+1]=escapeEscape;
                                j = j+2;
                        }
                        else {
                                mensagemFinal[j] = mensagem[i];
                                j++;
                        }
                }
        }

        if(sizeBCC2 == 1)
                mensagemFinal[j]=BCC2;
        else{
                mensagemFinal = (unsigned char*)realloc(mensagemFinal, ++sizeMensagemFinal);
                mensagemFinal[j]=BCC2Stuffed[0];
                mensagemFinal[j+1]=BCC2Stuffed[1];
                j++;
        }
        mensagemFinal[j+1]=FLAG;

        //printf("mensagem final\n");
        /*i =0;
           for(; i < sizeMensagemFinal;i++){
           printf("%x\n",mensagemFinal[i] );
           }*/

        //mandar mensagem
        int res;
        res = write(fd,mensagemFinal,sizeMensagemFinal);
        if(res!=0) printf("Enviou Mensagem\n");
}

//manda uma qualquer trama de controlo
void sendControlMessage(int fd, unsigned char C){
        unsigned char message[5];
        message[0]=FLAG;
        message[1]=A;
        message[2]=C;
        message[3]=message[1]^message[2];
        message[4]=FLAG;
        /*printf("%x\n", message[0]);
           printf("%x\n", message[1]);
           printf("%x\n", message[2]);
           printf("%x\n", message[3]);
           printf("%x\n", message[4]);*/
        write(fd,message,5);
}

//lê uma qualquer trama de controlo
int readControlMessage(int fd, unsigned char C){
        int state=0;
        unsigned char c;

        while(state != 5) {
                read(fd,&c,1);
                switch(state) {
                //recebe FLAG
                case 0:
                        //print("0\n");
                        if(c==FLAG)
                                state=1;
                        break;
                //recebe A
                case 1:
                        //printf("1\n");
                        if(c==A)
                                state=2;
                        else{
                                if(c==FLAG)
                                        state=1;
                                else
                                        state=0;
                        }
                        break;
                //recebe c
                case 2:
                        //printf("2\n");
                        if(c==C)
                                state=3;
                        else{
                                if(c==FLAG)
                                        state=1;
                                else
                                        state=0;
                        }
                        break;
                //recebe BCC
                case 3:
                        //printf("3\n");
                        if(c==(A^C))
                                state=4;
                        else
                                state=0;
                        break;
                //recebe FLAG final
                case 4:
                        //printf("4\n");
                        if(c==FLAG)
                                //printf("recebeu mensagem controlo\n");
                                state=5;
                        else
                                state=0;
                        break;
                }
                return TRUE;
        }
}


//calculo do BCC2 de uma mensagem
unsigned char calculoBCC2(unsigned char* mensagem, int size){
        unsigned char BCC2 = mensagem[0];
        int i;
        for(i=1; i < size; i++) {
                BCC2^=mensagem[i];
        }
        return BCC2;
}

unsigned char* stuffingBCC2(unsigned char BCC2, int* sizeBCC2){

        if(BCC2 == FLAG) {
                unsigned char* BCC2Stuffed= (unsigned char*)malloc(2*sizeof(unsigned char*));
                BCC2Stuffed[0]=Escape;
                BCC2Stuffed[1]=escapeFlag;
                (*sizeBCC2)++;
        }
        else{
                if(BCC2==Escape) {
                        unsigned char* BCC2Stuffed= (unsigned char*)malloc(2*sizeof(unsigned char*));
                        BCC2Stuffed[0]=Escape;
                        BCC2Stuffed[1]=escapeEscape;
                        (*sizeBCC2)++;
                }
        }

        return BCC2Stuffed;
}



FILE* openFile(unsigned char* fileName, unsigned int* sizeFile){
        FILE* f;
        struct stat metadata;

        if((f = fopen(fileName, "rb"))== NULL) {
                perror("error opening file!");
                exit(-1);
        }
        stat(fileName,&metadata);
        (*sizeFile) = metadata.st_size;
        printf("This file has %d bytes \n",sizeFile);

        return f;
}

unsigned char* controlPackageI(unsigned char state, unsigned int* sizeFile, unsigned char* fileName){

        unsigned char L1 = sizeof(unsigned int);
        unsigned char L2 = strlen(fileName);
        unsigned char* package = (unsigned char*)malloc(5 + L1 + L2);
        if(state==C2Start)
                package[0]=C2Start;
        else
                package[0]=C2End;
        package[1]=0;
        package[2]=L1;
        package[3]= (unsigned char)(*sizeFile);
        package[4]=1;
        package[5]=L2;

        int i;
        int j = 6;
        for(i=0; i < strlen(fileName); i++,j++) {
                package[j]=fileName[i];
        }
        return package;
}
