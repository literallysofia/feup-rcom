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
#define UA_BCC 0x04

volatile int STOP=FALSE;

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

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);



    printf("New termios structure set\n");
	
    char SET[5];
    SET[0]=SET_FLAG;
    SET[1]=SET_A;
    SET[2]=SET_C;
    SET[3]=SET_BCC;
    SET[4]=SET_FLAG;

    char UA[5];
    UA[0]=UA_FLAG;
    UA[1]=UA_A;
    UA[2]=UA_C;
    UA[3]=UA_BCC;
    UA[4]=UA_FLAG;


    int i = 0;
    while(STOP == FALSE) {
	res = read(fd,buf+i,1);
	if(res == 1) {
	    if (buf[i] == '\0') STOP = TRUE;
	    i++;
	}
    }

    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    printf("%s\n", buf);

    int res2 = write(fd, buf, strlen(buf) + 1);
    printf("%d bytes written\n", res2);



  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}


void LLOPEN (int fd, int x){
	
	int res;
	char buf[256];
	int flag = 0;
	int stage = 1;
	
	while(!flag){
	   res= read(fd, buf,2);

	}

	switch (stage)
	case 1: 
		if(buf==SET_FLAG){
		   stage=2;}
		break;
	case 2: 
		if(buf==SET_FLAG){}
		else if(buf==SET_A){
		   stage=3;}
		else {
		   stage=1;}
		break;
	case 3: 
		if(buf==SET_C){
		   stage=4;}
		else {
		   stage=1;}
		break;
			
			
		
	


}








