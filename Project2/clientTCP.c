/*      (C)2000 FEUP  */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define MAX_STRING_LENGTH 50

void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char filename[]);
void parseArgument(char* argument,char* user,char* pass,char* host,char* path);

int main(int argc, char** argv){
	char user[MAX_STRING_LENGTH];
	memset(user, 0, MAX_STRING_LENGTH);

	char pass[MAX_STRING_LENGTH];
	memset(pass, 0, MAX_STRING_LENGTH);

	char host[MAX_STRING_LENGTH];
	memset(host, 0, MAX_STRING_LENGTH);

	char path[MAX_STRING_LENGTH];
	memset(path, 0, MAX_STRING_LENGTH);
	parseArgument(argv[1],user,pass,host,path);

	printf("user %s\n",user);
	printf("pass %s\n",pass);
	printf("host %s\n",host);
	printf("path %s\n",path);
	int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "";
	int	bytes;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	/*connect to the server*/
    	if(connect(sockfd,
	           (struct sockaddr *)&server_addr,
		   sizeof(server_addr)) < 0){
        	perror("connect()");
		exit(0);
	}



    	/*send a string to the server*/


	bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	close(sockfd);
	exit(0);
}

// ./download ftp://anonymous:1@speedtest.tele2.net/1KB.zip
void parseArgument(char* argument,char* user,char* pass,char* host,char* path) {
	char start[] = "ftp://";
	int index = 0;
	int i = 0;
	int state = 0;
	int length = strlen(argument);
	while (i < length) {
		switch (state) {
			case 0://reads the ftp://
				if (argument[i] == start[i] && i < 5) {
					break;
				}
				if(i == 5 && argument[i] == start[i])
					state = 1;
				else
					printf("Error parsing ftp://");
				break;
			case 1://reads the username
				if(argument[i] == ':'){
					state = 2;
					index = 0;
				}else{
					user[index] = argument[i];
					index++;
				}
				break;
			case 2:
				if (argument[i] == '@') {
					state = 3;
					index = 0;
				}else{
					pass[index] = argument[i];
					index++;
				}
				break;
			case 3:
				if (argument[i] == '/') {
					state = 4;
					index = 0;
				}else{
					host[index] = argument[i];
					index++;
				}
				break;
			case 4:
				path[index] = argument[i];
				index++;
				break;
		}
		i++;
	}
}

void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char filename[])
{
  FILE *file = fopen((char *)filename, "wb+");
  fwrite((void *)mensagem, 1, *sizeFile, file);
  printf("%zd\n", *sizeFile);
  printf("New file created\n");
  fclose(file);
}
