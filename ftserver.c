/********************************************************** 
** Program Filename: ftserver.c
** Author: Jonathan Grocott
** Date: 11/19/18
** CS_372_400_F2018 Project 2
** File Transfer Server
** Requires ftclient.py on another host
** Reference: Code based off Beej guide to Client-Server Interaction
** http://beej.us/net2/bgnet.html#clientserver
** https://www.geeksforgeeks.org/socket-programming-cc/
**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>

#define MSG_SIZE 100000
int serverPort;
char currentDirectory[MSG_SIZE];


/*****************************************************************
* Description: Sends the data file
* input: int dataPort, char ptr filename
* output: void
****************************************************************/
void sendData(int dataPort, char* filename) {
	//int dataPort = *(int *)input);
	int dataSocket = createDataSocket(dataPort);

	size_t length = 0;
	ssize_t lineSize;
	char* line = NULL;
	FILE *file = fopen(filename, "r");
	while ((lineSize = getline(&line, &length, file)) != -1) {
		write(dataSocket, line, lineSize);
	}
	free(line);
	fclose(file);
	close(dataSocket);
}

/*****************************************************************
* Description: Lists the files
* input: int dataPort
* output: void
****************************************************************/
void listFiles(int dataPort) {
	//int dataPort = *(int *)input);
	char message[MSG_SIZE];
	memset(message, '\0', sizeof(message));
	int dataSocket = createDataSocket(dataPort);
	DIR *dir;
	struct dirent *current;
	dir = opendir(".");
	if (dir) {
		while ((current = readdir(dir)) != 0) {
			strcat(message, current->d_name);
			strcat(message, " ");
		}
		write(dataSocket, message, strlen(message));
	}
	else {
		perror("Error: Failed to open directory\n");
		exit(1);
	}
	closedir(dir);
	close(dataSocket);
}

/*****************************************************************
* Description: Check if a requested file exists
* input: char ptr filename
* output: int stat
****************************************************************/
int checkFileExists(char *filename) {
	struct stat buf;
	return stat(filename, &buf);
}

/*****************************************************************
* Description: Makes the data socket
* input: int dataPort
* output: int dataSocket
****************************************************************/
int createDataSocket(int dataPort) {
	struct sockaddr_in serverAddr;
	struct hostent *serverHost;

	memset(&serverAddr, '0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(dataPort);

	serverHost = gethostbyname("localhost");
	memcpy((char*)&serverAddr.sin_addr.s_addr, (char*)serverHost->h_addr, serverHost->h_length);

	int dataSocket = socket(AF_INET, SOCK_STREAM, 0);	//creates socket
	if (dataSocket < 0) {	//verifies that it worked
		printf("Failed Socket...\n");
		return 1;
	}
	if (connect(dataSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0) {	//connects to server
		printf("Failed to Connect...\n");
		return 1;
	}
	return dataSocket;	//returns value
}

/*****************************************************************
* Description: Creates the comm socket
* input: int serverPort
* output: int socket_descriptor
****************************************************************/
int createCommSocket(int serverPort) {
	struct sockaddr_in server;

	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_descriptor < 0) {
		printf("Failed Socket...\n");
		return 1;
	}

	memset(&server, '\0', sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(serverPort);

	if (bind(socket_descriptor, (struct sockaddr *)&server, sizeof(server)) != 0) {
		perror("Error: Failed Binding\n");	//print the error
		return 1;
	}

	return socket_descriptor;
}


/*****************************************************************
* Description: Handles the communication for file transfer
* input: void ptr to desc
* output: void
****************************************************************/
void* ftCommunication(void* desc) {
	int clientSocket = *(int *)desc;
	int messageSize;
	char command[MSG_SIZE], comm[MSG_SIZE], message[MSG_SIZE];
	char authentication[MSG_SIZE], username[MSG_SIZE], password[MSG_SIZE];
	char* token;
	//pthread_t dataThread;
	int dataPort = serverPort - 1;
	memset(command, '\0', sizeof(command));
	memset(comm, '\0', sizeof(comm));
	memset(message, '\0', sizeof(message));
	memset(authentication, '\0', sizeof(authentication));
	memset(username, '\0', sizeof(username));
	memset(password, '\0', sizeof(password));

	messageSize = recv(clientSocket, authentication, MSG_SIZE, 0);
	if (messageSize == 0) {
		printf("Connection Closed...\n");
		return;
	}
	else if (messageSize < 0) {
		perror("Error: Receive Message Failed\n");
		return;
	}
	int i = 0;
	while (authentication[i] != '&' && authentication[i] != '\0')	i++;
	strncpy(username, authentication, i);
	char* c = strstr(authentication, "&");
	c++;
	strcpy(password, c);

	if (strcmp(username, "admin") != 0 || strcmp(password, "password1") != 0) {
		sprintf(message, "Invalid Username or Password!");
		write(clientSocket, message, strlen(message));
		printf(message);
		return;
	}
	else {
		sprintf(message, "%s %s!", "Welcome", username);
		write(clientSocket, message, strlen(message));
	}

	memset(message, '\0', sizeof(message));
	messageSize = recv(clientSocket, command, MSG_SIZE, 0);
	if (messageSize == 0) {
		printf("Connection Closed...\n");
		return;
	}
	else if (messageSize < 0) {
		perror("Error: Receive Message Failed\n");
		return;
	}
	strcpy(comm, command);
	token = strtok(comm, " \0\n");
	if (strcmp(token, "-l") == 0) {
		/*if(pthread_create(&dataThread, NULL,  listFiles, (void*)&dataPort) < 0){
		perror("Thread creation failed. Error");
		exit(1);
		}*/
		listFiles(dataPort);
	}
	else if (strcmp(token, "-g") == 0) {
		token = strtok(NULL, " \0\n");
		if (checkFileExists(token) == -1) {
			sprintf(message, "%s", "FILE NOT FOUND");
			write(clientSocket, message, strlen(message));
		}
		else {
			/*if(pthread_create(&dataThread, NULL,  sendData, (void*)&dataPort) < 0){
			perror("Thread creation failed. Error");
			exit(1);
			}*/
			sendData(dataPort, token);
		}
	}
	else if (strcmp(token, "cd") == 0) {
		memset(currentDirectory, '\0', sizeof(currentDirectory));
		token = strtok(NULL, " \0\n");
		strcpy(currentDirectory, token);
	}
	else {
		memset(message, '\0', sizeof(message));
		sprintf(message, "%s", "INVALID COMMAND");
		write(clientSocket, message, strlen(message));
	}
	write(clientSocket, "/quit", 6);
}



/*****************************************************************
* Description: Main for file transfer server	
* input: arguments
* output: none
****************************************************************/
int main(int argc, char* argv[]){
	memset(currentDirectory,'\0',sizeof(currentDirectory));
	strcpy(currentDirectory,".");
	
	if(argc != 2){
		printf("Error: Invalid Arguments\n");
		printf("ftserver [port]\n");
		return 1;
	}

	serverPort = atoi(argv[1]);
	int socket_descriptor, clientSocket;
	struct sockaddr_in client;	
	
	socket_descriptor = createCommSocket(serverPort);
	listen(socket_descriptor, 5);
	
	socklen_t clientSize = sizeof(client);
	while(1){
		clientSocket = accept(socket_descriptor,(struct sockaddr *)&client,&clientSize);
		pthread_t commThread;
		if(pthread_create(&commThread, NULL,  ftCommunication, (void*)&clientSocket) < 0){
			perror("Error: Thread Creation Failed\n");
			return 1;
		}
	}
	close(socket_descriptor);
	return 0;
}
