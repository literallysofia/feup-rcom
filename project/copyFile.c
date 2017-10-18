#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


unsigned char* openReadFile(unsigned char* fileName, off_t* sizeFile){
  FILE* f;
  struct stat metadata;
  unsigned char* fileData;

  if((f = fopen(fileName, "rb"))== NULL){
    perror("error opening file!");
    exit(-1);
  }
  stat(fileName,&metadata);
  (*sizeFile) = metadata.st_size;
  printf("This file has %zd bytes \n",*sizeFile);

  int bitsSize=4*(*sizeFile);

  fileData = (unsigned char*)malloc(bitsSize);
  fread(fileData,sizeof(unsigned char),bitsSize,f);
  printf("File Content: ");
  size_t i;
  for (i = 0; i < bitsSize; i++) {
    printf("%c",fileData[i]);
  }
  printf("\n");
  return fileData;
}

unsigned char* createFile(unsigned char * mensagem, off_t* sizeFile){
  FILE* fp = fopen("teste.png", "a");

  size_t i;
  for (i = 0; i < 4*(*sizeFile); i++) {
    fprintf(fp,"%c",mensagem[i]);
  }

  printf("New file created\n");
}

int main (int argc, unsigned char** argv){

  FILE* f;
  off_t sizeFile;

  unsigned char* mensagem = openReadFile(argv[1], &sizeFile);
  createFile (mensagem, &sizeFile);

}
