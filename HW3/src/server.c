#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
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

// Response struct
typedef struct {
    int clientPID;
    int status; // 0: Success | 1: You are in the queue | 2: Server is full
    char message[256];
    int serverPID;
} ConnectionResponse;

typedef struct{
    int totalChunks;
    int chunkNumber;
    char data[65536]; //64KB
}FileResponse;


typedef struct {
    int front, rear, size;
    unsigned capacity;
    int *array;
} Queue;

Queue *createQueue(unsigned capacity);
int isFull(Queue *queue);
int isEmpty(Queue *queue);
void enqueue(Queue *queue, int item);
int dequeue(Queue *queue);
int front(Queue *queue);
int rear(Queue *queue);





void setup_directory(const char *dirname);

int client_child(int clientPID, int serverPID,const char *dirname);

void quit_child(int clientPID, int serverPID,int client_fd);

char* get_file_names(const char* directory_path);

void list(char *clientResponseFIFO, const char *dirname);

void readFile(char *clientResponseFIFO, const char *dirname, Request request);



void signal_handler(int sig) {
    // Handle signal (e.g., SIGINT for graceful shutdown)
    // Cleanup resources
    exit(0);
}

int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        printf("Usage: %s <dirname> <max.#ofClients>\n", argv[0]);
        exit(1);
    }

    char MAIN_FIFO[256];
    sprintf(MAIN_FIFO, "/tmp/main_fifo_%d", getpid());
    mkfifo(MAIN_FIFO, 0666);

    const char *dirname = argv[1];
    setup_directory(dirname);

    const int MAX_CLIENTS = atoi(argv[2]);
    int clientCount = 0;
    int client_count_all = 0;

    printf("Server PID: %d\n", getpid());
    
    signal(SIGINT, signal_handler);

    

    Queue *clientQueue = createQueue(MAX_CLIENTS);
   
    
    // Main server loop
    while (1) {
        
        Request request;
        

        int fd = open(MAIN_FIFO, O_RDONLY);
        if (fd == -1) {
            printf("Failed to open FIFO\n");
            exit(1);
        }

        int bytes_read = read(fd, &request, sizeof(Request));

        if (bytes_read == -1) {
            printf("Failed to read from FIFO\n");
            exit(1);
        }

         

        printf("Request: %d %d %d \n", request.PID, request.command, request.requestType);

        if (request.requestType == CONNECTION_REQUEST){
            
            ConnectionResponse response;
            char clientFIFO[256];
            sprintf(clientFIFO, "/tmp/client_response_fifo_%d", request.PID);

            int client_fd = open(clientFIFO, O_WRONLY);
            if (client_fd == -1) {
                printf("Failed to open client FIFO\n");
                exit(1);
            }
            
            // Server is full and client wants to connect with tryConnect
            if (MAX_CLIENTS == clientCount && request.command == TRY_CONNECT) {
                printf("Client PID: %d Server is full. You are rejected\n", request.PID);
                response.clientPID = request.PID;
                response.status = 2;
                sprintf(response.message, "Server is full");
                response.serverPID = getpid();
                write(client_fd, &response, sizeof(ConnectionResponse));
                close(client_fd);
                close(fd);
                continue;
            }
            
            if (MAX_CLIENTS == clientCount && request.command == CONNECT) {

                printf("Client PID: %d Server is full. You are in the queue. Order: %d\n", request.PID, clientQueue->size + 1);
                response.clientPID = request.PID;
                response.status = 1;
                sprintf(response.message, "You are in the queue");
                response.serverPID = getpid();
                write(client_fd, &response, sizeof(ConnectionResponse));
                close(client_fd);
                close(fd);
                enqueue(clientQueue, request.PID);
                continue;
            }

            printf("Client PID: %d connected as \"client_%d\"\n", request.PID, client_count_all);


            response.clientPID = request.PID;
            response.status = 0;
            sprintf(response.message, "Success");
            response.serverPID = getpid();

            int byte = write(client_fd, &response, sizeof(ConnectionResponse));

            if (byte == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }
            printf("Client PID: %d connected as \"client_%d\"\n", request.PID, client_count_all);
            clientCount++;
            client_count_all++;
            close(client_fd);
            
        }

        close(fd);


        printf("Client_child\n");
       

        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork");
            exit(1);
        }

        if (pid == 0) {
            client_child(request.PID, getpid(),dirname);
        }

    }

    return 0;
}


