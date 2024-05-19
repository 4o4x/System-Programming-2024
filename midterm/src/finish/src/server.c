#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>


sem_t *sem;


// Global variables for signal handling
int global_main_fifo_fd = -1;
int global_client_response_fd = -1;
int global_client_request_fd = -1;
int global_file_fd = -1;
char global_main_fifo[256];
char log_file_path[256];
int global_log_fd = -1;
int child_client_PID = -1;


/**
 * @brief Signal handler for child process
 * 
 * @param sig 
 */
void child_handler(int sig);

/**
 * @brief Signal handler for parent process
 * 
 * @param sig 
 */
void parent_handler(int sig);



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
    int size;
    char data[32768];
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
int freeQueue(Queue *queue);


Queue *clientQueue;

typedef struct {
    int clientPID;
    int childPID;
} ClientChild;

// Global client PIDs array
ClientChild *clientPIDs;
int MAX_CLIENTS;


/**
 * @brief Setup directory for the server
 * 
 * @param dirname  the directory path of the server
 */
void setup_directory(const char *dirname);



/**
 * @brief Child process function for each client
 * 
 * @param clientPID  the PID of the client
 * @param serverPID  the PID of the server
 * @param dirname  the directory path of the server
 * @return int 
 */
int client_child(int clientPID, int serverPID,const char *dirname);


/**
 * @brief Get file names in the directory
 * 
 * @param directory_path the directory path
 * @return char* 
*/
char* get_file_names(const char* directory_path);

/**
 * @brief List files in the directory
 * 
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param dirname the directory path of the server
 */
void list(int client_response_fd, const char *dirname);

/**
 * @brief Read file in the directory and send the content to the client using the client response FIFO
 * 
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param dirname the directory path of the server
 * @param request the request struct
 */
void readFile(int client_response_fd, const char *dirname, Request request);


/**
 * @brief Takes the file name and the line number as parameters
 * reads the string which is sent by the client and writes it to the file
 * 
 * @param client_request_fd the file descriptor of the client request FIFO
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param dirname the directory path of the server
 * @param request the request struct
 */
void write_file(int client_request_fd, int client_response_fd, const char *dirname, Request request);


/**
 * @brief Uploads the file to the server
 * 
 * @param client_request_fd the file descriptor of the client request FIFO
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param dirname the directory path of the server
 * @param request the request struct
 */
void upload(int client_request_fd, int client_response_fd, const char *dirname, Request request);

/**
 * @brief Downloads the file from the server
 * 
 * @param client_request_fd the file descriptor of the client request FIFO
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param dirname the directory path of the server
 * @param request the request struct
 */
void download(int client_request_fd, int client_response_fd, const char *dirname, Request request);

/**
 * @brief Quit the client process
 * 
 * @param client_request_fd the file descriptor of the client request FIFO
 * @param client_response_fd the file descriptor of the client response FIFO
 * @param request the request struct
 */
void quit(int client_request_fd, int client_response_fd, Request request);


/**
 * @brief Log the message to the log file
 * 
 * @param log_message the message to be logged
 * @return int 
 */
void logging(const char *log_message);

/**
    * @brief Split the filename and extension
    * 
    * @param path the path of the file
    * @param file_name the file name
    * @param extension the extension of the file
    * @return int 
*/
int splitFilename(const char *path, char *file_name, char *extension); 




