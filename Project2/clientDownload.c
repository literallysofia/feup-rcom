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
#include <ctype.h>

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_STRING_LENGTH 50
#define SOCKET_BUF_SIZE 1000

void readResponse(int socketfd, char *responseCode);
struct hostent *getip(char host[]);
void createFile(int fd, char* filename);
void parseArgument(char *argument, char *user, char *pass, char *host, char *path);
int sendCommandInterpretResponse(int socketfd, char cmd[], char commandContent[], char* filename, int socketfdClient);
int getServerPortFromResponse(int socketfd);
void parseFilename(char *path, char *filename);


int main(int argc, char **argv)
{
	int socketfd;
	int socketfdClient =-1;
	struct sockaddr_in server_addr;
	struct sockaddr_in server_addr_client;

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
	parseArgument(argv[1], user, pass, host, path);

	char filename[MAX_STRING_LENGTH];
	parseFilename(path, filename);

	printf(" - Username: %s\n", user);
	printf(" - Password: %s\n", pass);
	printf(" - Host: %s\n", host);
	printf(" - Path :%s\n", path);
	printf(" - Filename: %s\n", filename);

	h = getip(host);

	printf(" - IP Address : %s\n\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr))); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);											/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}

	readResponse(socketfd, responseCode); 
	if (responseCode[0] == '2')
	{										 
		printf(" > Connection Estabilished\n"); 
	}										 

	printf(" > Sending Username\n");
	int res = sendCommandInterpretResponse(socketfd, "user ", user, filename, socketfdClient);
	if (res == 1)
	{
		printf(" > Sending Password\n");
		res = sendCommandInterpretResponse(socketfd, "pass ", pass, filename, socketfdClient);
	}

	write(socketfd, "pasv\n", 5);
	int serverPort = getServerPortFromResponse(socketfd);

	/*server address handling*/
	bzero((char *)&server_addr_client, sizeof(server_addr_client));
	server_addr_client.sin_family = AF_INET;
	server_addr_client.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr))); /*32 bit Internet address network byte ordered*/
	server_addr_client.sin_port = htons(serverPort);										   /*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socketfdClient = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(socketfdClient, (struct sockaddr *)&server_addr_client, sizeof(server_addr_client)) < 0)
	{
		perror("connect()");
		exit(0);
	}
	printf("\n > Sending Retr\n");
	int resRetr =sendCommandInterpretResponse(socketfd, "retr ", path, filename, socketfdClient);

	if(resRetr==0){
		close(socketfdClient);
		close(socketfd);
		exit(0);
	}
	else printf(" > ERROR in RETR response\n");

	close(socketfdClient);
	close(socketfd);
	exit(1);

	
}

// ./download ftp://anonymous:1@speedtest.tele2.net/1KB.zip
void parseArgument(char *argument, char *user, char *pass, char *host, char *path)
{
	char start[] = "ftp://";
	int index = 0;
	int i = 0;
	int state = 0;
	int length = strlen(argument);
	while (i < length)
	{
		switch (state)
		{
		case 0: //reads the ftp://
			if (argument[i] == start[i] && i < 5)
			{
				break;
			}
			if (i == 5 && argument[i] == start[i])
				state = 1;
			else
				printf(" > Error parsing ftp://");
			break;
		case 1: //reads the username
			if (argument[i] == ':')
			{
				state = 2;
				index = 0;
			}
			else
			{
				user[index] = argument[i];
				index++;
			}
			break;
		case 2:
			if (argument[i] == '@')
			{
				state = 3;
				index = 0;
			}
			else
			{
				pass[index] = argument[i];
				index++;
			}
			break;
		case 3:
			if (argument[i] == '/')
			{
				state = 4;
				index = 0;
			}
			else
			{
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

void parseFilename(char *path, char *filename){
	int indexPath = 0;
	int indexFilename = 0;
	memset(filename, 0, MAX_STRING_LENGTH);

	for(;indexPath< strlen(path); indexPath++){

		if(path[indexPath]=='/'){
			indexFilename = 0;
			memset(filename, 0, MAX_STRING_LENGTH);
			
		}
		else{
			filename[indexFilename] = path[indexPath];
			indexFilename++;
		}
	}
}

//gets ip address according to the host's name
struct hostent *getip(char host[])
{
	struct hostent *h;

	if ((h = gethostbyname(host)) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	return h;
}

//reads response code from the server
void readResponse(int socketfd, char *responseCode)
{
	int state = 0;
	int index = 0;
	char c;

	while (state != 3)
	{	
		read(socketfd, &c, 1);
		printf("%c", c);
		switch (state)
		{
		//waits for 3 digit number followed by ' ' or '-'
		case 0:
			if (c == ' ')
			{
				if (index != 3)
				{
					printf(" > Error receiving response code\n");
					return;
				}
				index = 0;
				state = 1;
			}
			else
			{
				if (c == '-')
				{
					state = 2;
					index=0;
				}
				else
				{
					if (isdigit(c))
					{
						responseCode[index] = c;
						index++;
					}
				}
			}
			break;
		//reads until the end of the line
		case 1:
			if (c == '\n')
			{
				state = 3;
			}
			break;
		//waits for response code in multiple line responses
		case 2:
			if (c == responseCode[index])
			{
				index++;
			}
			else
			{
				if (index == 3 && c == ' ')
				{
					state = 1;
				}
				else 
				{
				  if(index==3 && c=='-'){
					index=0;
					
				}
				}
				
			}
			break;
		}
	}
}

//reads the server port when pasv is sent
int getServerPortFromResponse(int socketfd)
{
	int state = 0;
	int index = 0;
	char firstByte[4];
	memset(firstByte, 0, 4);
	char secondByte[4];
	memset(secondByte, 0, 4);

	char c;

	while (state != 7)
	{
		read(socketfd, &c, 1);
		printf("%c", c);
		switch (state)
		{
		//waits for 3 digit number followed by ' '
		case 0:
			if (c == ' ')
			{
				if (index != 3)
				{
					printf(" > Error receiving response code\n");
					return -1;
				}
				index = 0;
				state = 1;
			}
			else
			{
				index++;
			}
			break;
		case 5:
			if (c == ',')
			{
				index = 0;
				state++;
			}
			else
			{
				firstByte[index] = c;
				index++;
			}
			break;
		case 6:
			if (c == ')')
			{
				state++;
			}
			else
			{
				secondByte[index] = c;
				index++;
			}
			break;
		//reads until the first comma
		default:
			if (c == ',')
			{
				state++;
			}
			break;
		}
	}

	int firstByteInt = atoi(firstByte);
	int secondByteInt = atoi(secondByte);
	return (firstByteInt * 256 + secondByteInt);
}

//sends a command, reads the response from the server and interprets it
int sendCommandInterpretResponse(int socketfd, char cmd[], char commandContent[], char* filename, int socketfdClient)
{
	char responseCode[3];
	int action = 0;
	//sends the command
	write(socketfd, cmd, strlen(cmd));
	write(socketfd, commandContent, strlen(commandContent));
	write(socketfd, "\n", 1);

	while (1)
	{
		//reads the response
		readResponse(socketfd, responseCode);
		action = responseCode[0] - '0';

		switch (action)
		{
		//waits for another response
		case 1:
			if(strcmp(cmd, "retr ")==0){
				createFile(socketfdClient, filename);
				break;
			}
			readResponse(socketfd, responseCode);
			break;
		//command accepted, we can send another command
		case 2:
			return 0;
		//needs additional information
		case 3:
			return 1;
		//try again
		case 4:
			write(socketfd, cmd, strlen(cmd));
			write(socketfd, commandContent, strlen(commandContent));
			write(socketfd, "\r\n", 2);
			break;
		case 5:
			printf(" > Command wasn\'t accepted. Goodbye!\n");
			close(socketfd);
			exit(-1);
		}
	}
}

void createFile(int fd, char* filename)
{
	FILE *file = fopen((char *)filename, "wb+");

	char bufSocket[SOCKET_BUF_SIZE];
 	int bytes;
 	while ((bytes = read(fd, bufSocket, SOCKET_BUF_SIZE))>0) {
    	bytes = fwrite(bufSocket, bytes, 1, file);
    }

  	fclose(file);

	printf(" > Finished downloading file\n");
}
