/* 	Computer Systems COMP30023 Part B
*	by Samuel Xu #835273, samuelx@student.unimelb.edu.au  
*
*	Using the provided server.c code given in Lab Week 3
*
*  	This is a simple HTTP server program which should serve certain content 
*	with a given GET request.
*	It simply returns HTTP200 response and a file if it's found, and 404 if
*	not.
*   This server uses a basic implementation of Pthreads to process incoming
*	requests and sending messages. 
*
*	Style Notes:
	Following the provided server.c style, we'll be doing the following:
		- Character ruler of 78 characters. This allows us to read in consoles
			like VI or nano without wordwrap
		- Use underscores when there are names_with_spaces
		- We'll be putting asterisks before the variable names, not the type
			(allows us to define )
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

#define CONN_MAX 10
#define BUFFER_SIZE 256
#define VERSION_LEN 3
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define OK 200

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

void *connection_handler(void *args);
request_t parseRequest(char *raw_request, char *root_dir);
void printRequest(request_t request);
int checkPath(char* token);
char* getFileType(char* filepath);
char* buildResponse(request_t request);
void sendContent(char* filepath, int newsockfd);
void respond(request_t request, int newsockfd);

int main(int argc, char **argv)
{
	// Main should read input and start the necessary servers with Pthread
	// This should take in 2 command line arguments:
	// Port number and string path to root web dir

	int sockfd;						// socket file descriptor & port
	struct sockaddr_in serv_addr, cli_addr; // server/client addresses
	socklen_t clilen;						// Length of client address

	conn_handle input;				// Struct to hold variables for pthread 

	// Check that the correct number of arguments have been supplied
	if (argc < 3) 
	{
		perror("ERROR, Incorrect number of arguments supplied.\n\
				Usage: server <port number> <path to content>\n");
		exit(1);
	}

	// Parse arguments
	input.portno = atoi(argv[1]);
	input.root_dir = (char *) malloc(sizeof(argv[2])+1);
	strcpy(input.root_dir, argv[2]);
	
	 // Create a TCP socket
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));

	// Create an address that this machine is going to listen on
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(input.portno);  

	// Bind our address to our socket
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	// Listen on our socket. We'll accept CONN_MAX connections maximum.

	listen(sockfd, CONN_MAX);

	// Get the size of our client address for accepting connections later.

	clilen = sizeof(cli_addr);

	// Make an identifier for our threads
    pthread_t thread_id;

    // Keep serving requests (and hope we don't get DDoS'd)
    while(1) {

		// Accept a connection!
		input.newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
							&clilen);

		if (input.newsockfd < 0) 
		{
			perror("ERROR on accept");
			exit(1);
		}
		
		input.thread = (int)thread_id;
		
		// Create a thread to handle the connection.
		pthread_create(&thread_id, NULL, connection_handler, (void *) &input);

		
	}
	
	return 0; 
}

void *connection_handler(void *args) {
	// This handles incoming requests, parses them and responds with the
	// specified file or error code.

	int n;
	char *buffer;

	// Cast our void pointer back to struct so we can get the arguments
	// passed in from main()
	conn_handle vars = *((conn_handle *) args);

	// Read in characters from our client.
	// The maximum request size will be of BUFFER_SIZE.
	buffer = malloc(sizeof(char) * BUFFER_SIZE);
	bzero(buffer,BUFFER_SIZE);
	n = read(vars.newsockfd,buffer,BUFFER_SIZE);

	// Check that we received the message successfully.
	if (n < 0) 
	{
		perror("ERROR reading from socket. Make sure you aren't sending\n\
				a request that is too large.");
		exit(1);
	}

	// Parse the message from our client into a request.
	request_t request = parseRequest(buffer, vars.root_dir);

	// Respond to the request accordingly.
	respond(request, vars.newsockfd);

	// Close the connection and exit our thread.
	close(vars.newsockfd);
	pthread_exit(NULL);
}

request_t parseRequest(char *raw_request, char *root_dir) {
	// This should take a request and parse it into the request_t struct
	char *tmp_path;
	char *file_path;
	char *version;
	int test_code;
	request_t request;
	
	// Malloc all our strings
	tmp_path = malloc(sizeof(char) * strlen(raw_request));
	file_path = malloc(sizeof(char) * strlen(raw_request));
	version = malloc(sizeof(char) * VERSION_LEN);

	// Scan the request for the path
	sscanf(raw_request, "GET %s HTTP/%s %*[A-z0-9/:\n]", tmp_path, version);
	sprintf(file_path, "%s%s", root_dir, tmp_path);
	test_code = checkPath(file_path);

	// If our temp file path is NULL, then we have a bad request (not GET)
	// Otherwise set the necessary variables in the request struct.
	if (tmp_path == NULL) {
		request.code = BAD_REQUEST;
		request.filepath = NULL;
		request.version = version;
	} 
	else if (test_code == NOT_FOUND) {
		// 404 Error
		request.code = NOT_FOUND;
		request.filepath = NULL;
		request.version = version;
	}
	else if (test_code == OK) {
		// 200 good request
		request.code = OK;
		request.filepath = malloc(sizeof(char) * strlen(raw_request));
		request.version = version;
		strcpy(request.filepath, file_path);
	}

	return request;
};

void respond(request_t request, int newsockfd) {
	// Respond to the request!

	// First we build the header and write it
	char* response = buildResponse(request);
	write(newsockfd, response, strlen(response));

	// Then we send the file contents
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
    unsigned char *buffer = (unsigned char *) malloc(length+1);
    fread(buffer,length,sizeof(unsigned char),file);

    write(newsockfd, buffer, length);
    fclose(file);
}


void printRequest(request_t request) {
	printf("Code: %d\n", request.code);
	printf("Path: %s\n", request.filepath);
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