int main(int argc, char *argv[]) {
    

    // Check the number of arguments
    if (argc != 3) {
        printf("Usage: %s <dirname> <max.#ofClients>\n", argv[0]);
        exit(1);
    }

    /////////////////////////////////////////////////
    // Signal handling for parent process
    struct sigaction sa_parent;
    sa_parent.sa_handler = parent_handler;
    sigemptyset(&sa_parent.sa_mask);
    sa_parent.sa_flags = 0;
    sigaction(SIGINT, &sa_parent, NULL);
    /////////////////////////////////////////////////

    /////////////////////////////////////////////////
    // Create a shared semaphore
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (sem == MAP_FAILED) {
        perror("mmap() error");
        exit(1);
    }

    if (sem_init(sem, 1, 1) == -1) {
        perror("sem_init() error");
        exit(1);
    }
    /////////////////////////////////////////////////

    /////////////////////////////////////////////////
    // Create a main FIFO
    char MAIN_FIFO[256];
    sprintf(MAIN_FIFO, "/tmp/main_fifo_%d", getpid());
    strcpy(global_main_fifo, MAIN_FIFO);
    mkfifo(MAIN_FIFO, 0666);
    /////////////////////////////////////////////////

    /////////////////////////////////////////////////
    // Setup directory
    const char *dirname = argv[1];
    setup_directory(dirname);
    /////////////////////////////////////////////////


   
    /////////////////////////////////////////////////
    // Create array for client PIDs and create a queue for clients in the queue
    MAX_CLIENTS = atoi(argv[2]);
    int clientCount = 0;
    int client_count_all = 0;

    
    clientPIDs = (ClientChild *)malloc(MAX_CLIENTS * sizeof(ClientChild));
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientPIDs[i].childPID = -1;
        clientPIDs[i].clientPID = -1;
    }

    clientQueue = createQueue(MAX_CLIENTS);
    /////////////////////////////////////////////////


    /////////////////////////////////////////////////
    // Log file
    sprintf(log_file_path, "%s/serverLog.log", dirname);
    global_log_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);

    if (global_log_fd == -1) {
        perror("Failed to open log file");
        exit(1);
    }

    close(global_log_fd);
    global_log_fd = -1;

    logging("Server is started");
    
    char log_message[256];
    sprintf(log_message, "Server PID: %d", getpid());
    logging(log_message);

    printf("Server is started\n");
    printf("Server PID: %d\n", getpid());

    /////////////////////////////////////////////////



    
    // Main server loop
    while (1) {

        int client_pid_to_connect = -1;
        

        /////////////////////////////////////////////////
        // Maib request FIFO
        Request request;
        
        int fd = open(MAIN_FIFO, O_RDONLY);
        if (fd == -1) {
            printf("Failed to open FIFO\n");
            exit(1);
        }

        global_main_fifo_fd = fd;

        int bytes_read = read(fd, &request, sizeof(Request));

        if (bytes_read == -1) {
            printf("Failed to read from FIFO\n");
            exit(1);
        }

        //printf("Main FIFO Read: %d %d %d %s %s %s\n", request.PID, request.command, request.requestType, request.parameter1, request.parameter2, request.parameter3);

        /////////////////////////////////////////////////

        if (request.requestType == CONNECTION_REQUEST){
            
            ConnectionResponse response;

            memset(&response, 0, sizeof(ConnectionResponse));
            char clientFIFO[256];
            sprintf(clientFIFO, "/tmp/client_response_fifo_%d", request.PID);

            int client_fd = open(clientFIFO, O_WRONLY);
            if (client_fd == -1) {
                printf("Failed to open client FIFO\n");
                exit(1);
            }

            global_client_response_fd = client_fd;
            
            // Server is full and client wants to connect with tryConnect
            if (MAX_CLIENTS == clientCount && request.command == TRY_CONNECT) {
                
                
                char message[256];
                sprintf(message, "Client PID: %d Server is full. You are rejected", request.PID);
                logging(message);
                printf(">>%s\n", message);


                response.clientPID = request.PID;
                response.status = 2;
                sprintf(response.message, "Server is full");
                response.serverPID = getpid();

                int byte = write(client_fd, &response, sizeof(ConnectionResponse));

                if (byte == -1) {
                    perror("Failed to write to client FIFO");
                    exit(1);
                }

                close(client_fd);
                global_client_response_fd = -1;
                close(fd);
                global_main_fifo_fd = -1;
                continue;
            }
            
            else if (MAX_CLIENTS == clientCount && request.command == CONNECT) {
                
                char message[256];
                sprintf(message, "Client PID: %d Server is full. Connection request is added to the queue", request.PID);
                logging(message);
                printf(">>%s\n", message);

                response.clientPID = request.PID;
                response.status = 1;
                sprintf(response.message, "You are in the queue");
                response.serverPID = getpid();
                
                int byte = write(client_fd, &response, sizeof(ConnectionResponse));

                if (byte == -1) {
                    perror("Failed to write to client FIFO");
                    exit(1);
                }

                close(client_fd);
                global_client_response_fd = -1;
                close(fd);
                global_main_fifo_fd = -1;
                enqueue(clientQueue, request.PID);

                continue;
            }

            char message[256];
            sprintf(message, "Client PID: %d connected as \"client_%d\"", request.PID, client_count_all);
            logging(message);
            printf(">>%s\n", message);

            response.clientPID = request.PID;
            response.status = 0;
            sprintf(response.message, "Success");
            response.serverPID = getpid();

            int byte = write(client_fd, &response, sizeof(ConnectionResponse));

            if (byte == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }

            clientCount++;
            client_count_all++;

            client_pid_to_connect = request.PID;

            close(client_fd);
            global_client_response_fd = -1;

        }

        else if (request.requestType == CHILD_PARENT) {
            
            if (request.command == DISCONNECT) {

                char message[256];
            

                clientCount--;
                
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    
                    if (clientPIDs[i].childPID == request.PID) {
                        clientPIDs[i].childPID = -1;

                        sprintf(message, "Client PID: %d is disconnected", clientPIDs[i].clientPID);
                        logging(message);
                        printf(">>%s\n", message);
                        clientPIDs[i].clientPID = -1;
                        break;
                    }
                }

                if (!isEmpty(clientQueue)) {

                    int nextClientPID = dequeue(clientQueue);

                    char message[256];
                    sprintf(message, "Client PID: %d connected as \"client_%d\"", nextClientPID, client_count_all);
                    logging(message);
                    printf(">>%s\n", message);

                    clientCount++;
                    client_count_all++;

                    char clientFIFO[256];
                    sprintf(clientFIFO, "/tmp/client_response_fifo_%d", nextClientPID);

                    int client_fd = open(clientFIFO, O_WRONLY);

                    if (client_fd == -1) {
                        printf("Failed to open client FIFO\n");
                        exit(1);
                    }

                    global_client_response_fd = client_fd;

                    ConnectionResponse response;
                    response.clientPID = nextClientPID;
                    response.status = 0;
                    sprintf(response.message, "Success");
                    response.serverPID = getpid();
                    
                    int byte = write(client_fd, &response, sizeof(ConnectionResponse));

                    if (byte == -1) {
                        perror("Failed to write to client FIFO");
                        exit(1);
                    }


                    close(client_fd);
                    global_client_response_fd = -1;
                    client_pid_to_connect = nextClientPID;
                }

                else {
                    close(fd);
                    global_main_fifo_fd = -1;
                    continue;
                }
            }
        }

        else if (request.requestType == REQUEST && request.command == KILL) {

            char message[256];
            sprintf(message, "Server is terminated");
            logging(message);
            printf(">>%s\n", message);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientPIDs[i].childPID != -1) {
                    kill(clientPIDs[i].childPID, SIGINT);
                }
            }

            // Wait for all child processes to terminate
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientPIDs[i].childPID != -1) {
                    waitpid(clientPIDs[i].childPID, NULL, 0);
                }
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientPIDs[i].childPID != -1) {
                    kill(clientPIDs[i].childPID, SIGKILL);
                }
            }

            
            freeQueue(clientQueue);
            free(clientPIDs);

            close(fd);
            global_main_fifo_fd = -1;

            unlink(MAIN_FIFO);

            sem_destroy(sem);
            munmap(sem, sizeof(sem_t));

            exit(0);

        }

        else {
            printf("Invalid request type\n");
            close(fd);
            global_main_fifo_fd = -1;
            continue;
        }
        
        close(fd);
        global_main_fifo_fd = -1;

       

        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork");
            exit(1);
        }

        if (pid == 0) {
            
            // Signal handling for child process
            struct sigaction sa_child;
            sa_child.sa_handler = child_handler;
            sigemptyset(&sa_child.sa_mask);
            sa_child.sa_flags = 0;
            sigaction(SIGINT, &sa_child, NULL);

            client_child(client_pid_to_connect, getpid(), dirname);
            exit(0);
        }

        // Client PID is added to the clientPIDs array
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientPIDs[i].childPID == -1 && clientPIDs[i].clientPID == -1) {
                clientPIDs[i].childPID = pid;
                clientPIDs[i].clientPID = client_pid_to_connect;
                break;
            }
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

