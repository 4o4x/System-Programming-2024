#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


// Global variables
int request_global_fd = -1;
int response_global_fd = -1;
int serverPID_global = -1;
int global_file_fd = -1;
char global_response_fifo[256];
char global_request_fifo[256];
char SERVER_FIFO[256];




typedef enum requestType {
    CONNECTION_REQUEST = 0,
    CHILD_PARENT = 1,
    REQUEST = 2,
} RequestType;


typedef enum commandType {
    CONNECT = 0,
    TRY_CONNECT = 1,
    DISCONNECT = 2,
    KILL = 3,
    LIST = 4,
    READFILE = 5,
    WRITEFILE = 6,
    UPLOAD = 7,
    DOWNLOAD = 8,
    ARCH_SERVER = 9,
    QUIT = 10,

} CommandType;



// Request struct
typedef struct {
    RequestType requestType;
    int PID; // PID of the client or child
    CommandType command;
    char parameter1 [256];
    char parameter2 [256];
    char parameter3 [256];
}Request;


typedef enum responseStatus {
    SUCCESS = 0,
    IN_QUEUE = 1,
    SERVER_FULL = 2,
} ResponseStatus;

// Response struct
typedef struct {
    int clientPID;
    ResponseStatus status;
    char message[256];
    int serverPID;
} ConnectionResponse;


typedef struct{
    int totalChunks;
    int chunkNumber;
    int size;
    char data[32768];
    
}FileResponse;

/**
    Connect to the server
    @param SERVER_FIFO: server FIFO
    @param client_response_fifo: client response FIFO
    @param serverPID: server PID
    @param clientPID: client PID
    @param command: command type
    @return: 0 if successful, -1 if failed
*/
int connection_handler(const char *SERVER_FIFO, const char *client_response_fifo ,int serverPID, int clientPID, CommandType command);


/**
    List files on the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @return: list of files
*/  
char * list(int client_request_fd, int client_response_fd);


/**
    Help function
*/
void help();

/**
    Help command function
    @param command: command to display help for
*/
void help_command(const char *command);

/**
    Divide string into tokens
    @param string: input string
    @param tokens: array of tokens
    @param delim: delimiter
    @return: number of tokens
*/
int divide_string(char *string, char **tokens,const char *delim);

/**
    Read file from the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
*/
void readF(int client_request_fd, int client_response_fd, char **tokens, int token_count);

/**
    Write file to the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
    @param input: input string
*/
void writeF(int client_request_fd, int client_response_fd, char **tokens, int token_count, char *input);

/**
    Upload file to the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
*/
void upload(int client_request_fd, int client_response_fd, char **tokens, int token_count);

/**
    Download file from the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
    @param path: path to save the file
*/
void download(int client_request_fd, int client_response_fd, char **tokens, int token_count , char *path);

/**
    Archive server files
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
*/
void tar2(int client_request_fd, int client_response_fd, char **tokens, int token_count);

/**
    Quit the client
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
*/
void quit (int client_request_fd, int client_response_fd, char **tokens, int token_count);

/**
    Kill the server
    @param client_request_fd: file descriptor for writing to the server
    @param client_response_fd: file descriptor for reading from the server
    @param tokens: array of tokens of input
    @param token_count: number of tokens
*/
void killServer(int client_request_fd, int client_response_fd, char **tokens, int token_count);


/**
    * @brief Split the filename and extension
    * 
    * @param path the path of the file
    * @param file_name the file name
    * @param extension the extension of the file
    * @return int 
*/
int splitFilename(const char *path, char *file_name, char *extension); 


/**
    Signal handler for interrupt signal
    @param sig: signal number
*/
void interrupt_handler(int sig) {

    printf("Interrupt signal received. Exiting...\n");

    if (request_global_fd != -1) {
        close(request_global_fd);
    }

    if (response_global_fd != -1) {
        close(response_global_fd);
    }

    // TODO: Server PID global variable
    if (serverPID_global != -1) {
        kill(serverPID_global, SIGINT);
    }

    if (global_file_fd != -1) {
        close(global_file_fd);
    }

    unlink(global_request_fifo);
    unlink(global_response_fifo);
    
    exit(0);
}


