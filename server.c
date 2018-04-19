/*  Computer Systems COMP30023 Part A
*   name: Samuel Xu 
*   studentno: #835273
*   email: samuelx@student.unimelb.edu.au
*   login: samuelx
*
*   Using the provided server.c code given in Lab Week 3
*
*   This is a simple HTTP server program which should serve certain content 
*   with a given GET request.
*   It simply returns HTTP200 response and a file if it's found, and 404 if
*   not.
*   This server uses a basic implementation of Pthreads to process incoming
*   requests and sending messages. 
*
*   Style Notes:
*   Following the provided server.c style, we'll be doing the following:
*       - Character ruler of 78 characters. This allows us to read in consoles
*           like VI or nano without word wrap
*
*       - Use underscores when there are names_with_spaces, in both functions
*           and variables
*
*       - We'll be putting asterisks before the variable names, not the type
*           (allows us to define pointers and normal types in the same line)
*
*       - #defines are ALL_CAPITAL_LETTERS
*
*       - Convert tabs to spaces
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
#include <assert.h>

    /*  Sizes   */
#define CONN_MAX        10      // Maximum connections accepted
#define BUFFER_SIZE     512     // Size of buffer (used for requests/response)
#define FILE_TYPE_LEN   32      // Maximum file type length
#define OP_SIZE         4       // Maximum length of primitives

    /*  Status Codes    */
#define BAD_REQUEST     400 
#define NOT_FOUND       404
#define OK              200

    /*  Types   */
#define HTML_TYPE   "html"
#define HTML_MIME   "text/html"
#define CSS_TYPE    "css"
#define CSS_MIME    "text/css"
#define JS_TYPE     "js"
#define JS_MIME     "text/javascript"
#define JPG_TYPE    "jpg"
#define JPG_MIME    "image/jpeg"

    /*  Responses   */
#define GET                     "GET"
#define NOT_FOUND_RESPONSE      "HTTP/1.0 404\n"
#define BAD_REQUEST_RESPONSE    "HTTP/1.0 400\n"
#define RESPONSE_HEADER         "HTTP/1.0 200 OK\nContent-Type:"

typedef struct {
    char* file_path;
    int code;
} request_t;

typedef struct {
    char* root_dir;
    int socket_file_desc;
    int thread;
} thread_input_t;

void *connection_handler(void *args);
request_t parse_request(char *raw_request, char *root_dir);
int check_path(char *file_path);
char *get_file_type(char *file_path);
char *build_response(request_t request);
void send_content(char *file_path, int socket_file_desc);
void respond(request_t request, int socket_file_desc);

static void *safe_malloc(size_t size) {
    // This malloc checks if a malloc has completed successfully before
    // continuing
    void *pointer = malloc(size);
    if (!pointer) {
        perror("Bad malloc, out of memory!\n");
        exit(1);
    }

    return pointer;
}

int main(int argc, char **argv)
{
    // Main should read input and start the necessary servers with Pthread
    // This should take in 2 command line arguments:
    // Port number and string path to root web dir

    int sockfd, portno;                     // socket file descriptor & port
    struct sockaddr_in serv_addr, cli_addr; // server/client addresses
    socklen_t clilen;                       // Length of client address
    char* root_dir;
     

    // Check that the correct number of arguments have been supplied
    if (argc < 3) 
    {
        perror("ERROR, Incorrect number of arguments supplied.\n\
                Usage: server <port number> <path to content>\n");
        exit(1);
    }

    // Parse arguments
    portno = atoi(argv[1]);
    root_dir = (char *) safe_malloc(sizeof(argv[2])+1);
    strcpy(root_dir, argv[2]);
    
     // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }
    

    // Create an address that this machine is going to listen on
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);  

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
        int socket_file_desc = accept(sockfd, (struct sockaddr *) &cli_addr, 
                            &clilen);
        if (socket_file_desc < 0) 
        {
            perror("ERROR on accept");
            exit(1);
        }

        // Struct to hold variables for pthread
        thread_input_t input;           

        input.socket_file_desc = socket_file_desc;
        input.root_dir = 
                        (char *) safe_malloc(sizeof(char) * strlen(root_dir));
        strcpy(input.root_dir, root_dir);
        input.thread = thread_id;

        // Create a thread to handle the connection.
        pthread_create(&thread_id, NULL, connection_handler, (void *) &input);

        // Detach our thread once we've finished serving the connection.
        pthread_detach(thread_id);


        
    }
    
    // Close our socket and free remaining variables.
    close(sockfd);
    free(root_dir);

    return 0; 
}

void *connection_handler(void *args) {
    // This handles incoming requests, parses them and responds with the
    // specified file or error code.

    int n;
    char buffer[BUFFER_SIZE];

    // Cast our void pointer back to struct so we can get the arguments
    // passed in from main()
    thread_input_t vars = *((thread_input_t *) args);

    // Read in characters from our client.
    // The maximum request size will be of BUFFER_SIZE.
    bzero(buffer, BUFFER_SIZE);
    n = read(vars.socket_file_desc, buffer, BUFFER_SIZE);

    // Check that we received the message successfully.
    if (n < 0) 
    {
        perror("ERROR reading from socket. Make sure you aren't sending\n\
                a request that is too large.");
        exit(1);
    }

    // Parse the message from our client into a request.
    request_t request = parse_request(buffer, vars.root_dir);

    // Respond to the request accordingly.
    respond(request, vars.socket_file_desc);


    // Free memory
    free(vars.root_dir);
    
    // Close the connection and exit our thread.
    close(vars.socket_file_desc);
    pthread_exit(NULL);

}

