#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define CANCEL_MSG "CANCEL"

typedef struct {
    int ip;
    int port;
    int pid;
    int order_id;
    int p;
    int q;
    int shop_x;
    int shop_y;
} Order;

int sock = 0;
int cancel = 0;

int get_random_number(const int x);
void print_order(Order order);
void handle_signal(int signal);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <IP> <Number_of_clients> <p> <q>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    
    // Open log file
    int log_fd = open("client.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    const int number_of_clients = atoi(argv[2]);
    const int p = atoi(argv[3]);
    const int q = atoi(argv[4]);

    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    Order orders[number_of_clients];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    // Set up signal handling using sigaction
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);  // Ctrl + C
    sigaction(SIGTERM, &sa, NULL); // Termination signal

    for (int i = 0; i < number_of_clients; i++) {
        orders[i].ip = inet_addr(ip);
        orders[i].port = PORT;
        orders[i].pid = getpid();
        orders[i].order_id = i;

        orders[i].p = get_random_number(p);
        orders[i].q = get_random_number(q);
        orders[i].shop_x = p / 2;
        orders[i].shop_y = q / 2;
    }

    // Initialize the buffer to avoid uninitialized byte errors
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "%d", number_of_clients);
    send(sock, buffer, BUFFER_SIZE, 0);
    printf("Number of clients sent to server\n");
    usleep(100000); // 100 ms

    for (int i = 0; i < number_of_clients; i++) {
        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE); // Ensure message buffer is zeroed out
        snprintf(message, BUFFER_SIZE, "%d %d %d %d %d %d %d %d ", orders[i].ip, orders[i].port, orders[i].pid, orders[i].order_id, orders[i].p, orders[i].q, orders[i].shop_x, orders[i].shop_y);

        send(sock, message, BUFFER_SIZE, 0);

        // Write to log file: time, order_id, p, q
        char log_message[BUFFER_SIZE * 2];
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        snprintf(log_message, sizeof(log_message), "[%d-%02d-%02d %02d:%02d:%02d] Order ID: %d, p: %d, q: %d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, orders[i].order_id, orders[i].p, orders[i].q);
        int n = write(log_fd, log_message, strlen(log_message));
        
        if (n == -1) {
            perror("write");
            close(sock);
            exit(EXIT_FAILURE);
        }

        usleep(100000); // 100 ms
    }

    printf("Orders sent to server\n");

    // Read from server
    int n = number_of_clients * 4;

    for (int i = 0; i < n; i++) {
        memset(buffer, 0, BUFFER_SIZE); // Ensure buffer is zeroed out before reading
        read(sock, buffer, BUFFER_SIZE);

        // Write server response to log file
        char log_message[BUFFER_SIZE * 2];
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        snprintf(log_message, sizeof(log_message), "[%d-%02d-%02d %02d:%02d:%02d] %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, buffer);
        n = write(log_fd, log_message, strlen(log_message));
        
        if (n == -1) {
            perror("write");
            close(sock);
            exit(EXIT_FAILURE);
        }

        usleep(100000); // 100 ms
    }
    
    printf("Every client received the order\n");
    close(sock);
    close(log_fd);
    printf("PID: %d\n", getpid());

    return 0;
}

int get_random_number(const int x) {
    return rand() % x;
}

void print_order(Order order) {
    printf("IP: %d\n", order.ip);
    printf("Port: %d\n", order.port);
    printf("PID: %d\n", order.pid);
    printf("Order ID: %d\n", order.order_id);
    printf("P: %d\n", order.p);
    printf("Q: %d\n", order.q);
    printf("Shop X: %d\n", order.shop_x);
    printf("Shop Y: %d\n", order.shop_y);
    printf("\n");
}

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cancel = 1;
        send(sock, CANCEL_MSG, strlen(CANCEL_MSG), 0); // Send cancellation message to the server
        close(sock);
        exit(EXIT_SUCCESS);
    }
}