int main(int argc, char *argv[]) {

    
    //////////////////////////////////////////
    // Signal handler for interrupt signal
    

    struct sigaction act;
    act.sa_handler = interrupt_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);

    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);


    //////////////////////////////////////////

    // Check arguments
    
    if (argc != 3) {
        printf("Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "Connect") != 0 && strcmp(argv[1], "tryConnect") != 0) {
        printf("Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(1);
    }

    if (atoi(argv[2]) <= 0) {
        printf("Invalid server PID\n");
        exit(1);
    }

    //////////////////////////////////////////

    
    sprintf(SERVER_FIFO, "/tmp/main_fifo_%s", argv[2]);


    //////////////////////////////////////////
    // Create client-specific FIFOS
    char client_response_fifo[256];
    sprintf(client_response_fifo, "/tmp/client_response_fifo_%d", getpid());
    mkfifo(client_response_fifo, 0666);

    char client_request_fifo[256];
    sprintf(client_request_fifo, "/tmp/client_request_fifo_%d", getpid());
    mkfifo(client_request_fifo, 0666);
    
    int serverPID = atoi(argv[2]);

    //////////////////////////////////////////

    //////////////////////////////////////////
    // Connect to the server

    CommandType command = strcmp(argv[1], "Connect") == 0 ? CONNECT : TRY_CONNECT;

    if (connection_handler(SERVER_FIFO, client_response_fifo, serverPID, getpid(), command) == -1) {
        return 0;
    }

    //////////////////////////////////////////


    //////////////////////////////////////////
    // Open client-specific FIFOS

    int client_request_fd = open(client_request_fifo, O_WRONLY);
    if (client_request_fd == -1) {
        perror("Failed to open FIFO for writing");
        exit(1);
    }

    request_global_fd = client_request_fd;

    int client_response_fd = open(client_response_fifo, O_RDONLY);

    if (client_response_fd == -1) {
        perror("Failed to open FIFO for reading");
        exit(1);
    }

    response_global_fd = client_response_fd;


    ConnectionResponse response;
    if (read(client_response_fd, &response, sizeof(ConnectionResponse)) == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    serverPID_global = response.serverPID;

    //////////////////////////////////////////

    //////////////////////////////////////////
    
    // Read terminal input
    while (1)
    {
        //////////////////////////////////////////
        // Read terminal input and process the command
        char input[256];
        printf("\n>> Enter command: ");
        fgets(input, 256, stdin);
        input[strlen(input) - 1] = '\0';

 
        char *tokens[4];
        int token_count = divide_string(strdup(input), tokens, " ");

        if (token_count == 0) {
            printf("Invalid command\n");
            continue;
        }
        //////////////////////////////////////////

        if (strcmp(tokens[0], "help") == 0  && token_count == 1) {
            help();
            continue;
        }

        if (strcmp(tokens[0], "help") == 0) {
            help_command(tokens[1]);
            continue;
        }

        if (strcmp(tokens[0], "list") == 0) {
            list(client_request_fd, client_response_fd);
            continue;
        }

        if (strcmp(tokens[0], "readF") == 0) {
            readF(client_request_fd, client_response_fd, tokens, token_count);
            continue;
        }

        if (strcmp(tokens[0], "writeF") == 0) {
            writeF(client_request_fd, client_response_fd, tokens, token_count, input);
            continue;
        }

        if (strcmp(tokens[0], "upload") == 0) {
            upload(client_request_fd, client_response_fd, tokens, token_count);
            continue;
        }

        if (strcmp(tokens[0], "download") == 0) {
            download(client_request_fd, client_response_fd, tokens, token_count, NULL);
            continue;
        }

        if (strcmp(tokens[0], "archServer") == 0) {
            tar2(client_request_fd, client_response_fd, tokens, token_count);
            continue;
        }

        if (strcmp(tokens[0], "quit") == 0) {
            quit(client_request_fd, client_response_fd, tokens, token_count);
            break;
        }
        
        if(strcmp(tokens[0], "killServer") == 0){
            printf("Killing server\n");
            killServer(client_request_fd, client_response_fd, tokens, token_count);
            break;
        }        
    }

    printf("Exiting...\n");

    close(client_request_fd);
    close(client_response_fd);
    
    unlink(client_response_fifo);
    unlink(client_request_fifo);
    return 0;
}

int connection_handler(const char *SERVER_FIFO, const char *client_response_fifo ,int serverPID, int clientPID, CommandType command) {
    
    Request connection_request;

    memset(&connection_request, 0, sizeof(Request));

    connection_request.requestType = CONNECTION_REQUEST;
    connection_request.PID = clientPID;
    connection_request.command = command;
    connection_request.parameter1[0] = '\0';
    connection_request.parameter2[0] = '\0';
    connection_request.parameter3[0] = '\0';


    int fd = open(SERVER_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("Failed to connect to server");
        exit(1);
    }

    global_file_fd = fd;

   
    int bytes_written = write(fd, &connection_request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    close(fd);
    global_file_fd = -1;

    int client_response_fd = open(client_response_fifo, O_RDONLY);
    if (client_response_fd == -1) {
        perror("Failed to open FIFO for reading");
        exit(1);
    }

    response_global_fd = client_response_fd;

    ConnectionResponse connection_response;

    if (read(client_response_fd, &connection_response, sizeof(ConnectionResponse)) == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    //printf("Response: %d %d %s %d\n", connection_response.clientPID, connection_response.status, connection_response.message, connection_response.serverPID);

    if (connection_response.status == SUCCESS) {
        printf("Connected to server\n\n");
    } else if (connection_response.status == IN_QUEUE) {
        printf("You are in the queue\n\n");

        close(client_response_fd);
        global_file_fd = -1;

        client_response_fd = open(client_response_fifo, O_RDONLY);
        if (client_response_fd == -1) {
            perror("Failed to open FIFO for reading");
            exit(1);
        }

        response_global_fd = client_response_fd;

        int read_bytes = read(client_response_fd, &connection_response, sizeof(ConnectionResponse));

        if (read_bytes == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        if (connection_response.status == SUCCESS) {
            printf("Connected to server\n\n");
        } else {
            printf("Failed to connect to server\n\n");
            close(client_response_fd);
            return -1;
        }


    } else {
        printf("Server is full. You are rejected\n\n");
        close(client_response_fd);
        unlink(client_response_fifo);
        return -1;
    }

    close(client_response_fd);
    response_global_fd = -1;

    return 0;
}

char * list(int client_request_fd, int client_response_fd) {

    // Have Done: memset and done

    Request request;
    memset(&request, 0, sizeof(Request));
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = LIST;


    int bytes_written = write(client_request_fd, &request, sizeof(Request));

    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    if (bytes_written == 0) {
        perror("Failed to write to FIFO");
        exit(1);
    }
    
    FileResponse response;

    int read_bytes = read(client_response_fd, &response, sizeof(FileResponse));

    if (read_bytes == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    while (read_bytes < sizeof(FileResponse)) {
        
        int remaining_bytes = sizeof(FileResponse) - read_bytes;
        printf("Remaining bytes: %d\n", remaining_bytes);
        char buffer[remaining_bytes];

        int read_again = read(client_response_fd, buffer, remaining_bytes);

        if (read_again == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        for (int i = 0; i < read_again; i++) {
            response.data[read_bytes + i] = buffer[i];
        }

        read_bytes += read_again;
    }

    if (response.totalChunks == -1) {
        printf("%s\n", response.data);
        return NULL;
    }

    char * tokens[32768];

    int token_count = divide_string(strdup(response.data), tokens, "\n");

    printf("\n");

    for (int i = 2; i < token_count; i++) {
        printf("    %s\n", tokens[i]);
    }


    return strdup(response.data);

}

void help() {
    
    printf("\n");
    printf("     Available commands are:\n");
    printf("           help <command#>\n");
    printf("           list\n");
    printf("           readF <filename> <line#>\n");
    printf("           writeF <filename> <line#> \"<string>\"\n");
    printf("           upload <filename>\n");
    printf("           download <filename>\n");
    printf("           archServer <tarfilename>\n");
    printf("           quit\n");
    printf("           killServer\n");
    printf("\n");

}

void help_command (const char *command) {

    if (strcmp(command, "help") == 0) {
        
        printf("\n");
        printf("     help <command#>\n");
        printf("           displays the help message\n");
        printf("           <command#> is the command to display help for (optional)\n");
        printf("           Available commands are: help, list, readF, writeF, upload, download, archServer, quit, killServer\n\n");
        printf("           Example: help list\n");
        printf("\n");

    } 
    
    else if (strcmp(command, "list") == 0) {
        printf("\n");
        printf("     list\n");
        printf("           List the files on the server\n");
        printf("\n");
    } 
    
    else if (strcmp(command, "readF") == 0) {
        
        printf("\n");
        printf("     readF <filename> <line#>\n");
        printf("           Read a file from the server\n");
        printf("           <filename> is the name of the file to read\n");
        printf("           <line#> is the line number to read (optional)\n");
        printf("\n");

    } 
    else if (strcmp(command, "writeF") == 0) {

        printf("\n");
        printf("     writeF <filename> <line#> \"<string>\"\n");
        printf("           Write a file to the server\n");
        printf("           <filename> is the name of the file to write\n");
        printf("           <line#> is the line number to write (optional)\n");
        printf("           <string> is the string to write\n");
        printf("\n");
       
    } 
    
    else if (strcmp(command, "upload") == 0) {
        printf("\n");
        printf("     upload <filename>\n");
        printf("           Upload a file to the server\n");
        printf("           <filename> is the name of the file to upload\n");
        printf("\n");
    } 
    
    else if (strcmp(command, "download") == 0) {
        
        printf("\n");
        printf("     download <filename>\n");
        printf("           Download a file from the server\n");
        printf("           <filename> is the name of the file to download\n");
        printf("\n");
    } 
    
    else if (strcmp(command, "archServer") == 0) {
        printf("\n");
        printf("     archServer <tarfilename>\n");
        printf("           Archive the server files\n");
        printf("           <tarfilename> is the name of the tar file\n");
        printf("\n");
    } 
    
    else if (strcmp(command, "quit") == 0) {
        printf("\n");
        printf("     quit\n");
        printf("           Quit the client\n");
        printf("\n");
    } 
    
    else if (strcmp(command, "killServer") == 0) {
        printf("\n");
        printf("     killServer\n");
        printf("           Kill the server\n");
        printf("\n");
    } 
    
    else {
        printf("Invalid command\n");
    }
}

int divide_string(char *string, char **tokens,const char *delim){
    
    int token_count = 0;
    char *token = strtok(string, delim);
    
    while (token != NULL) {
        tokens[token_count] = token;
        token_count++;
        token = strtok(NULL, delim);
    }

    return token_count;
}

void readF(int client_request_fd, int client_response_fd, char **tokens, int token_count) {
    printf("\n");
    //printf("Token count: %d\n", token_count);

    // TODO: Yeni fileresponse structına före düzenleme yapılacak
    
    if (token_count != 2 && token_count != 3) {
        printf("Too few or too many arguments\n");
        return;
    }

    if (tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }

    Request request;

    memset(&request, 0, sizeof(Request));

    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = READFILE;
    strcpy(request.parameter1, tokens[1]);




    if (token_count == 3) {

        //printf("Token count: %d\n", token_count);
        int line = atoi(tokens[2]);
        if (line <= 0) {
            printf("Invalid command Second parameter should be a number and greater than 0\n");
            return;
        }

        strcpy(request.parameter2, tokens[2]);


        int bytes_written = write(client_request_fd, &request, sizeof(Request));
        if (bytes_written == -1) {
            perror("Failed to write to FIFO");
            exit(1);
        }


        FileResponse response;

        if (read(client_response_fd, &response, sizeof(FileResponse)) == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        printf("\n%s\n", response.data);

              
    }

    else {
        strcpy(request.parameter2, "-1");

        int bytes_written = write(client_request_fd, &request, sizeof(Request));
        if (bytes_written == -1) {
            perror("Failed to write to FIFO");
            exit(1);
        }

        FileResponse response;

        if (read(client_response_fd, &response, sizeof(FileResponse)) == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        if (response.totalChunks == -1) {
            printf("%s\n", response.data);
            return;
        }

        int total = response.totalChunks;

        //printf("Total chunks: %d\n", total);

        for (int i = 0; i < total; i++) {
               
            int read_bytes = read(client_response_fd, &response, sizeof(FileResponse));

            if (read_bytes == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            while (read_bytes < sizeof(FileResponse)) {
                
                int remaining_bytes = sizeof(FileResponse) - read_bytes;
                printf("Remaining bytes: %d\n", remaining_bytes);
                char buffer[remaining_bytes];

                int read_again = read(client_response_fd, buffer, remaining_bytes);

                if (read_again == -1) {
                    perror("Failed to read from FIFO");
                    exit(1);
                }

                for (int i = 0; i < read_again; i++) {
                    response.data[read_bytes + i] = buffer[i];
                }

                read_bytes += read_again;
            }

            int stdout_fd_write = write(1, response.data, response.size);

            if (stdout_fd_write == -1) {
                perror("Failed to write to stdout");
                exit(1);
            }
        }
        
    }
}

void writeF(int client_request_fd, int client_response_fd, char **tokens, int token_count,char *input) {

    printf("\n");

    //printf("Token count: %d\n", token_count);

    if (token_count == 2 || token_count == 1) {
        printf("Too few or too many arguments\n");
        return;
    }

    if(strchr(input, '\"') == NULL){
        printf("Invalid command\n");
        return;
    }
    
    char * new_tokens[32768];

    int new_token_count = divide_string(input, new_tokens, "\"");

    //printf("New token count: %d\n", new_token_count);

    char * commad_tokens[3];

    int command_token_count = divide_string(new_tokens[0], commad_tokens, " ");


    if (command_token_count != 2 && command_token_count != 3) {
        printf("Too few or too many arguments\n");
        return;
    }


    if (commad_tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }
    
    int line = -1;
        
    if (command_token_count == 3) {
        if (commad_tokens[2] == NULL) {
            printf("Invalid command\n");
            return;
        }

        line = atoi(commad_tokens[2]);

        if (line <= 0) {
            printf("Invalid command Second parameter should be a number and greater than 0\n");
            return;
        }
    }


    //printf("First Request\n");

    Request request;
    memset(&request, 0, sizeof(Request));

    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = WRITEFILE;

    strcpy(request.parameter1, commad_tokens[1]);
    
    if (line != -1) {
        strcpy(request.parameter2, commad_tokens[2]);
    } else {
        strcpy(request.parameter2, "-1");
    }



    //printf("Sending request %d %d %d\n", request.PID, request.command, request.requestType);
    int bytes_written = write(client_request_fd, &request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    FileResponse request_response;

    //printf("First Response\n");

    int request_read = read(client_response_fd, &request_response, sizeof(FileResponse));

    if (request_read == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    if (request_response.totalChunks == -1) {
        printf("%s\n", request_response.data);
        return;
    }
    

    //printf("Total chunks: %d\n", request_response.totalChunks);


    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));
    response.totalChunks = 1;
    response.chunkNumber = 0;
    response.size = strlen(new_tokens[1]);
    strcpy(response.data, new_tokens[1]);
    
    int write_bytes = write(client_request_fd, &response, sizeof(FileResponse));

    if (write_bytes == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }


    FileResponse response2;

    int read_bytes = read(client_response_fd, &response2, sizeof(FileResponse));

    if (read_bytes == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    while (read_bytes < sizeof(FileResponse)) {
        
        int remaining_bytes = sizeof(FileResponse) - read_bytes;
        printf("Remaining bytes: %d\n", remaining_bytes);
        char buffer[remaining_bytes];

        int read_again = read(client_response_fd, buffer, remaining_bytes);

        if (read_again == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        for (int i = 0; i < read_again; i++) {
            response2.data[read_bytes + i] = buffer[i];
        }

        read_bytes += read_again;
    }

    printf("%s\n", response2.data);
    return;    
}

void upload(int client_request_fd, int client_response_fd, char **tokens, int token_count) {
    
    if (token_count != 2) {
        printf("Too few or too many arguments\n");
        return;
    }

    if (tokens[0] == NULL || tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }

    int file = open(tokens[1], O_RDONLY);
    if (file == -1) {
        perror("Failed to open file");
        return;
    }


    Request request;
    memset(&request, 0, sizeof(Request));
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = UPLOAD;
    strcpy(request.parameter1, tokens[1]);

    //printf("Sending request %d %d %d\n", request.PID, request.command, request.requestType);
    
    int bytes_written = write(client_request_fd, &request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }


    struct stat file_stat;

    if (fstat(file, &file_stat) == -1) {
        perror("Failed to get file stats");
        return;
    }

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));

    int file_size = file_stat.st_size;

    

    int total_chunks = file_size / 32768 + 1;

    //printf("Total chunks: %d\n", total_chunks);

    if (file_size % 32768 == 0) {
        total_chunks--;
    }

    int last_chunk_size = file_size % 32768;

    //printf("Total chunks: %d\n", total_chunks);

    response.totalChunks = total_chunks;
    response.chunkNumber = 0;
    response.data[0] = '\0';

    bytes_written = write(client_request_fd, &response, sizeof(FileResponse));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    for (int i = 0; i < total_chunks; i++) {


        // if it is the last chunk then set the size to the last_chunk_size
        if (i == total_chunks - 1) {
            response.size = last_chunk_size;
        } else {
            response.size = 32768;
        }


        int bytes_read = read(file, response.data, 32768);

        //printf("Bytes read: %d\n", bytes_read);

        
        if (bytes_read == 0) {
            break;
        }

        if (bytes_read == -1) {
            perror("Failed to read from file");
            return;
        }


        // if all bytes are not read from the file then read again
        while(bytes_read < response.size) {

            int reamining_bytes = response.size - bytes_read;
            char buffer[reamining_bytes];
            
            int read_again = read(file, buffer, reamining_bytes);

            if (read_again == -1) {
                perror("Failed to read from file");
                return;
            }

            for (int i = 0; i < read_again; i++) {
                response.data[bytes_read + i] = buffer[i];
            }

            bytes_read += read_again;
        }

        bytes_written = write(client_request_fd, &response, sizeof(FileResponse));
        
        if (bytes_written == -1) {
            perror("Failed to write to FIFO");
            exit(1);
        }

        //printf("Bytes written: %d\n", bytes_written);

        response.chunkNumber++;
    }

    close(file);

    printf("File uploaded successfully\n");
    printf("%d bytes is uploaded\n", file_size);

    if (read(client_response_fd, &response, sizeof(FileResponse)) == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    //printf("%s\n", response.data);

    return;

}

void download(int client_request_fd, int client_response_fd, char **tokens, int token_count, char *path) {
    
    printf("\n");
    if (token_count != 2) {
        printf("Too few or too many arguments\n");
        return;
    }

    if (tokens[0] == NULL || tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }


    /////////////////////////////////////////////////////////////////
    // File creation

   

    Request request;
    memset(&request, 0, sizeof(Request));
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = DOWNLOAD;
    strcpy(request.parameter1, tokens[1]);

    int bytes_written = write(client_request_fd, &request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    // Chunk response

    FileResponse response;

    int read_chunk = read(client_response_fd, &response, sizeof(FileResponse));

    if (read_chunk == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    //printf("Read chunk: %d\n", read_chunk);

    while (read_chunk < sizeof(FileResponse)) {
        
        int remaining_bytes = sizeof(FileResponse) - read_chunk;
        //printf("Remaining bytes: %d\n", remaining_bytes);
        char buffer[remaining_bytes];

        int read_again = read(client_response_fd, buffer, remaining_bytes);

        if (read_again == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        for (int i = 0; i < read_again; i++) {
            response.data[read_chunk + i] = buffer[i];
        }

        read_chunk += read_again;
    }


    if (response.totalChunks == -1) {
        printf("%s\n", response.data);
        return;
    }
    
    /////////////////////////////////////////////////////////////////
     int file_fd ;
    int counter = 0;
    char file_name[512];

    if (path == NULL) {
        sprintf(file_name, "%s", tokens[1]);
    } else {
        sprintf(file_name, "%s/%s", path, tokens[1]);
    }

    char *temp = strdup(file_name);

    while (file_fd = open(temp, O_WRONLY | O_CREAT | O_EXCL, 0666) , file_fd == -1) {

        if (errno == EEXIST) {
            counter++;
            char temp_file_name[256];
            char temp_file_extension[64];
            int return_value = splitFilename(file_name, temp_file_name, temp_file_extension);

            if (return_value == -1) {
                sprintf(temp, "%s_%d", temp_file_name, counter);
            }

            else{
                sprintf(temp, "%s_%d.%s", temp_file_name, counter, temp_file_extension);
            }
           
        } else {
            perror("Failed to open file");
            return;
        }
    }

    //printf("File fd: %d\n", file_fd);

    global_file_fd = file_fd;


    /////////////////////////////////////////////////////////////////
    // Reading chunks

    int total = response.totalChunks;

    printf("Download %s(%d Byte) started\n", tokens[1], response.size);

    //printf("Total chunks: %d\n", total);

    for (int i = 0; i < total; i++) {
        
        /////////////////////////////
        // Reading chunk
        int read_bytes = read(client_response_fd, &response, sizeof(FileResponse));

        //printf("Read bytes: %d\n", read_bytes);

        if (read_bytes == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        while (read_bytes < sizeof(FileResponse)) {
            
            int remaining_bytes = sizeof(FileResponse) - read_bytes;
            //printf("Remaining bytes: %d\n", remaining_bytes);
            char buffer[remaining_bytes];

            int read_again = read(client_response_fd, buffer, remaining_bytes);

            if (read_again == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            for (int i = 0; i < read_again; i++) {
                response.data[read_bytes + i] = buffer[i];
            }

            read_bytes += read_again;
        }

        /////////////////////////////

        /////////////////////////////
        // Writing chunk to file

        //printf("Size: %d\n", response.size);
        
        int bytes_written = write(file_fd, response.data, response.size);

        if (bytes_written == -1) {
            perror("Failed to write to file");
            exit(1);
        }

        //printf("Bytes written: %d\n", bytes_written);

        /////////////////////////////
        
    }

    close(file_fd);

    global_file_fd = -1;

    /////////////////////////////////////////////////////////////////

    printf("File downloaded successfully\n");

    //sleep(25);

    return;

} 

void tar2(int client_request_fd, int client_response_fd, char **tokens, int token_count){

    printf("\n");
    if (token_count != 2) {
        printf("Too few or too many arguments\n");
        return;
    }

    if (tokens[0] == NULL || tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }

    char tar_file[256];
    sprintf(tar_file, "%s", tokens[1]);

    char *list_of_files = list(client_request_fd, client_response_fd);

    //printf("List of files: %s\n", list_of_files);

    char *files[256];

    int file_count = divide_string(list_of_files, files, "\n");

    if (file_count == 0) {
        printf("No files to archive\n");
        return;
    }

    //Create directory for the tar file
    char* file_tokens[2];

    char directory_name[256];
    

    int file_token_count = divide_string(tokens[1], file_tokens, ".");
    sprintf(directory_name, "%s", file_tokens[0]);

    printf("Creating directory %s\n", directory_name);
    

    if (file_token_count != 2) {
        printf("Invalid file name\n");
        return;
    }

    if (strcmp(file_tokens[1], "tar") != 0) {
        printf("Invalid file type\n");
        return;
    }

    int mkdir_status = mkdir(directory_name, 0777);

    if (mkdir_status == -1) {
        perror("Failed to create directory");
        return;
    }

    char * token_for_download[2];

    for (int i = 2; i < file_count; i++) {
        token_for_download[0] = "download";
        token_for_download[1] = files[i];
        download(client_request_fd, client_response_fd, token_for_download, 2, directory_name);
    }
    
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork");
        return;
    }

    if (pid == 0) {

        printf("Calling tar utility .. child PID %d\n", getpid());
        
        char * args[4];
        args[0] = "tar";
        args[1] = "-cf";
        args[2] = tar_file;
        args[3] = directory_name;
        args[4] = NULL;

        int exec_status = execvp("tar", args);

        if (exec_status == -1) {
            perror("Failed to execute tar");
            exit(1);
        }

        exit(0);

    }

    int status;

    
    //Wait for the tar process to complete

    waitpid(pid, &status, 0);


    if (WIFEXITED(status)) {
        printf("child returned with SUCCESS...\n");
    } else {
        printf("Tar failed\n");
    }

    //Remove the directory

    for (int i = 2; i < file_count; i++) {
        char remove_path[512];
        snprintf(remove_path, 512, "%s/%s", directory_name, files[i]);
        int remove_status = remove(remove_path);

        if (remove_status == -1) {
            perror("Failed to remove file");
            return;
        }
    }

    int remove_status = rmdir(directory_name);

    if (remove_status == -1) {
        perror("Failed to remove directory");
        return;
    }

    printf("Directory removed\n");

    return;
}

void quit (int client_request_fd, int client_response_fd, char **tokens, int token_count) {
    
    if (token_count != 1) {
        printf("Too few or too many arguments\n");
        return;
    }

    Request request;
    memset(&request, 0, sizeof(Request));
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = QUIT;

    int bytes_written = write(client_request_fd, &request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));

    if (read(client_response_fd, &response, sizeof(FileResponse)) == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    printf("%s\n", response.data);

    return;
}
        
void killServer(int client_request_fd, int client_response_fd, char **tokens, int token_count) {

    if (token_count != 1) {
        printf("Too few or too many arguments\n");
        return;
    }

    Request request;
    memset(&request, 0, sizeof(Request));
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = KILL;
    request.parameter1[0] = '\0';
    request.parameter2[0] = '\0';
    request.parameter3[0] = '\0';

    int main_fd = open(SERVER_FIFO, O_WRONLY);
    if (main_fd == -1) {
        perror("Failed to open FIFO for writing");
        exit(1);
    }

    int bytes_written = write(main_fd, &request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    close(main_fd);
   
}


int splitFilename(const char *path, char *file_name, char *extension){
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) {
        return -1;
    } else {       
        strcpy(extension, dot + 1);
        int file_name_length = dot - path;
        strncpy(file_name, path, file_name_length);
        file_name[file_name_length] = '\0'; 
        return 0; 
    }
}