int freeQueue(Queue *queue) {
    free(queue->array);
    free(queue);
    return 0;
}

int client_child(int clientPID, int serverPID,const char *dirname) {
    
    //printf("Client_child function is called childPID: %d\n", getpid());
    freeQueue(clientQueue);
    
    char clientRequestFIFO[256];
    sprintf(clientRequestFIFO, "/tmp/client_request_fifo_%d", clientPID);
    char clientResponseFIFO[256];
    sprintf(clientResponseFIFO, "/tmp/client_response_fifo_%d", clientPID);

    int client_request_fd = open(clientRequestFIFO, O_RDONLY);

    if (client_request_fd == -1) {
            perror("Failed to open client FIFO for reading");
            exit(1);
    }

    global_client_request_fd = client_request_fd;

    int client_response_fd = open(clientResponseFIFO, O_WRONLY);

    if (client_response_fd == -1) {
            perror("Failed to open client FIFO for writing");
            exit(1);
    }

    global_client_response_fd = client_response_fd;


    ConnectionResponse response;
    memset(&response, 0, sizeof(ConnectionResponse));
    response.clientPID = clientPID;
    response.status = 0;
    sprintf(response.message, "Success");
    response.serverPID = getpid();

    int bytes_written = write(client_response_fd, &response, sizeof(ConnectionResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }

    while (1)
    {
        Request request;
    
        int bytes_read = read(client_request_fd, &request, sizeof(Request));
        

        if (bytes_read == -1) {
            perror("Failed to read from client FIFO");
            exit(1);
        }
        
        if (bytes_read == 0) {
            sleep(20);
            exit(0);
        }

        if (request.command == QUIT) {
            
            printf("Client PID: %d -> QUIT\n", clientPID);
            quit(client_request_fd, client_response_fd, request);
            break;
        }

        else if (request.command == LIST) {
            printf("Client PID: %d -> LIST\n", clientPID);
            list(client_response_fd, dirname);
        }

        else if (request.command == READFILE) {
            sem_wait(sem);
            printf("Client PID: %d -> READFILE\n", clientPID);
            readFile(client_response_fd, dirname, request);
            sem_post(sem);
        }

        else if (request.command == WRITEFILE) {
            sem_wait(sem);
            printf("Client PID: %d -> WRITEFILE\n", clientPID);
            write_file(client_request_fd, client_response_fd, dirname, request);    
            sem_post(sem);
        }

        else if (request.command == UPLOAD) {
            sem_wait(sem);
            printf("Client PID: %d -> UPLOAD\n", clientPID);
            upload(client_request_fd, client_response_fd, dirname, request);
            sem_post(sem);
        }

        else if (request.command == DOWNLOAD) {
            sem_wait(sem);
            printf("Client PID: %d -> DOWNLOAD\n", clientPID);
            download(client_request_fd, client_response_fd, dirname, request);
            sem_post(sem);
        }

        else if (request.command == ARCH_SERVER) {
            sem_wait(sem);
            printf("Client PID: %d -> ARCH_SERVER\n", clientPID);
            sem_post(sem);
        }

        else {
            printf("Invalid command\n");
        }

        
        
    }

    close(client_request_fd);
    close(client_response_fd);

    global_client_request_fd = -1;
    global_client_response_fd = -1;
    
    exit(0);
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




void list(int client_response_fd, const char *dirname) {

    // Have done: memset,

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));
    char *files = get_file_names(dirname);
    response.totalChunks = 0;
    response.chunkNumber = 0;
    sprintf(response.data, "%s", files);


    int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }

    free(files);
}


