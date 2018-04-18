/* 	Computer Systems COMP30023 Part B
*	by Samuel Xu #835273, samuelx@student.unimelb.edu.au  
*
*	Using the provided server.c code given in Workshop 3
*
*  	This is a simple HTTP server program which should serve certain content 
*	with a given GET request.
*	It simply returns HTTP200 response and a file if it's found, and 404 if
*	not.
*   This server uses a basic implementation of Pthreads to process incoming
*	requests and sending messages. 
*
*	
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
	char* filepath;
	int code;
	char* version;
} request_t;

typedef struct {
	char* root_dir;
	int portno;
	int newsockfd;
	int thread;
} conn_handle;

request_t parseRequest(char *raw_request, char *root_dir);
void printRequest(request_t request);
int checkPath(char* token);
char* getFileType(char* filepath);
char* buildResponse(request_t request);
void sendContent(char* filepath, int newsockfd);
void respond(request_t request, int newsockfd);
void *connection_handler(void *args);

int main(int argc, char **argv)
{
	// Main should read input and start the necessary servers with Pthread
	// This should take in 2 command line arguments:
	// Port number and string path to root web dir

	int sockfd, newsockfd, portno;// clilen;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	int n;

	conn_handle input;

	// Check that the correct number of arguments have been supplied
	if (argc < 3) 
	{
		fprintf(stderr,"ERROR, Incorrect number of arguments supplied.\n\
						Usage: server <port number> <path to content>\n");
		exit(1);
	}

	// Parse arguments
	input.portno = atoi(argv[1]);
	input.root_dir = (char *) malloc(sizeof(argv[2])+1);
	strcpy(input.root_dir, argv[2]);
	//strcat(input.root_dir, "/");

	printf("Port number input: %d\n", input.portno);
	printf("Root directory for request: %s\n", input.root_dir);
	
	 /* Create TCP socket */
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// printf("socket file desciptor: %d\n", sockfd);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));

	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(input.portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */

	listen(sockfd,10);

	// printf("listening...\n");

	/* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */

	clilen = sizeof(cli_addr);

    pthread_t snifferThread;
    while(1) {
		input.newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
							&clilen);

		// printf("newsockfd %d\n", newsockfd);

		if (input.newsockfd < 0) 
		{
			perror("ERROR on accept");
			exit(1);
		}
		
		// printf("Connection Accepted!\n");
		input.thread = (int)snifferThread;
		
		pthread_create(&snifferThread, NULL, connection_handler, (void *) &input);

		
	}
	
	return 0; 
}

void *connection_handler(void *args) {
	int n;
	char *buffer;
	conn_handle vars = *((conn_handle *) args);
	printf("Starting thread %d\n", vars.thread);

	buffer = malloc(sizeof(char) * 1000);

	bzero(buffer,1000);

	/* Read characters from the connection,
		then process */

	n = read(vars.newsockfd,buffer,1000);

	// printf("read request: \n\n%s\n", buffer);

	request_t request = parseRequest(buffer, vars.root_dir);

	//printRequest(request);

	respond(request, vars.newsockfd);

	if (n < 0) 
	{
		perror("ERROR reading from socket");
		exit(1);
	}
	
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	close(vars.newsockfd);
	pthread_exit(NULL);
	return;
}

void respond(request_t request, int newsockfd) {
	// printf("Building response...\n");
	char* response = buildResponse(request);
	write(newsockfd, response, strlen(response));
	if (request.code == 200) {
		sendContent(request.filepath, newsockfd);
	}
}

char* buildResponse(request_t request) {
	char *filetype;
	char *response = NULL;
	if (request.code == 404) {
		response = malloc(sizeof(char) * 100);
		sprintf(response, "HTTP/%s 404\n", request.version);
	}
	else if (request.code == 200) {
		filetype = getFileType(request.filepath);	
		response = malloc(sizeof(char) * 100 + strlen(filetype));
		sprintf(response, "HTTP/%s 200 OK\nContent-Type: %s\n\n", request.version, filetype);
	}
	printf("Serving response:\n\n%s \n\n", response);
	return response;
}

char* getFileType(char* filepath) {
	printf("getting filetype from %s...\n", filepath);
	char* tmp = NULL;
	char* filetype = NULL;

	tmp = malloc(sizeof(char) * 4);
	filetype = malloc(sizeof(char) * 10);
	sscanf(filepath, "./%*[A-z0-9/:\n].%s", tmp);
	if (tmp == NULL) {
		printf("Trying again...\n");
		sscanf(filepath, "%*[A-z0-9/:\n].%s", tmp);
	}
	// printf("File type of request is: %s\n", tmp);
	if ((strcmp(tmp, "html") == 0)) {
		strcpy(filetype, "text/html");
	}
	else if (strcmp(tmp, "css") == 0) {
		strcpy(filetype, "text/css");
	}
	else if (strcmp(tmp, "js") == 0) {
		strcpy(filetype, "text/javascript");
	}
	else if (strcmp(tmp,"jpg") == 0) {
		sprintf(filetype, "image/jpeg");
	}
	return filetype;
}

void sendContent(char* filepath, int newsockfd) {
	printf("getting content...\n");
	FILE* file = fopen(filepath, "rb");
	// assert(file);
    fseek(file, 0, SEEK_END);
    unsigned long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *buffer = (char *) malloc(length+1);
    fread(buffer,length,sizeof(unsigned char),file);

    write(newsockfd, buffer, length);
    fclose(file);
    
    return buffer;
}

request_t parseRequest(char *raw_request, char *root_dir) {
	char *tmpPath;
	char *filePath;
	char *version;
	int testCode;
	request_t request;
	
	tmpPath = malloc(sizeof(char) * strlen(raw_request));
	filePath = malloc(sizeof(char) * strlen(raw_request));
	version = malloc(sizeof(char) * 3);
	//printf("Scanning request: %s\n", raw_request);
	sscanf(raw_request, "GET %s HTTP/%s %*[A-z0-9/:\n]", tmpPath, version);
	sprintf(filePath, "%s%s", root_dir, tmpPath);

	testCode = checkPath(filePath);
	if (testCode == 404) {
		request.code = 404;
		request.filepath = NULL;
		request.version = version;
	}
	else if (testCode == 200) {
		request.code = 200;
		request.filepath = malloc(sizeof(char) * strlen(raw_request));
		request.version = version;
		strcpy(request.filepath, filePath);
	}

	return request;
};

void printRequest(request_t request) {
	// printf("Code: %d\n", request.code);
	// printf("Path: %s\n", request.filepath);
}

int checkPath(char* token) {
	printf("Checking path: %s\n", token);
	if (fopen(token, "r") == NULL) {
		// printf("file not found, giving 404\n");
		return 404;
	}
	else {
		// printf("file %s found! giving code 200\n", token);
		return 200;
	}
}