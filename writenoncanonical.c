/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

////////////////////////////////////////////////////
#define FLAG 0x7E
#define A 0x03
#define setC 0x03
#define uaC 0x07
#define BCCA 0x04
#define BCCSET 0x00

volatile int STOP=FALSE;


int sumAlarms=0;
int connectionEstabilished=0;
int fd;
int flagAlarm = 0;
void alarmTimeout(){

  printf("Alarm=%d", sumAlarms);

  //se ainda nao tivermos recebido UA e a soma dos alarmes for inferior a 4
  if(!connectionEstabilished && sumAlarms < 4 )
  {
    sumAlarms++;

  }
  else
  {
    printf("O pedido falhou!\n");
    exit(-1);
  }

}

int main(int argc, char** argv)
{
  int c, res;
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
  //(void) signal(SIGALRM,alarmTimeout);

  LLOPEN(fd,0);

  sleep(1);
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}



void LLOPEN(int fd, int x){
  requestConnection(fd);
}



void requestConnection(int fd){

 unsigned char SET[5] = {FLAG,A,setC,BCCSET,FLAG};
  write(fd,SET,5);
  printf("mandou set\n");
  alarm(3);

  unsigned char c;
  int state=0;

  while(!connectionEstabilished){
    read(fd,&c,1);
    switch(state){
      //recebe flag
      case 0:
        printf("state = %d \n",state );
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
        if(c==uaC)
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
        if(c==BCCA)
          state = 4;
        else
          state=0;
      break;
      //recebe FLAG final
      case 4:
        printf("state = %d \n",state );
        if(c==FLAG){
          connectionEstabilished=1;
          printf("recebeu ua\n");
        }
        else
          state = 0;
      break;
    }
  }
}