void readFile(int client_response_fd, const char *dirname, Request request) {

    //printf("ReadFile\n");
    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dirname, request.parameter1);

    int line_number = atoi(request.parameter2);

    //printf("File Path: %s\n", file_path);

    int file_fd = open(file_path, O_RDONLY);
    
    if (file_fd == -1) {
        perror("Failed to open file");

        FileResponse response;
        memset(&response, 0, sizeof(FileResponse));
        response.totalChunks = -1;
        response.chunkNumber = -1;

        //strerror(errno) -> error message
        sprintf(response.data, "%s", strerror(errno));

        int by = write(client_response_fd, &response, sizeof(FileResponse));

        if (by == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }

        return;
        
    }
    global_file_fd = file_fd;

    
    //printf("Line Number: %d\n", line_number);
    if(line_number == -1){

        //printf("There is no line number\n");
        FileResponse response;
        memset(&response, 0, sizeof(FileResponse));
        
        struct stat statbuf;
        int result;

        // Dosya bilgilerini al
        result = stat(file_path, &statbuf);

        if (result == -1) {
            perror("stat() error");
            exit(1);
        }

        int totalChunks = statbuf.st_size / 32768 + 1;

        if (statbuf.st_size % 32768 == 0) {
            totalChunks--;
        }

        response.totalChunks = totalChunks;
        response.chunkNumber = 0;

        //printf("Total cunks: %d\n", totalChunks);

        int lastChunkSize = statbuf.st_size % 32768;
        
        int by = write(client_response_fd, &response, sizeof(FileResponse));

        if (by == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }



        for (int i = 0; i < totalChunks; i++) {

            if(i == totalChunks - 1){
                response.size = lastChunkSize;
            }
            else{
                response.size = 32768;
            }

            
            int n = read(file_fd, response.data, sizeof(response.data));
            if (n == -1) {
                perror("Failed to read from file");
                exit(1);
            }
            if (n == 0) {
                break;
            }

            while (n < response.size) {
                int remaining = response.size - n;
                //printf("Remaining: %d\n", remaining);
                
                char buffer[remaining];
                int bytes = read(file_fd, buffer, remaining);
                //printf("Read Bytes: %d\n", bytes);

                if (bytes == -1) {
                    perror("Failed to read from FIFO");
                    exit(1);
                }

                //overwrite the buffer
                
                for (int i = 0; i < bytes; i++) {
                    response.data[n] = buffer[i];
                    n++;
                }

            }

            //printf("Response: %s\n", response.data);
            int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));
            if (bytes_written == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }
            response.chunkNumber++;
        }

        close(file_fd);
        global_file_fd = -1;
        
    }

    else{

        int n;
        char buffer[32768]; // 32KB
        char line[32768];    // 32KB
        int lineIndex = 0;
        int flag = 0;
        int lineCount = 0;

        //printf("There is a line number\n");


        while ((n = read(file_fd, buffer, sizeof(buffer) - 1)) > 0) {
            
            for (int i = 0; i < n; i++) {
                char c = buffer[i];
                if (c == '\n') {
                    lineCount++;
        
                    line[lineIndex] = '\0'; // Null-terminate the line

                    //printf("%s\n", line); // Print the line
                    lineIndex = 0;       // Reset the line index

                    if (lineCount == line_number){
                        flag = 1;
                        break;
                    }
                    
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
        global_file_fd = -1;


        if(flag == 0){
            //printf("Line not found\n");
            FileResponse response;
            memset(&response, 0, sizeof(FileResponse));
            response.totalChunks = -1;
            response.chunkNumber = -1;
            sprintf(response.data, "Line not found");

            int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

            if (bytes_written == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }

            return;
        }

        


        FileResponse response;
        memset(&response, 0, sizeof(FileResponse));

        response.totalChunks = 1;
        response.chunkNumber = 0;
        sprintf(response.data, "%s", line);


        //printf("Line: %s\n", response.data);
    
        int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

        //printf("Bytes Written: %d\n", bytes_written);

        if (bytes_written == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }
        
    }


}


void child_handler(int sig) {
    
    printf("Child process PID %d is terminated\n", getpid());

    if (global_file_fd != -1) {
        close(global_file_fd);
    }

    if (global_client_request_fd != -1) {
        close(global_client_request_fd);
    }

    if (global_client_response_fd != -1) {
        close(global_client_response_fd);
    }

    //printf("Global main fifo: %s\n", global_main_fifo);

    int server_fd = open(global_main_fifo, O_WRONLY);

    if (server_fd == -1) {
        perror("Failed to open server FIFO for writing");
        exit(1);
    }

    Request request;
    memset(&request, 0, sizeof(Request));
    request.PID = getpid();
    request.command = DISCONNECT;
    request.requestType = CHILD_PARENT; 

    if (write(server_fd, &request, sizeof(Request)) == -1) {
        perror("Failed to write to server FIFO");
        exit(1);
    }

    close(server_fd);

    printf("Child process is terminated\n");

    exit(0);
}

void write_file(int client_request_fd, int client_response_fd, const char *dirname, Request request) {
    
    //printf("WriteFile\n");

    //printf("Parameter1: %s\n", request.parameter1);
    //printf("Parameter2: %s\n", request.parameter2);


    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dirname, request.parameter1);

    int line = atoi(request.parameter2);
    int fd;

    if (line == -1) {
        fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        
    } else {
        fd = open(file_path, O_RDWR | O_CREAT , 0666);
    }

    if (fd == -1) {
        perror("Failed to open file");

        FileResponse response;
        response.totalChunks = -1;
        response.chunkNumber = -1;

        //strerror(errno) -> error message
        sprintf(response.data, "%s", strerror(errno));

        int by = write(client_response_fd, &response, sizeof(FileResponse));

        if (by == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }

        return;
    }


    global_file_fd = fd;

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));
    response.totalChunks = 0;
    response.chunkNumber = 0;
    sprintf(response.data, "Success");
    //printf("Success\n");

    int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }



    if (line == -1) {
        
        //printf("There is no line number\n");

        FileResponse response;

        int read_bytes = read(client_request_fd, &response, sizeof(FileResponse));

        if (read_bytes == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        while (read_bytes < sizeof(FileResponse)) {
            int remaining = sizeof(FileResponse) - read_bytes;
            //printf("Remaining: %d\n", remaining);
            
            char buffer[remaining];
            int n = read(client_request_fd, buffer, remaining);
            //printf("Read Bytes: %d\n", n);

            if (n == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            //overwrite the buffer
            
            for (int i = 0; i < n; i++) {
                response.data[read_bytes] = buffer[i];
                read_bytes++;
            }

        }

        //printf("Response: %s\n", response.data);

        int write_new_line = write(fd, "\n", 1);

        if (write_new_line == -1) {
            perror("Failed to write to file");
            exit(1);
        }

        int write_bytes = write(fd, response.data, response.size);

        if (write_bytes == -1) {
            perror("Failed to write to file");
            exit(1);
        }

    }

    else{

        FileResponse response;
        memset(&response, 0, sizeof(FileResponse));

        int read_bytes = read(client_request_fd, &response, sizeof(FileResponse));

        if (read_bytes == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }

        while (read_bytes < sizeof(FileResponse)) {
            int remaining = sizeof(FileResponse) - read_bytes;
            //printf("Remaining: %d\n", remaining);
            
            char buffer[remaining];
            int n = read(client_request_fd, buffer, remaining);
            //printf("Read Bytes: %d\n", n);

            if (n == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            //overwrite the buffer
            
            for (int i = 0; i < n; i++) {
                response.data[read_bytes] = buffer[i];
                read_bytes++;
            }

        }

        int n;
        char buffer[32768]; // 32KB
        int flag = 0;
        int lineCount = 0;
        int readed_bytes = 0;
        char buffer_line[32768];

 
        while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
            
            for (int i = 0; i < n; i++) {

                if (lineCount+1 == line && flag == 0){
                        
                        flag = 1;
                        
                        lseek(fd, readed_bytes, SEEK_SET);

                        

                        int write_bytes = write(fd, response.data, response.size);

                        if (write_bytes == -1) {
                            perror("Failed to write to file");
                            exit(1);
                        }

                        int write_new_line = write(fd, "\n", 1);

                        if (write_new_line == -1) {
                            perror("Failed to write to file");
                            exit(1);
                        }

                        readed_bytes += response.size + 1;
                    }
                
                
                char c = buffer[i];
                if (c == '\n') {


                    
                    lineCount++;
                }
                
                lseek(fd, readed_bytes, SEEK_SET);
                int write_bytes = write(fd, &c, 1);

                if (write_bytes == -1) {
                    perror("Failed to write to file");
                    exit(1);
                }

                readed_bytes++;
                
            }

            

        }


        if (n == -1) {
            perror("Failed to read from file");
            exit(1);
        }

        if(flag == 0){
            
            close(fd);
            global_file_fd = -1;

            FileResponse response;
            memset(&response, 0, sizeof(FileResponse));
            response.totalChunks = -1;
            response.chunkNumber = -1;
            sprintf(response.data, "Line not found");

            int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

            if (bytes_written == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }

            return;
        }

        
        close(fd);
        global_file_fd = -1;
        
    }

    FileResponse response2;
    memset(&response2, 0, sizeof(FileResponse));
    
    response2.totalChunks = 1;
    response2.chunkNumber = 0;

    sprintf(response2.data, "Success");

    int bytes_written2 = write(client_response_fd, &response2, sizeof(FileResponse));

    if (bytes_written2 == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }
    

}

void upload(int client_request_fd, int client_response_fd, const char *dirname, Request request) {


    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dirname, request.parameter1);

    int file_fd;
    int count = 0;

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));

    char *temp = strdup(file_path);

    while (file_fd = open(temp, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666), file_fd == -1) {
        
        if(errno == EEXIST){
            count++;
            char temp_file_name[256];
            char temp_file_extension[64];
            int return_value = splitFilename(file_path, temp_file_name, temp_file_extension);

            if (return_value == -1) {
                sprintf(temp, "%s_%d", temp_file_name, count);
            }

            else{
                sprintf(temp, "%s_%d.%s", temp_file_name, count, temp_file_extension);
            }
        }
        else{
            perror("Failed to open file");
            
            response.totalChunks = -1;
            response.chunkNumber = -1;

            //strerror(errno) -> error message
            sprintf(response.data, "%s", strerror(errno));

            int by = write(client_response_fd, &response, sizeof(FileResponse));

            if (by == -1) {
                perror("Failed to write to client FIFO");
                exit(1);
            }

            return;

        }
        
    }

    global_file_fd = file_fd;

    //printf("File Path: %s\n", file_path);
    /////////////////////////////////////////////////////////////////////////////
    
    int read_bytes = read(client_request_fd, &response, sizeof(FileResponse));

    //printf("Total Chunks: %d\n", response.totalChunks);

    if (read_bytes == -1) {
        perror("Failed to read from FIFO");
        exit(1);
    }
    /////////////////////////////////////////////////////////////////////////////

    FileResponse response_data;
    memset(&response_data, 0, sizeof(FileResponse));
    
    for (int i = 0; i < response.totalChunks; i++) {
        
        int x = read(client_request_fd, &response_data, sizeof(FileResponse));
        if (x == -1) {
            perror("Failed to read from FIFO");
            exit(1);
        }
        //printf("Read Bytes: %d\n", x);

        // if all data is not read from the FIFO read the remaining data and add to the response_data.data
        while (x < sizeof(FileResponse)) {
            int remaining = sizeof(FileResponse) - x;
            //printf("Remaining: %d\n", remaining);
            
            char buffer[remaining];
            int n = read(client_request_fd, buffer, remaining);
            //printf("Read Bytes: %d\n", n);

            if (n == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            //overwrite the buffer
            
            for (int i = 0; i < n; i++) {
                response_data.data[x] = buffer[i];
                x++;
            }

        }

        //printf("Response: %s\n", response.data);

        //printf("Chunk Number: %d\n", response_data.chunkNumber);
        //printf("Data size: %d\n", response_data.size);
        int n = write(file_fd, response_data.data, response_data.size);

        if (n == -1) {
            perror("Failed to write to file");
            exit(1);
        }

    }

    close(file_fd);
    global_file_fd = -1;
    /////////////////////////////////////////////////////////////////////////////

    response.totalChunks = 0;
    response.chunkNumber = 0;
    sprintf(response.data, "Success");

    int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }

    //printf("Upload is completed\n");

    return;

}