void setup_directory(const char *dirname) {
    mkdir(dirname, 0777); // Create directory with full access permissions
}

Queue *createQueue(unsigned capacity) {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (int *)malloc(queue->capacity * sizeof(int));
    return queue;
}

int isFull(Queue *queue) {
    return (queue->size == queue->capacity);
}

int isEmpty(Queue *queue) {
    return (queue->size == 0);
}

// add an item to the queue if is full double the size of the queue
// return location of the item in the queue

void enqueue(Queue *queue, int item) {
    if (isFull(queue)) {
        queue->capacity *= 2;
        queue->array = (int *)realloc(queue->array, queue->capacity * sizeof(int));
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;

  
}

int dequeue(Queue *queue) {
    if (isEmpty(queue)) {
        return -1;
    }
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

int front(Queue *queue) {
    if (isEmpty(queue)) {
        return -1;
    }
    return queue->array[queue->front];
}

int rear(Queue *queue) {
    if (isEmpty(queue)) {
        return -1;
    }
    return queue->array[queue->rear];
}


int client_child(int clientPID, int serverPID,const char *dirname) {
    
    
    printf("Child Process\n");
    
    char clientRequestFIFO[256];
    sprintf(clientRequestFIFO, "/tmp/client_request_fifo_%d", clientPID);
    char clientResponseFIFO[256];
    sprintf(clientResponseFIFO, "/tmp/client_response_fifo_%d", clientPID);



    while (1)
    {
        int client_request_fd = open(clientRequestFIFO, O_RDONLY);

        if (client_request_fd == -1) {
            perror("Failed to open client FIFO for reading");
            exit(1);
        }

        
        
        Request request;
    
        int bytes_read = read(client_request_fd, &request, sizeof(Request));

        if (bytes_read == -1) {
            perror("Failed to read from client FIFO");
            exit(1);
        }

        printf("Request: %d %d %d \n", request.PID, request.command, request.requestType);

        if (bytes_read == 0) {
            printf("Yazık\n");
            exit(1);
        }

        close(client_request_fd);
        

        if (request.command == QUIT) {
            
            printf("Client PID: %d -> QUIT\n", clientPID);
            quit_child(clientPID, serverPID,client_request_fd);
            close(client_request_fd);
            exit(0);
        }

        else if (request.command == LIST) {
            printf("Client PID: %d -> LIST\n", clientPID);
            list(clientResponseFIFO, dirname); 
        }

        else if (request.command == READFILE) {
            
            printf("Client PID: %d -> READFILE\n", clientPID);
            readFile(clientResponseFIFO, dirname, request);
        }

        
    }
    
    return 0;
}


char* get_file_names(const char* directory_path) {
    DIR *dir;
    struct dirent *entry;
    char *files;
    int size = 1024;  // Başlangıç boyutu

    // Bellek ayırma
    files = malloc(size * sizeof(char));
    if (files == NULL) {
        perror("malloc() error");
        return NULL;
    }
    files[0] = '\0';  // İlk karakteri null karakteri yap

    // Dizin açma
    dir = opendir(directory_path);
    if (dir == NULL) {
        perror("opendir() error");
        free(files);
        return NULL;
    }

    // Dizin içindeki dosyaları okuma
    while ((entry = readdir(dir)) != NULL) {
        if (strlen(files) + strlen(entry->d_name) + 2 > size) {
            size *= 2;  // İhtiyaç duyulan boyutu iki katına çıkar
            files = realloc(files, size);  // Belleği yeniden boyutlandır
            if (files == NULL) {
                perror("realloc() error");
                closedir(dir);
                return NULL;
            }
        }
        strcat(files, entry->d_name);  // Dosya adını stringe ekle
        strcat(files, "\n");           // Satır sonu ekle
    }

    // Dizini kapatma
    closedir(dir);
    return files;
}

void quit_child(int clientPID, int serverPID,int client_fd) {
    
    Request quit_request;
    quit_request.PID = getpid();
    quit_request.command = DISCONNECT;
    quit_request.requestType = CHILD_PARENT;
    
    char serverFIFO[256];
    sprintf(serverFIFO, "/tmp/main_fifo_%d", serverPID);
    int server_fd = open(serverFIFO, O_WRONLY);

    if (server_fd == -1) {
        perror("Failed to open server FIFO for writing");
        exit(1);
    }

    if (write(server_fd, &quit_request, sizeof(Request)) == -1) {
        perror("Failed to write to server FIFO");
        exit(1);
    }

    close(server_fd);

    ConnectionResponse response;
    response.clientPID = clientPID;
    response.status = 0;
    sprintf(response.message, "Success");
    response.serverPID = serverPID;

    int bytes_written = write(client_fd, &response, sizeof(ConnectionResponse));
    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }
}


void list(char *clientResponseFIFO, const char *dirname) {

    int client_response_fd = open(clientResponseFIFO, O_WRONLY);

    if (client_response_fd == -1) {
        perror("Failed to open client FIFO for writing");
        exit(1);
    }
    
    ConnectionResponse response;
    char *files = get_file_names(dirname);
    response.clientPID = getpid();
    response.status = 0;
    sprintf(response.message, "%s", files);
    response.serverPID = getpid();

    int bytes_written = write(client_response_fd, &response, sizeof(ConnectionResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }

    free(files);
    close(client_response_fd);
}


void readFile(char *clientResponseFIFO, const char *dirname, Request request) {
    
    printf("ReadFile\n");
    
    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dirname, request.parameter1);

    int line_number = atoi(request.parameter2);

    printf("File Path: %s\n", file_path);

    int file_fd = open(file_path, O_RDONLY);
    
    if (file_fd == -1) {
        perror("Failed to open file");
        exit(1);
    }

    
    printf("Line Number: %d\n", line_number);
    if(line_number == -1){

        int client_response_fd = open(clientResponseFIFO, O_WRONLY);

        if (client_response_fd == -1) {
            perror("Failed to open client FIFO for writing");
            exit(1);
        }
        
        FileResponse response;
        struct stat statbuf;
        int result;

        // Dosya bilgilerini al
        result = stat(file_path, &statbuf);

        if (result == -1) {
            perror("stat() error");
            exit(1);
        }

        response.totalChunks = statbuf.st_size / 65536;
        response.chunkNumber = 0;

        while (1) {
            int n = read(file_fd, response.data, sizeof(response.data));
            if (n == -1) {
                perror("Failed to read from file");
                exit(1);
            }
            if (n == 0) {
                break;
            }
            int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));
            if (bytes_written == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }
            response.chunkNumber++;
        }
        close(file_fd);
        close(client_response_fd);
    }

    else{

        int client_response_fd = open(clientResponseFIFO, O_WRONLY);

        if (client_response_fd == -1) {
            perror("Failed to open client FIFO for writing");
            exit(1);
        }

        int n;
        char buffer[65536]; // 64KB
        char line[65536]; // 64KB
        int lineIndex = 0;
        int flag = 0;
        int lineCount = 0;

        printf("There is a line number\n");


        while ((n = read(file_fd, buffer, sizeof(buffer) - 1)) > 0) {
            
            for (int i = 0; i < n; i++) {
                char c = buffer[i];
                if (c == '\n') {
                    
                    if (lineCount == line_number){
                        flag = 1;
                        break;
                    }

                    line[lineIndex] = '\0'; // Null-terminate the line

                    //printf("%s\n", line); // Print the line
                    lineIndex = 0;          // Reset the line index
                    lineCount++;
                    continue;
                }
                line[lineIndex++] = c; // Add the character to the line
            }

            if (flag == 1){
                break;
            }

        }


        if (n == -1) {
            perror("Failed to read from file");
            exit(1);
        }

        close(file_fd);

        FileResponse response;

        response.totalChunks = 1;
        response.chunkNumber = 0;
        sprintf(response.data, "%s", line);


        printf("Line: %s\n", response.data);
            
        int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

        printf("Bytes Written: %d\n", bytes_written);

        if (bytes_written == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }
        
        
       
    }

    



}

    