#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

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
    char data[65536]; //64KB
}FileResponse;

int connection_handler(const char *SERVER_FIFO, const char *client_response_fifo ,int serverPID, int clientPID, CommandType command);
void list(const char *client_request_fifo, const char *client_response_fifo);

void help();
void help_command(const char *command);
int divide_string(char *string, char **tokens);
void readF(const char *client_request_fifo, const char *client_response_fifo, char ** tokens, int token_count);


int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        printf("Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "Connect") != 0 && strcmp(argv[1], "tryConnect") != 0) {
        printf("Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(1);
    }

    char SERVER_FIFO[256];
    sprintf(SERVER_FIFO, "/tmp/main_fifo_%s", argv[2]);
    
    // Create client-specific FIFOS
    char client_response_fifo[256];
    sprintf(client_response_fifo, "/tmp/client_response_fifo_%d", getpid());
    mkfifo(client_response_fifo, 0666);

    char client_request_fifo[256];
    sprintf(client_request_fifo, "/tmp/client_request_fifo_%d", getpid());
    mkfifo(client_request_fifo, 0666);

    int serverPID = atoi(argv[2]);
    CommandType command = strcmp(argv[1], "Connect") == 0 ? CONNECT : TRY_CONNECT;

    if (connection_handler(SERVER_FIFO, client_response_fifo, serverPID, getpid(), command) == -1) {
        return 0;
    }

    
    // Read terminal input
    while (1)
    {
        char input[256];
        printf("\n>> Enter command: ");
        fgets(input, 256, stdin);
        input[strlen(input) - 1] = '\0';
 
        char *tokens[4];
        int token_count = divide_string(input, tokens);

        for (int i = 0; i < token_count; i++) {
            printf("Token %d: %s\n", i, tokens[i]);
        }

        //printf("Token: %s\n", token);

        if (token_count == 0) {
            printf("Invalid command\n");
            continue;
        }

        if (strcmp(tokens[0], "quit") == 0) { 
            //printf("List command\n");
            list(client_request_fifo, client_response_fifo);
            continue;
        }

        if (strcmp(tokens[0], "help") == 0  && token_count == 1) {
            help();
            continue;
        }

        if (strcmp(tokens[0], "help") == 0) {
            help_command(tokens[1]);
            continue;
        }

        if (strcmp(tokens[0], "list") == 0) {
            list(client_request_fifo, client_response_fifo);
            continue;
        }

        if (strcmp(tokens[0], "readF") == 0) {
            readF(client_request_fifo, client_response_fifo, tokens, token_count);
            continue;
        }

        
        
    }
    
    unlink(client_response_fifo);
    unlink(client_request_fifo);
    return 0;
}

int connection_handler(const char *SERVER_FIFO, const char *client_response_fifo ,int serverPID, int clientPID, CommandType command) {
    
    int fd = open(SERVER_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("Failed to connect to server");
        exit(1);
    }

    Request connection_request;
    connection_request.requestType = CONNECTION_REQUEST;
    connection_request.PID = clientPID;
    connection_request.command = command;

    int bytes_written = write(fd, &connection_request, sizeof(Request));
    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    close(fd);

    int client_response_fd = open(client_response_fifo, O_RDONLY);
    if (client_response_fd == -1) {
        perror("Failed to open FIFO for reading");
        exit(1);
    }

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
    } else {
        printf("Server is full. You are rejected\n\n");
        close(client_response_fd);
        unlink(client_response_fifo);
        return -1;
    }

    close(client_response_fd);

    return 0;
}

void list(const char *client_request_fifo, const char *client_response_fifo) {


    int client_request_fd = open(client_request_fifo, O_WRONLY);
    if (client_request_fd == -1) {
        perror("Failed to open FIFO for writing");
        exit(1);
    }

    
    

    Request request;
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = LIST;

    printf("Sending request %d %d %d\n", request.PID, request.command, request.requestType);

    int bytes_written = write(client_request_fd, &request, sizeof(Request));

    if (bytes_written == -1) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    if (bytes_written == 0) {
        perror("Failed to write to FIFO");
        exit(1);
    }

    close(client_request_fd);

    ConnectionResponse response;

    int client_response_fd = open(client_response_fifo, O_RDONLY);
    if (client_response_fd == -1) {
        perror("Failed to open FIFO for reading");
        exit(1);
    }

    if (read(client_response_fd, &response, sizeof(ConnectionResponse)) == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }

    printf("\n%s\n", response.message);

    
    close(client_response_fd);

}

void help() {
    
    printf("Available comments are : \n");
    printf("help, list, readF, writeT, upload, download, archServer,quit, killServer\n\n");

}

void help_command (const char *command) {

    if (strcmp(command, "help") == 0) {
        printf("help: Display available commands\n");
    } else if (strcmp(command, "list") == 0) {
        printf("list: List all connected clients\n");
    } else if (strcmp(command, "readF") == 0) {
        printf("readF: Read a file from the server\n");
    } else if (strcmp(command, "writeF") == 0) {
        printf("writeF: Write a file to the server\n");
    } else if (strcmp(command, "upload") == 0) {
        printf("upload: Upload a file to the server\n");
    } else if (strcmp(command, "download") == 0) {
        printf("download: Download a file from the server\n");
    } else if (strcmp(command, "archServer") == 0) {
        printf("archServer: Archive the server\n");
    } else if (strcmp(command, "quit") == 0) {
        printf("quit: Disconnect from the server\n");
    } else if (strcmp(command, "killServer") == 0) {
        printf("killServer: Kill the server\n");
    } else {
        printf("Invalid command\n");
    }
}

int divide_string(char *string, char **tokens){
    
    int token_count = 0;
    char *token = strtok(string, " ");
    while (token != NULL) {
        tokens[token_count] = token;
        token_count++;
        token = strtok(NULL, " ");
    }

    return token_count;
}

void readF(const char *client_request_fifo, const char *client_response_fifo, char ** tokens, int token_count) {
    //printf("Read file\n");
    //printf("Token count: %d\n", token_count);

    
    
    if (token_count != 2 && token_count != 3) {
        printf("Too few or too many arguments\n");
        return;
    }

    if (tokens[1] == NULL) {
        printf("Invalid command\n");
        return;
    }

    Request request;
    request.requestType = REQUEST;
    request.PID = getpid();
    request.command = READFILE;
    strcpy(request.parameter1, tokens[1]);




    if (token_count == 3) {

        //printf("Token count: %d\n", token_count);
        int line = atoi(tokens[2]);
        if (line == 0) {
            printf("Invalid command Second parameter should be a number\n");
            return;
        }

        strcpy(request.parameter2, tokens[2]);

        int client_request_fd = open(client_request_fifo, O_WRONLY);
        if (client_request_fd == -1) {
            perror("Failed to open FIFO for writing");
            exit(1);
        }



        //printf("Sending request %d %d %d, %s, %s\n", request.PID, request.command, request.requestType, request.parameter1, request.parameter2);


        int bytes_written = write(client_request_fd, &request, sizeof(Request));
        if (bytes_written == -1) {
            perror("Failed to write to FIFO");
            exit(1);
        }

        close(client_request_fd);

        int client_response_fd = open(client_response_fifo, O_RDONLY);
        if (client_response_fd == -1) {
            perror("Failed to open FIFO for reading");
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
    }


}