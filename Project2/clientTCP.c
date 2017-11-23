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

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_STRING_LENGTH 50

void readResponse(int socketfd, char* responseCode);
struct hostent *getip(char host[]);
void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char filename[]);
void parseArgument(char* argument,char* user,char* pass,char* host,char* path);
int sendCommandInterpretResponse(int socketfd,char cmd[],char commandContent[]);

int main(int argc, char** argv){
	int	socketfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "";
	int	bytes;

	struct hostent *h;

	char user[MAX_STRING_LENGTH];
	char responseCode[3];
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

	h = getip(host);

	printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));


	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr)));	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if(connect(socketfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
			perror("connect()");
			exit(0);
		}

	////////////////////////////////////////////
	//Aqui so temos em conta se a responsta foi
	//positiva, nao sei como fazer se nao for //
	readResponse(socketfd,responseCode);	//
	if(responseCode[0]=='2'){											//
		printf("Connection Estabilished\n");		//
	}																					//
	////////////////////////////////////////////

	printf("Sending Username\n");
	int res = sendCommandInterpretResponse(socketfd,"user ",user);
	if(res == 1){
		printf("Sending Password\n");
		res = sendCommandInterpretResponse(socketfd,"pass ",pass);
	}





		/*send a string to the server*/
		bytes = write(socketfd, buf, strlen(buf));
		printf("Bytes escritos %d\n", bytes);

		close(socketfd);
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

//gets ip address according to the host's name
struct hostent * getip(char host[]){
	struct hostent *h;

	if ((h=gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	return h;
}

//reads response code from the server
void readResponse(int socketfd, char* responseCode){
	int state = 0;
	int index = 0;
	char c;

	while (state != 3) {
		read(socketfd,&c,1);
		printf("%c",c);
		switch (state) {
			//waits for 3 digit number followed by ' ' or '-'
			case 0:
				if(c == ' '){
					if (index != 3) {
						printf("Error receiving response code\n");
						return;
					}
					index = 0;
					state = 1;
				}else{
					if (c == '-') {
						state = 2;
					}
					else{
						responseCode[index] = c;
						index++;
					}
				}
				break;
			//reads until the end of the line
			case 1:
				if(c == '\n'){
					state = 3;
				}
				break;
			//waits for response code in multiple line responses
			case 2:
				if(c == responseCode[index]){
					index++;
				}else{
					if(index == 3 && c == ' '){
						state = 1;
					}else {
						printf("Error receiving response code\n");
						return;
					}
				}
				break;
		}
	}
}

int sendCommandInterpretResponse(int socketfd,char cmd[],char commandContent[]){
	char responseCode[3];
	int action = 0;
	//sends the command
	write(socketfd,cmd,strlen(cmd));
	write(socketfd,commandContent,strlen(commandContent));
	write(socketfd,"\n",1);

	while(1){
		//reads the response
	 	readResponse(socketfd,responseCode);
		action = responseCode[0] - '0';

		switch (action) {
			//waits for another response
			case 1:
				readResponse(socketfd,responseCode);
				break;
			//command accepted, we can send another command
			case 2:
				return 0;
			//needs additional information
			case 3:
				return 1;
			//try again
			case 4:
				write(socketfd,cmd,strlen(cmd));
				write(socketfd,commandContent,strlen(commandContent));
				write(socketfd,"\n",1);
				break;
			case 5:
				printf("Command wasn\'t accepted!\n Goodbye");
				close(socketfd);
				exit(-1);
		}
	}
}

void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char filename[]){
	FILE *file = fopen((char *)filename, "wb+");
	fwrite((void *)mensagem, 1, *sizeFile, file);
	printf("%zd\n", *sizeFile);
	printf("New file created\n");
	fclose(file);
}