void download(int client_request_fd, int client_response_fd, const char *dirname, Request request) {
    
    printf("Download\n");

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dirname, request.parameter1);

    int file_fd = open(file_path, O_RDONLY);

    if (file_fd == -1) {
        perror("Failed to open file");

        FileResponse response;
        response.totalChunks = -1;
        response.chunkNumber = -1;

        //strerror(errno) -> error message
        sprintf(response.data, "%s", strerror(errno));

        int by = write(client_response_fd, &response, sizeof(FileResponse));

        if (by == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }

        return;
    }

    global_file_fd = file_fd;

    FileResponse response;
    memset(&response, 0, sizeof(FileResponse));
    struct stat statbuf;
    int result;

    // Dosya bilgilerini al
    result = stat(file_path, &statbuf);

    if (result == -1) {
        perror("stat() error");
        exit(1);
    }

    int totalChunks = statbuf.st_size / 32768+1;

    if (statbuf.st_size % 32768 == 0) {
        totalChunks--;
    }

    int lastChunkSize = statbuf.st_size % 32768;

    printf("Last Chunk Size: %d\n", lastChunkSize);

    response.totalChunks = totalChunks;
    response.chunkNumber = 0;
    response.size = statbuf.st_size;

    int by = write(client_response_fd, &response, sizeof(FileResponse));

    if (by == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }

    for (int i = 0; i < totalChunks; i++) {

        if (i == totalChunks - 1) {
            response.size = lastChunkSize;
        } else {
            response.size = 32768;
        }

        response.totalChunks = totalChunks;
        response.chunkNumber = i;

        int bytes_read = read(file_fd, response.data, response.size);
        
        if (bytes_read == -1) {
            perror("Failed to read from file");
            exit(1);
        }


        while (bytes_read < response.size) {
            int remaining = response.size - bytes_read;
            printf("Remaining: %d\n", remaining);
            
            char buffer[remaining];
            int bytes = read(file_fd, buffer, remaining);
            printf("Read Bytes: %d\n", bytes);

            if (bytes == -1) {
                perror("Failed to read from FIFO");
                exit(1);
            }

            //overwrite the buffer
            
            for (int i = 0; i < bytes; i++) {
                response.data[bytes_read] = buffer[i];
                bytes_read++;
            }

        }

        int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));
        if (bytes_written == -1) {
            perror("Failed to write to client FIFO");
            exit(1);
        }


    }

    close(file_fd);

    global_file_fd = -1;

    printf("Download is completed\n");

    /*
    FileResponse response_of_write;

    response_of_write.totalChunks = 0;
    response_of_write.chunkNumber = 0;
    sprintf(response_of_write.data, "Success");

    int bytes_written = write(client_response_fd, &response, sizeof(FileResponse));

    if (bytes_written == -1) {
        perror("Failed to write to client FIFO");
        exit(1);
    }
    */

    return;
}