request_t parse_request(char *raw_request, char *root_dir) {
    // This should take a request and parse it into the request_t struct.
    char *tmp_path;
    char *file_path;
    char primitive[OP_SIZE];
    int test_code;
    request_t request;
    
    // safe_malloc all our strings.
    tmp_path = (char *) safe_malloc(sizeof(char) * strlen(raw_request));
    file_path = (char *) safe_malloc(sizeof(char) * strlen(raw_request));

    // Scan the request for the path.
    // It will ignore all characters after the primitive and the path.
    sscanf(raw_request, "%s %s %*[A-z0-9/:\n]", primitive, tmp_path);
    sprintf(file_path, "%s%s", root_dir, tmp_path);

    // If we don't have a GET request, respond with a 400 bad response error
    if (strcmp(primitive, GET) != 0) {
        request.code = BAD_REQUEST;
        request.file_path = NULL;
        return request;
    } 

    // Check if our file actually exists on the server
    test_code = check_path(file_path);

    if (test_code == NOT_FOUND) {
        // 404 Error
        request.code = NOT_FOUND;
        request.file_path = NULL;
    }
    else if (test_code == OK) {
        // 200 good request
        request.code = OK;
        request.file_path = 
                    (char *) safe_malloc(sizeof(char) * strlen(raw_request));
        strcpy(request.file_path, file_path);
    }

    free(tmp_path);
    free(file_path);

    return request;
};


char *build_response(request_t request) {
    // This builds a response to send from a specified request

    char *file_type = NULL;
    char *response = NULL;

    if (request.code == 400) {
        // If we have a request we can't handle, then return a 400 response
        response = safe_malloc(sizeof(char) * BUFFER_SIZE);
        sprintf(response, BAD_REQUEST_RESPONSE);
    }
    else if (request.code == 404) {
        // If we have a invalid path request, then return a 404 response 
        response = safe_malloc(sizeof(char) * BUFFER_SIZE);
        sprintf(response, NOT_FOUND_RESPONSE);
    }
    else if (request.code == 200) {
        // Otherwise, return a 200 OK response
        file_type = get_file_type(request.file_path);   
        response = safe_malloc(sizeof(char) * BUFFER_SIZE + FILE_TYPE_LEN);
        sprintf(response, "%s %s\n\n", RESPONSE_HEADER, file_type);

        // Free our file type, since we don't need it anymore.
        free(file_type);
    }

    
    return response;
}

char *get_file_type(char* file_path) {
    // This function gets the file type of the requested file

    char tmp[FILE_TYPE_LEN];
    char *file_type;

    file_type = safe_malloc(FILE_TYPE_LEN);

    // Scan the specified file path for our file type
    // (i.e. ignore all characters up until the . character)
    sscanf(file_path, "./%*[A-z0-9/:\n].%s", tmp);
    if (tmp == NULL) {
        // Try again if the path does not include a './' at the beginning
        sscanf(file_path, "%*[A-z0-9/:\n].%s", tmp);
    }

    // Translate the file type into the correct MIME format.
    if ((strcmp(tmp, HTML_TYPE) == 0)) {
        strcpy(file_type, HTML_MIME);
    }
    else if (strcmp(tmp, CSS_TYPE) == 0) {
        strcpy(file_type, CSS_MIME);
    }
    else if (strcmp(tmp, JS_TYPE) == 0) {
        strcpy(file_type, JS_MIME);
    }
    else if (strcmp(tmp, JPG_TYPE) == 0) {
        sprintf(file_type, JPG_MIME);
    }

    return file_type;
}

int check_path(char *file_path) {
    // This function checks if the file in the path exists.
    // It will return the necessary error code if the file does not exist,
    // otherwise return 200.

    if (fopen(file_path, "r") == NULL) {
        return NOT_FOUND;
    }
    else {
        return OK;
    }
}

void respond(request_t request, int socket_file_desc) {
    // This function responds to the request!

    // First we build the header and write it
    char *response = build_response(request);
    write(socket_file_desc, response, strlen(response));

    // Then we send the file contents
    if (request.code == 200) {
        send_content(request.file_path, socket_file_desc);
    }

    // Free our strings
    free(request.file_path);
    free(response);
}

void send_content(char* file_path, int socket_file_desc) {
    // This function sends the contents of the requested file to the client

    // Open and check our file (with binary flag for images)
    FILE* file = fopen(file_path, "rb");
    assert(file);

    // Load the file into a buffer
    fseek(file, 0, SEEK_END);
    // Find the length of the file
    unsigned long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *buffer = (unsigned char *) safe_malloc(length+1);

    // Reac in our file to buffer
    fread(buffer,length,sizeof(unsigned char),file);

    // Write it to the socket!
    write(socket_file_desc, buffer, length);

    fclose(file);
}