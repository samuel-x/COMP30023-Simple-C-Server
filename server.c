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

int main(int argc, char **argv)
{
	// Main should read input and start the necessary servers with Pthread
	// This should take in 2 command line arguments:
	// Port number and string path to root web dir

	int sockfd, newsockfd, portno;// clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	int n;

	// Get current dir for later relative response
	char* root_dir;

	// Check that the correct number of arguments have been supplied
	if (argc < 3) 
	{
		fprintf(stderr,"ERROR, Incorrect number of arguments supplied.\n
						Usage: ./server <port number> <path to content>\n");
		exit(1);
	}

	// Parse arguments
	portno = atoi(argv[1]);
	strcopy(root_dir, argv[2]);
	
	 /* Create TCP socket */
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

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
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */
	
	listen(sockfd,5);
	
	clilen = sizeof(cli_addr);

	/* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */

	newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, 
						&clilen);

	if (newsockfd < 0) 
	{
		perror("ERROR on accept");
		exit(1);
	}
	
	bzero(buffer,256);

	/* Read characters from the connection,
		then process */
	
	n = read(newsockfd,buffer,255);

	/* Parse input */



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
	
	/* close socket */
	
	close(sockfd);
	
	return 0; 
}