void quit(int client_request_fd, int client_response_fd, Request request) {
    
    printf("Quit\n");

    Request quit_request;
    memset(&quit_request, 0, sizeof(Request));
    quit_request.PID = getpid();
    quit_request.command = DISCONNECT;
    quit_request.requestType = CHILD_PARENT;
    quit_request.parameter1[0] = '\0';
    quit_request.parameter2[0] = '\0';
    quit_request.parameter3[0] = '\0';

    int server_fd = open(global_main_fifo, O_WRONLY);
    global_file_fd = server_fd;

    if (server_fd == -1) {
        perror("Failed to open server FIFO for writing");
        exit(1);
    }

    if (write(server_fd, &quit_request, sizeof(Request)) == -1) {
        perror("Failed to write to server FIFO");
        exit(1);
    }

    close(server_fd);
    global_file_fd = -1;

    return;
   
}


void logging(const char *message) {

    // get the current time and timestamp
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char timestamp[256];
    strftime(timestamp, 256, "%Y-%m-%d %H:%M:%S", timeinfo);

    char log_message[512];

    sprintf(log_message, "[%s] %s\n", timestamp, message);


    int fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);

    if (fd == -1) {
        perror("Failed to open log file");
        exit(1);
    }

    
    global_log_fd = fd;
    int n = write(fd, log_message, strlen(log_message));

    if (n == -1) {
        perror("Failed to write to log file");
        exit(1);
    }

    close(fd);
    global_log_fd = -1;    
}

void parent_handler(int sig) {

    if (global_log_fd != -1) {
        close(global_log_fd);
    }

    if (global_main_fifo_fd != -1) {
        close(global_main_fifo_fd);
    }

    if (global_client_request_fd != -1) {
        close(global_client_request_fd);
    }

    if (global_client_response_fd != -1) {
        close(global_client_response_fd);
    }

    if (global_file_fd != -1) {
        close(global_file_fd);
    }

    
    
    freeQueue(clientQueue);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientPIDs[i].childPID != -1) {
            kill(clientPIDs[i].childPID, SIGINT);
        }
    }

    // Wait for all child processes to terminate

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientPIDs[i].childPID != -1) {
            waitpid(clientPIDs[i].childPID, NULL, 0);
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientPIDs[i].childPID != -1) {
            kill(clientPIDs[i].childPID, SIGKILL);
        }
    }

    free(clientPIDs);

    if (global_log_fd != -1) {
        close(global_log_fd);
    }

    unlink(global_main_fifo);

    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));

    exit(0);
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