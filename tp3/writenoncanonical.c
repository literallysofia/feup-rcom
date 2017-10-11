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

////////////////////////////////////////////////////
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


volatile int STOP=FALSE;

void LLOPEN(int fd, int x);

void LLWRITE(int fd, char* mensagem, int size);

char calculoBCC2(char* mensagem, int size);

int sumAlarms=0;

int flagAlarm = FALSE;
int trama = 0;
int paragem = FALSE;

//handler
void alarmHandler(){
  printf("Alarm=%d\n", sumAlarms);
  flagAlarm=TRUE;
  sumAlarms++;
}

int main(int argc, char** argv)
{
  int fd, c, res;
  struct termios oldtio,newtio;
  char buf[255];
  int i, sum = 0, speed = 0;

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

  newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

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

  char mensagem[] = "Hello World";

  //LLOPEN(fd,0);
  LLWRITE(fd,mensagem,strlen(mensagem));

  sleep(1);
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}



void LLOPEN(int fd, int x){

  unsigned char SET[5] = {FLAG,A,SET_C, SET_BCC,FLAG};
	unsigned char c;

  do {
    write(fd,SET,5);
    printf("mandou set\n");
    alarm(3);
    flagAlarm=0;


    int state=0;

    while(!paragem && !flagAlarm){
      read(fd,&c,1);
	printf("%x\n",c);
      switch(state){
        //recebe flag
        case 0:
          //printf("state = %d \n",state );
          if(c==FLAG)
            state=1;
          break;
        //recebe A
        case 1:
          printf("state = %d \n",state );
          if(c==A)
            state=2;
          else
            {
              if(c==FLAG)
                state=1;
              else
                state = 0;
            }
          break;
        //recebe C
        case 2:
          printf("state = %d \n",state );
          if(c==UA_C)
            state=3;
          else{
            if(c==FLAG)
              state=1;
            else
              state = 0;
          }
        break;
        //recebe BCC
        case 3:
          printf("state = %d \n",state );
          if(c == UA_BCC)
            state = 4;
          else
            state=0;
        break;
        //recebe FLAG final
        case 4:
          printf("state = %d \n",state );
          if(c==FLAG){
            paragem=TRUE;
            alarm(0);
            printf("recebeu ua\n");
          }
          else
            state = 0;
        break;
      }
    }
  } while(flagAlarm && sumAlarms < NUMMAX);

}


void LLWRITE(int fd, char* mensagem, int size){
  char BCC2;
  char* mensagemFinal = (char*)malloc((size+6)*sizeof(char));
  int sizeMensagemFinal = size + 6;
  BCC2 = calculoBCC2(mensagem,size);
  mensagemFinal[0]= FLAG;
  mensagemFinal[1]=A;
  if(trama == 0){
    mensagemFinal[2]=C10;
    trama=1;
  } else{
    mensagemFinal[2]=C11;
    trama=0;
  }
  mensagemFinal[3] = (mensagemFinal[1]^mensagemFinal[2]);

  int i=0;
  int j=4;
  for(; i < size; i++){
    if(mensagem[i] == FLAG){
      mensagemFinal = (char*)realloc(mensagemFinal, ++sizeMensagemFinal);
      mensagemFinal[j]=Escape;
      mensagemFinal[j+1]=escapeFlag;
      j = j+2;
    }
    else{
      if(mensagem[i]==Escape){
        mensagemFinal = (char*)realloc(mensagemFinal, ++sizeMensagemFinal);
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
  mensagemFinal[j]=BCC2;
  mensagemFinal[j+1]=FLAG;

  i =0;
  for(; i < sizeMensagemFinal;i++){
    printf("%x\n",mensagemFinal[i] );
  }


}

char calculoBCC2(char* mensagem, int size){
  char BCC2 = mensagem[0];
  int i;
  for(i=1; i < size; i++){
    BCC2^=mensagem[i];
  }
  return BCC2;
}
