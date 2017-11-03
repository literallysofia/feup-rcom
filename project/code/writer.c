/*Non-Canonical Input Processing*/
#include "writer.h"

int sumAlarms = 0;
int flagAlarm = FALSE;
int trama = 0;
int paragem = FALSE;
unsigned char numMensagens = 0;
int numTotalTramas = 0;

struct termios oldtio, newtio;

//handler do sinal de alarme
void alarmHandler()
{
  printf("Alarm=%d\n", sumAlarms + 1);
  flagAlarm = TRUE;
  sumAlarms++;
}

int main(int argc, char **argv)
{
  int fd;
  off_t sizeFile; //tamanho do ficheiro em bytes
  off_t indice = 0;
  int sizeControlPackageI = 0;

  if ((argc < 3) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }
  /*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
  */
  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }


  // instalar handler do alarme
  (void)signal(SIGALRM, alarmHandler);


  unsigned char *mensagem = openReadFile((unsigned char *)argv[2], &sizeFile);

  //inicio do relógio
  struct timespec requestStart, requestEnd;
  clock_gettime(CLOCK_REALTIME, &requestStart);

  //se nao conseguirmos efetuar a ligaçao atraves do set e do ua o programa termina
  if (!LLOPEN(fd, 0))
  {
    return -1;
  }

  int sizeOfFileName = strlen(argv[2]);
  unsigned char *fileName = (unsigned char *)malloc(sizeOfFileName);
  fileName = (unsigned char *)argv[2];
  unsigned char *start = controlPackageI(C2Start, sizeFile, fileName, sizeOfFileName, &sizeControlPackageI);

  LLWRITE(fd, start, sizeControlPackageI);
  printf("Mandou trama START\n");

  int sizePacket = sizePacketConst;
  srand(time(NULL));

  while (sizePacket == sizePacketConst && indice < sizeFile)
  {
    //split mensagem
    unsigned char *packet = splitMessage(mensagem, &indice, &sizePacket, sizeFile);
    printf("Mandou packet numero %d\n", numTotalTramas);
    //header nivel aplicação
    int headerSize = sizePacket;
    unsigned char *mensagemHeader = headerAL(packet, sizeFile, &headerSize);
    //envia a mensagem
    if (!LLWRITE(fd, mensagemHeader, headerSize))
    {
      printf("Limite de alarmes atingido\n");
      return -1;
    }
  }

  unsigned char *end = controlPackageI(C2End, sizeFile, fileName, sizeOfFileName, &sizeControlPackageI);
  LLWRITE(fd, end, sizeControlPackageI);
  printf("Mandou trama END\n");

  LLCLOSE(fd);

  //fim do relógio
  clock_gettime(CLOCK_REALTIME, &requestEnd);

  double accum = (requestEnd.tv_sec - requestStart.tv_sec) + (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;

  printf("Seconds passed: %f\n", accum);

  sleep(1);


  close(fd);
  return 0;
}

unsigned char *headerAL(unsigned char *mensagem, off_t sizeFile, int *sizePacket)
{
  unsigned char *mensagemFinal = (unsigned char *)malloc(sizeFile + 4);
  mensagemFinal[0] = headerC;
  mensagemFinal[1] = numMensagens % 255;
  mensagemFinal[2] = (int)sizeFile / 256;
  mensagemFinal[3] = (int)sizeFile % 256;
  memcpy(mensagemFinal + 4, mensagem, *sizePacket);
  *sizePacket += 4;
  numMensagens++;
  numTotalTramas++;
  return mensagemFinal;
}

unsigned char *splitMessage(unsigned char *mensagem, off_t *indice, int *sizePacket, off_t sizeFile)
{
  unsigned char *packet;
  int i = 0;
  off_t j = *indice;
  if (*indice + *sizePacket > sizeFile)
  {
    *sizePacket = sizeFile - *indice;
  }
  packet = (unsigned char *)malloc(*sizePacket);
  for (; i < *sizePacket; i++, j++)
  {
    packet[i] = mensagem[j];
  }
  *indice = j;
  return packet;
}

