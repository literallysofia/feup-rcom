/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define SET_FLAG 0x7E
#define SET_A 0x03
#define SET_C 0x03
#define SET_BCC 0x00
#define UA_FLAG 0x7E
#define UA_A 0x03
#define UA_C 0x07
#define UA_BCC UA_A^UA_C


volatile int STOP=FALSE;

void readRequestConnection(int fd);

int main(int argc, char** argv)
{
        int fd,c, res;
        struct termios oldtio,newtio;
        char buf[255];

        if ( (argc < 2) ||
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

        newtio.c_cc[VTIME]    = 1;/* inter-character timer unused */
        newtio.c_cc[VMIN]     = 0;/* blocking read until 5 chars received */



        /*
           VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
           leitura do(s) próximo(s) caracter(es)
         */



        tcflush(fd, TCIOFLUSH);



        printf("New termios structure set\n");


        /* int i = 0;
           while(STOP == FALSE) {
           res = read(fd,buf+i,1);
           if(res == 1) {
           if (buf[i] == '\0') STOP = TRUE;
           i++;
           }
           }*/

        sleep(1);
        if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
                perror("tcsetattr");
                exit(-1);
        }



        readRequestConnection(fd);


        tcsetattr(fd,TCSANOW,&oldtio);
        close(fd);
        return 0;
}


void readRequestConnection(int fd){
        unsigned char c;
        int state=0;

        unsigned char UA[5] = {UA_FLAG, UA_A, UA_C, UA_BCC, UA_FLAG};

        while(state!=5) {

                read(fd,&c,1);

                switch(state) {
                //recebe flag
                case 0:
                        printf("0\n");
                        if(c==SET_FLAG)
                                state=1;
                        break;
                //recebe A
                case 1:
                        printf("1\n");
                        if(c==SET_A)
                                state=2;
                        else
                        {
                                if(c==SET_FLAG)
                                        state=1;
                                else
                                        state = 0;
                        }
                        break;
                //recebe C
                case 2:
                        printf("2\n");
                        if(c==SET_C)
                                state=3;
                        else{
                                if(c==SET_FLAG)
                                        state=1;
                                else
                                        state = 0;
                        }
                        break;
                //recebe BCC
                case 3:
                        printf("3\n");
                        if(c==SET_BCC)
                                state = 4;
                        else
                                state=0;
                        break;
                //recebe FLAG final
                case 4:
                        printf("4\n");
                        if(c==SET_FLAG) {
                                printf("Recebeu SET\n");
                                write(fd,UA,5);
                                printf("Enviou UA\n");
                                state=5;
                                break;
                        }
                        else
                                state = 0;
                        break;

                }
        }
}