void stateMachineUA(int *state, unsigned char *c)
{

  switch (*state)
  {
  //recebe flag
  case 0:
    if (*c == FLAG)
      *state = 1;
    break;
  //recebe A
  case 1:
    if (*c == A)
      *state = 2;
    else
    {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  //recebe C
  case 2:
    if (*c == UA_C)
      *state = 3;
    else
    {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  //recebe BCC
  case 3:
    if (*c == UA_BCC)
      *state = 4;
    else
      *state = 0;
    break;
  //recebe FLAG final
  case 4:
    if (*c == FLAG)
    {
      paragem = TRUE;
      alarm(0);
      printf("Recebeu UA\n");
    }
    else
      *state = 0;
    break;
  }
}

int LLOPEN(int fd, int x)
{

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1; /* inter-unsigned character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 unsigned chars received */

  /*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a

  leitura do(s) pr�ximo(s) caracter(es)

  */

  tcflush(fd, TCIOFLUSH);


  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");


  unsigned char c;
  do
  {
    sendControlMessage(fd, SET_C);
    alarm(TIMEOUT);
    flagAlarm = 0;
    int state = 0;

    while (!paragem && !flagAlarm)
    {
      read(fd, &c, 1);
      stateMachineUA(&state, &c);
    }
  } while (flagAlarm && sumAlarms < NUMMAX);
  printf("flag alarm %d\n", flagAlarm);
  printf("soma %d\n", sumAlarms);
  if (flagAlarm && sumAlarms == 3)
  {
    return FALSE;
  }
  else
  {
    flagAlarm = FALSE;
    sumAlarms = 0;
    return TRUE;
  }
}

int LLWRITE(int fd, unsigned char *mensagem, int size)
{
  unsigned char BCC2;
  unsigned char *BCC2Stuffed = (unsigned char *)malloc(sizeof(unsigned char));
  unsigned char *mensagemFinal = (unsigned char *)malloc((size + 6) * sizeof(unsigned char));
  int sizeMensagemFinal = size + 6;
  int sizeBCC2 = 1;
  BCC2 = calculoBCC2(mensagem, size);
  BCC2Stuffed = stuffingBCC2(BCC2, &sizeBCC2);
  int rejeitado = FALSE;

  mensagemFinal[0] = FLAG;
  mensagemFinal[1] = A;
  if (trama == 0)
  {
    mensagemFinal[2] = C10;
  }
  else
  {
    mensagemFinal[2] = C11;
  }
  mensagemFinal[3] = (mensagemFinal[1] ^ mensagemFinal[2]);

  int i = 0;
  int j = 4;
  for (; i < size; i++)
  {
    if (mensagem[i] == FLAG)
    {
      mensagemFinal = (unsigned char *)realloc(mensagemFinal, ++sizeMensagemFinal);
      mensagemFinal[j] = Escape;
      mensagemFinal[j + 1] = escapeFlag;
      j = j + 2;
    }
    else
    {
      if (mensagem[i] == Escape)
      {
        mensagemFinal = (unsigned char *)realloc(mensagemFinal, ++sizeMensagemFinal);
        mensagemFinal[j] = Escape;
        mensagemFinal[j + 1] = escapeEscape;
        j = j + 2;
      }
      else
      {
        mensagemFinal[j] = mensagem[i];
        j++;
      }
    }
  }

  if (sizeBCC2 == 1)
    mensagemFinal[j] = BCC2;
  else
  {
    mensagemFinal = (unsigned char *)realloc(mensagemFinal, ++sizeMensagemFinal);
    mensagemFinal[j] = BCC2Stuffed[0];
    mensagemFinal[j + 1] = BCC2Stuffed[1];
    j++;
  }
  mensagemFinal[j + 1] = FLAG;

  //mandar mensagem
  do
  {

    unsigned char *copia;
    copia = messUpBCC1(mensagemFinal, sizeMensagemFinal); //altera bcc1
    copia = messUpBCC2(copia, sizeMensagemFinal);         //altera bcc2
    write(fd, copia, sizeMensagemFinal);

    flagAlarm = FALSE;
    alarm(TIMEOUT);
    unsigned char C = readControlMessageC(fd);
    if ((C == CRR1 && trama == 0) || (C == CRR0 && trama == 1))
    {
      printf("Recebeu rr %x, trama = %d\n", C, trama);
      rejeitado = FALSE;
      sumAlarms = 0;
      trama ^= 1;
      alarm(0);
    }
    else
    {
      if (C == CREJ1 || C == CREJ0)
      {
        rejeitado = TRUE;
        printf("Recebeu rej %x, trama=%d\n", C, trama);
        alarm(0);
      }
    }
  } while ((flagAlarm && sumAlarms < NUMMAX) || rejeitado);
  if (sumAlarms >= NUMMAX)
    return FALSE;
  else
    return TRUE;
}

void sendControlMessage(int fd, unsigned char C)
{
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = C;
  message[3] = message[1] ^ message[2];
  message[4] = FLAG;
  write(fd, message, 5);
}

unsigned char readControlMessageC(int fd)
{
  int state = 0;
  unsigned char c;
  unsigned char C;

  while (!flagAlarm && state != 5)
  {
    read(fd, &c, 1);
    switch (state)
    {
    //recebe FLAG
    case 0:
      if (c == FLAG)
        state = 1;
      break;
    //recebe A
    case 1:
      if (c == A)
        state = 2;
      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe c
    case 2:
      if (c == CRR0 || c == CRR1 || c == CREJ0 || c == CREJ1 || c == DISC)
      {
        C = c;
        state = 3;
      }
      else
      {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe BCC
    case 3:
      if (c == (A ^ C))
        state = 4;
      else
        state = 0;
      break;
    //recebe FLAG final
    case 4:
      if (c == FLAG)
      {
        alarm(0);
        state = 5;
        return C;
      }
      else
        state = 0;
      break;
    }
  }
  return 0xFF;
}

unsigned char calculoBCC2(unsigned char *mensagem, int size)
{
  unsigned char BCC2 = mensagem[0];
  int i;
  for (i = 1; i < size; i++)
  {
    BCC2 ^= mensagem[i];
  }
  return BCC2;
}

unsigned char *stuffingBCC2(unsigned char BCC2, int *sizeBCC2)
{
  unsigned char *BCC2Stuffed;
  if (BCC2 == FLAG)
  {
    BCC2Stuffed = (unsigned char *)malloc(2 * sizeof(unsigned char *));
    BCC2Stuffed[0] = Escape;
    BCC2Stuffed[1] = escapeFlag;
    (*sizeBCC2)++;
  }
  else
  {
    if (BCC2 == Escape)
    {
      BCC2Stuffed = (unsigned char *)malloc(2 * sizeof(unsigned char *));
      BCC2Stuffed[0] = Escape;
      BCC2Stuffed[1] = escapeEscape;
      (*sizeBCC2)++;
    }
  }

  return BCC2Stuffed;
}

unsigned char *openReadFile(unsigned char *fileName, off_t *sizeFile)
{
  FILE *f;
  struct stat metadata;
  unsigned char *fileData;

  if ((f = fopen((char *)fileName, "rb")) == NULL)
  {
    perror("error opening file!");
    exit(-1);
  }
  stat((char *)fileName, &metadata);
  (*sizeFile) = metadata.st_size;
  printf("This file has %ld bytes \n", *sizeFile);

  fileData = (unsigned char *)malloc(*sizeFile);

  fread(fileData, sizeof(unsigned char), *sizeFile, f);
  return fileData;
}

unsigned char *controlPackageI(unsigned char state, off_t sizeFile, unsigned char *fileName, int sizeOfFileName, int *sizeControlPackageI)
{
  *sizeControlPackageI = 9 * sizeof(unsigned char) + sizeOfFileName;
  unsigned char *package = (unsigned char *)malloc(*sizeControlPackageI);

  if (state == C2Start)
    package[0] = C2Start;
  else
    package[0] = C2End;
  package[1] = T1;
  package[2] = L1;
  package[3] = (sizeFile >> 24) & 0xFF;
  package[4] = (sizeFile >> 16) & 0xFF;
  package[5] = (sizeFile >> 8) & 0xFF;
  package[6] = sizeFile & 0xFF;
  package[7] = T2;
  package[8] = sizeOfFileName;
  int k = 0;
  for (; k < sizeOfFileName; k++)
  {
    package[9 + k] = fileName[k];
  }
  return package;
}

void LLCLOSE(int fd)
{
  sendControlMessage(fd, DISC);
  printf("Mandou DISC\n");
  unsigned char C;
  //espera ler o DISC
  C = readControlMessageC(fd);
  while (C != DISC)
  {
    C = readControlMessageC(fd);
  }
  printf("Leu DISC\n");
  sendControlMessage(fd, UA_C);
  printf("Mandou UA final\n");
  printf("Writer terminated \n");

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }
}

unsigned char *messUpBCC2(unsigned char *packet, int sizePacket)
{
  unsigned char *copia = (unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc2ErrorPercentage)
  {
    int i = (rand() % (sizePacket - 5)) + 4;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC2\n");
  }
  return copia;
}

unsigned char *messUpBCC1(unsigned char *packet, int sizePacket)
{
  unsigned char *copia = (unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc1ErrorPercentage)
  {
    int i = (rand() % 3) + 1;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC1\n");
  }
  return copia;
}
