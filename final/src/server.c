#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/socket.h>
#include <complex.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define PORT 8080
#define OVEN_CAPACITY 6
#define COURIER_THRESHOLD 3
#define CANCEL_MSG "CANCEL"
#define LOG_SIZE 1024

#define ROWS 30
#define COLS 40

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

typedef struct {
    Order *order_queue;
    int order_count;
    pthread_mutex_t order_mutex;
} OrderQueue;

typedef struct {
    Order *cooked_orders;
    int cooked_count;
    pthread_mutex_t cooked_mutex;
    pthread_cond_t cooked_cond;
} CookedQueue;

volatile sig_atomic_t cancel_flag = 0;
pthread_mutex_t cancel_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cancel_cond = PTHREAD_COND_INITIALIZER;

char log_buffer[LOG_SIZE];


OrderQueue order_queue = {.order_mutex = PTHREAD_MUTEX_INITIALIZER};
CookedQueue cooked_queue = {.cooked_mutex = PTHREAD_MUTEX_INITIALIZER, .cooked_cond = PTHREAD_COND_INITIALIZER};

int orders_completed = 0;
int all_orders_handled = 0; // Flag to indicate all orders are handled
pthread_mutex_t orders_completed_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t oven_sem;
sem_t paddle_sem;
sem_t oven_entry_sem;  // New semaphore to limit oven entry to 2

int *chef_counts = NULL;
int *courier_counts = NULL;
double courier_speed; // m / min
int num_chefs;
int num_couriers;
int num_orders;

int new_socket = 0;

void* chef(void* arg);
void* courier(void* arg);
void* manager(void* arg);
void reset_data();
void print_order(Order order);
double hypotenuse(double a, double b);

void conjugateTranspose(complex double matrix[ROWS][COLS], complex double result[COLS][ROWS]);
void matrixProduct(complex double mat1[COLS][ROWS], complex double mat2[ROWS][COLS], complex double result[COLS][COLS]);
int gaussJordan(complex double matrix[COLS][COLS], complex double inverse[COLS][COLS]);
void pseudoInverse(complex double matrix[ROWS][COLS], complex double result[COLS][ROWS]);
double calculate_psuedo_inverse_time();
void loggin(char *message);

void handle_cancellation();

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
       
        exit(EXIT_SUCCESS);

    }
}

void send_message_to_client(const char *message);
double calculate_time(int start_x, int start_y, int dest_x, int dest_y) {

    //printf("Calculating time to deliver from (%d, %d) to (%d, %d)\n", start_x, start_y, dest_x, dest_y);
    double distance = hypotenuse(dest_x - start_x, dest_y - start_y) * 1000; // in meters
    //printf("Distance: %f\n", distance);
    return (distance / courier_speed) * 60;

}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <IP> <Number of chefs> <Number of couriers> <Courier speed>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);


    const char *ip = argv[1];
    num_chefs = atoi(argv[2]);
    num_couriers = atoi(argv[3]);
    courier_speed = atoi(argv[4]);

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    
    char buffer[BUFFER_SIZE] = {0};
    memset(buffer, 0, BUFFER_SIZE);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }


    printf("Server listening on %s:%d\n", ip, PORT);
    sprintf(log_buffer, "Server listening on %s:%d\n", ip, PORT);
    loggin(log_buffer);
    

        while (1) {
        
        int pid_client = 0;
        printf("Waiting for connection...\n");
        sprintf(log_buffer, "Waiting for connection...\n");
        loggin(log_buffer);


        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        while (read(new_socket, buffer, BUFFER_SIZE) > 0) {

            if (strncmp(buffer, CANCEL_MSG, strlen(CANCEL_MSG)) == 0) {
                handle_cancellation();
                break;
            }

         

            num_orders = atoi(buffer);
            printf("Number of orders: %d\n", num_orders);
            sprintf(log_buffer, "Number of orders: %d\n", num_orders);
            loggin(log_buffer);

            Order orders[num_orders];

            for (int i = 0; i < num_orders; i++) {
                memset(buffer, 0, BUFFER_SIZE);
                if (read(new_socket, buffer, BUFFER_SIZE) < 0) {
                    perror("read");
                    close(new_socket);
                    close(server_fd);
                    exit(EXIT_FAILURE);
                }

                if (strncmp(buffer, CANCEL_MSG, strlen(CANCEL_MSG)) == 0) {
                    handle_cancellation();
                    break;
                }

                sscanf(buffer, "%d %d %d %d %d %d %d %d", &orders[i].ip, &orders[i].port, &orders[i].pid, &orders[i].order_id, &orders[i].p, &orders[i].q, &orders[i].shop_x, &orders[i].shop_y);
                pid_client = orders[i].pid;
              
            }

            reset_data();

            pthread_t chefs[num_chefs];
            pthread_t couriers[num_couriers];
            pthread_t manager_thread;

            // Copy orders to the order queue
            for (int i = 0; i < num_orders; i++) {
                order_queue.order_queue[i] = orders[i];
            }

            // Create threads
            pthread_create(&manager_thread, NULL, manager, NULL);
            for (int i = 0; i < num_chefs; i++) {
                pthread_create(&chefs[i], NULL, chef, (void*)(long)i);
            }
            for (int i = 0; i < num_couriers; i++) {
                pthread_create(&couriers[i], NULL, courier, (void*)(long)i);
            }

            // Join threads
            for (int i = 0; i < num_chefs; i++) {
                pthread_join(chefs[i], NULL);
            }

            // Set flag to indicate all orders are handled
            pthread_mutex_lock(&orders_completed_mutex);
            all_orders_handled = 1;
            pthread_mutex_unlock(&orders_completed_mutex);

            // Signal manager to wake up
            pthread_cond_broadcast(&cooked_queue.cooked_cond);

            pthread_join(manager_thread, NULL);
            for (int i = 0; i < num_couriers; i++) {
                pthread_join(couriers[i], NULL);
            }

            // Find the most active chef and courier
            int max_chef_count = 0, max_chef_id = 0;
            for (int i = 0; i < num_chefs; i++) {
                if (chef_counts[i] > max_chef_count) {
                    max_chef_count = chef_counts[i];
                    max_chef_id = i;
                }
            }

            int max_courier_count = 0, max_courier_id = 0;
            for (int i = 0; i < num_couriers; i++) {
                if (courier_counts[i] > max_courier_count) {
                    max_courier_count = courier_counts[i];
                    max_courier_id = i;
                }
            }

            printf("Most active chef: Chef %d with %d orders\n", max_chef_id, max_chef_count);
            sprintf(log_buffer, "Most active chef: Chef %d with %d orders\n", max_chef_id, max_chef_count);
            loggin(log_buffer);

            printf("Most active courier: Courier %d with %d orders\n", max_courier_id, max_courier_count);
            sprintf(log_buffer, "Most active courier: Courier %d with %d orders\n", max_courier_id, max_courier_count);
            loggin(log_buffer);

            // Destroy semaphores
            sem_destroy(&oven_sem);
            sem_destroy(&paddle_sem);
            sem_destroy(&oven_entry_sem);

            // Send response back to client
  

            printf("done serving client @ %s Pid: %d\n", inet_ntoa(address.sin_addr), pid_client);
            sprintf(log_buffer, "done serving client @ %s Pid: %d\n", inet_ntoa(address.sin_addr), pid_client);
            loggin(log_buffer);

            close(new_socket);
        }
    }

    close(server_fd);
    return 0;
}

void* chef(void* arg) {
    int id = (int)(long)arg;

    while (1) {
        // Check for cancellation
        pthread_mutex_lock(&cancel_mutex);
        if (cancel_flag) {
            pthread_mutex_unlock(&cancel_mutex);
            break;
        }
        pthread_mutex_unlock(&cancel_mutex);

        Order order;

        // Get an order from the queue
        pthread_mutex_lock(&order_queue.order_mutex);
        if (order_queue.order_count > 0) {
            order = order_queue.order_queue[--order_queue.order_count];
            pthread_mutex_unlock(&order_queue.order_mutex);
        } else {
            pthread_mutex_unlock(&order_queue.order_mutex);
            break;
        }

        // Prepare the order (1 second)
        //printf("Chef %d is preparing order %d\n", id, order.order_id);
        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);
        sprintf(message, "Order %d prepared by chef %d", order.order_id, id);
        
        send_message_to_client(message);

        double time_to_prepare = calculate_psuedo_inverse_time();

        usleep(time_to_prepare ); 

        //printf("Time to prepare: %f\n", time_to_prepare);

        sem_wait(&oven_sem);

        // Wait for a paddle and oven space to put the order in the oven
        sem_wait(&paddle_sem);

        // Wait for an entry to the oven
        sem_wait(&oven_entry_sem);

        // Place the order in the oven
        //printf("Chef %d placed order %d in the oven\n", id, order.order_id);
        memset(message, 0, BUFFER_SIZE);
        message[BUFFER_SIZE];
        sprintf(message, "Order %d placed in the oven by chef %d", order.order_id, id);
        send_message_to_client(message);

        // Release the oven entry
        sem_post(&oven_entry_sem);

        // Release the paddle
        sem_post(&paddle_sem);

        usleep(time_to_prepare /2); 

        // Wait for a paddle to take the order out of the oven
        sem_wait(&paddle_sem);

        // Wait for an entry to the oven
        sem_wait(&oven_entry_sem);

        // Take the order out of the oven
        //printf("Chef %d took order %d out of the oven\n", id, order.order_id);

        // Release the oven entry
        sem_post(&oven_entry_sem);

        // Release the paddle
        sem_post(&paddle_sem);

        // Release the oven space
        sem_post(&oven_sem);

        // Put the cooked order in the cooked orders queue
        pthread_mutex_lock(&cooked_queue.cooked_mutex);
        cooked_queue.cooked_orders[cooked_queue.cooked_count++] = order;
        //printf("Chef %d placed order %d in the cooked orders queue\n", id, order.order_id);
        pthread_cond_signal(&cooked_queue.cooked_cond);
        pthread_mutex_unlock(&cooked_queue.cooked_mutex);

        // Update the completed orders count
        pthread_mutex_lock(&orders_completed_mutex);
        orders_completed++;
        pthread_mutex_unlock(&orders_completed_mutex);

        // Increment chef's order count
        chef_counts[id]++;
    }

    return NULL;
}


void* courier(void* arg) {
    int id = (int)(long)arg;
    Order deliveries[COURIER_THRESHOLD];

    while (1) {
        // Check for cancellation
        pthread_mutex_lock(&cancel_mutex);
        if (cancel_flag) {
            pthread_mutex_unlock(&cancel_mutex);
            break;
        }
        pthread_mutex_unlock(&cancel_mutex);

        pthread_mutex_lock(&cooked_queue.cooked_mutex);
        while (cooked_queue.cooked_count < COURIER_THRESHOLD) {
            pthread_mutex_lock(&orders_completed_mutex);
            if (all_orders_handled && cooked_queue.cooked_count > 0) {
                pthread_mutex_unlock(&orders_completed_mutex);
                break;
            }
            if (all_orders_handled && cooked_queue.cooked_count == 0) {
                pthread_mutex_unlock(&orders_completed_mutex);
                pthread_mutex_unlock(&cooked_queue.cooked_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&orders_completed_mutex);
            pthread_cond_wait(&cooked_queue.cooked_cond, &cooked_queue.cooked_mutex);

            // Check for cancellation
            pthread_mutex_lock(&cancel_mutex);
            if (cancel_flag) {
                pthread_mutex_unlock(&cancel_mutex);
                pthread_mutex_unlock(&cooked_queue.cooked_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&cancel_mutex);
        }

        int count = cooked_queue.cooked_count < COURIER_THRESHOLD ? cooked_queue.cooked_count : COURIER_THRESHOLD;
        for (int i = 0; i < count; i++) {
            deliveries[i] = cooked_queue.cooked_orders[--cooked_queue.cooked_count];
        }
        pthread_mutex_unlock(&cooked_queue.cooked_mutex);

        int start_x = deliveries[0].shop_x;
        int start_y = deliveries[0].shop_y;

        double total_time = 0;
        // Deliver the orders
        //printf("Courier %d is delivering orders:", id);
        for (int i = 0; i < count; i++) {
            double time_to_deliver = calculate_time(start_x, start_y, deliveries[i].p, deliveries[i].q);
            total_time += time_to_deliver;
            char message[BUFFER_SIZE];
            memset(message, 0, BUFFER_SIZE);
            sprintf(message, "Order %d has been given to courier %d. Delivery time: %f seconds", deliveries[i].order_id, id, total_time);
            send_message_to_client(message);

            // Sleep time_to_deliver seconds using usleep
            usleep(time_to_deliver * 1000000); // Convert seconds to microseconds
            memset(message, 0, BUFFER_SIZE);
            sprintf(message, "Order %d has been delivered by courier %d", deliveries[i].order_id, id);
            send_message_to_client(message);

            start_x = deliveries[i].p;
            start_y = deliveries[i].q;
        }

        double return_time = calculate_time(start_x, start_y, deliveries[0].shop_x, deliveries[0].shop_y);
        usleep(return_time * 1000000); // Convert seconds to microseconds

        // Increment courier's delivery count
        courier_counts[id] += count;
    }

    return NULL;
}

void* manager(void* arg) {
    while (1) {
        // Check for cancellation
        pthread_mutex_lock(&cancel_mutex);
        if (cancel_flag) {
            pthread_mutex_unlock(&cancel_mutex);
            break;
        }
        pthread_mutex_unlock(&cancel_mutex);

        pthread_mutex_lock(&cooked_queue.cooked_mutex);
        while (cooked_queue.cooked_count == 0) {
            pthread_mutex_lock(&orders_completed_mutex);
            if (all_orders_handled) {
                pthread_mutex_unlock(&orders_completed_mutex);
                pthread_mutex_unlock(&cooked_queue.cooked_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&orders_completed_mutex);
            pthread_cond_wait(&cooked_queue.cooked_cond, &cooked_queue.cooked_mutex);

            // Check for cancellation
            pthread_mutex_lock(&cancel_mutex);
            if (cancel_flag) {
                pthread_mutex_unlock(&cancel_mutex);
                pthread_mutex_unlock(&cooked_queue.cooked_mutex);
                return NULL;
            }
            pthread_mutex_unlock(&cancel_mutex);
        }
        // Signal couriers to pick up cooked orders
        pthread_cond_broadcast(&cooked_queue.cooked_cond);
        pthread_mutex_unlock(&cooked_queue.cooked_mutex);
    }
    return NULL;
}

void reset_data() {
    // Reset all data
    orders_completed = 0;
    all_orders_handled = 0;

    // Free previous dynamic arrays if they exist
    if (order_queue.order_queue) free(order_queue.order_queue);
    if (cooked_queue.cooked_orders) free(cooked_queue.cooked_orders);
    if (chef_counts) free(chef_counts);
    if (courier_counts) free(courier_counts);

    // Allocate memory for new dynamic arrays
    order_queue.order_queue = (Order*)malloc(num_orders * sizeof(Order));
    cooked_queue.cooked_orders = (Order*)malloc(num_orders * sizeof(Order));
    chef_counts = (int*)malloc(num_chefs * sizeof(int));
    courier_counts = (int*)malloc(num_couriers * sizeof(int));

    for (int i = 0; i < num_orders; i++) {
        order_queue.order_queue[i] = (Order){.ip = 127001, .port = 8080, .pid = i, .order_id = i + 1, .p = 0, .q = 0};
    }

    for (int i = 0; i < num_chefs; i++) {
        chef_counts[i] = 0;
    }

    for (int i = 0; i < num_couriers; i++) {
        courier_counts[i] = 0;
    }

    order_queue.order_count = num_orders;
    cooked_queue.cooked_count = 0;

    // Initialize semaphores
    sem_init(&oven_sem, 0, OVEN_CAPACITY);
    sem_init(&paddle_sem, 0, 3);
    sem_init(&oven_entry_sem, 0, 2);  // Only 2 entries to the oven
}

void print_order(Order order) {
    printf("Order:\n");
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

double hypotenuse(double a, double b) {
    return sqrt(a * a + b * b);
}
void send_message_to_client(const char *message) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "%s", message);
    send(new_socket, buffer, BUFFER_SIZE, 0);
    usleep(100000); // 0.1 ms

 

    usleep(100000); // 0.1 ms

    return;
}


void handle_cancellation() {
    pthread_mutex_lock(&cancel_mutex);
    cancel_flag = 1;
    pthread_cond_broadcast(&cancel_cond);
    pthread_mutex_unlock(&cancel_mutex);
    
    pthread_mutex_lock(&order_queue.order_mutex);
    order_queue.order_count = 0; // Clear the order queue
    pthread_mutex_unlock(&order_queue.order_mutex);

    pthread_mutex_lock(&cooked_queue.cooked_mutex);
    cooked_queue.cooked_count = 0; // Clear the cooked queue
    pthread_mutex_unlock(&cooked_queue.cooked_mutex);

    pthread_mutex_lock(&orders_completed_mutex);
    orders_completed = 0;
    all_orders_handled = 0; // Reset the flag
    pthread_mutex_unlock(&orders_completed_mutex);

    printf("Received cancellation. All orders have been cleared.\n");
}



#define ROWS 30
#define COLS 40

// Function to compute the conjugate transpose of a matrix
void conjugateTranspose(complex double matrix[ROWS][COLS], complex double result[COLS][ROWS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            result[j][i] = conj(matrix[i][j]);
        }
    }
}

// Function to compute the product of two matrices
void matrixProduct(complex double mat1[COLS][ROWS], complex double mat2[ROWS][COLS], complex double result[COLS][COLS]) {
    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < COLS; j++) {
            result[i][j] = 0.0 + 0.0 * I;
            for (int k = 0; k < ROWS; k++) {
                result[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

// Function to perform Gauss-Jordan elimination to invert a matrix
int gaussJordan(complex double matrix[COLS][COLS], complex double inverse[COLS][COLS]) {
    
    int i, j, k, maxRow;
    complex double ratio, temp;

    // Initialize the inverse matrix as the identity matrix
    for (i = 0; i < COLS; i++) {
        for (j = 0; j < COLS; j++) {
            if (i == j)
                inverse[i][j] = 1.0 + 0.0 * I;
            else
                inverse[i][j] = 0.0 + 0.0 * I;
        }
    }

    // Perform Gauss-Jordan elimination
    for (i = 0; i < COLS; i++) {
        // Find the maximum element in the current column for pivot
        maxRow = i;
        for (k = i + 1; k < COLS; k++) {
            if (cabs(matrix[k][i]) > cabs(matrix[maxRow][i])) {
                maxRow = k;
            }
        }

        // Swap the rows if necessary
        if (i != maxRow) {
            for (k = 0; k < COLS; k++) {
                temp = matrix[i][k];
                matrix[i][k] = matrix[maxRow][k];
                matrix[maxRow][k] = temp;

                temp = inverse[i][k];
                inverse[i][k] = inverse[maxRow][k];
                inverse[maxRow][k] = temp;
            }
        }

        // Check if the matrix is singular
        if (cabs(matrix[i][i]) == 0.0) {
            return -1;
        }

        // Normalize the pivot row
        for (k = 0; k < COLS; k++) {
            matrix[i][k] /= matrix[i][i];
            inverse[i][k] /= matrix[i][i];
        }

        // Eliminate the other rows
        for (j = 0; j < COLS; j++) {
            if (j != i) {
                ratio = matrix[j][i];
                for (k = 0; k < COLS; k++) {
                    matrix[j][k] -= ratio * matrix[i][k];
                    inverse[j][k] -= ratio * inverse[i][k];
                }
            }
        }
    }

    return 0;
}

// Function to compute the pseudo-inverse of a matrix
void pseudoInverse(complex double matrix[ROWS][COLS], complex double result[COLS][ROWS]) {
    complex double conjugateTrans[COLS][ROWS];
    complex double product[COLS][COLS];
    complex double inverse[COLS][COLS];

    // Compute the conjugate transpose
    conjugateTranspose(matrix, conjugateTrans);

    // Compute the product of the conjugate transpose and the original matrix
    matrixProduct(conjugateTrans, matrix, product);

    // Compute the inverse of the product using Gauss-Jordan elimination
    if (gaussJordan(product, inverse) == -1) {
        //printf("Matrix is singular and cannot be inverted.\n");
        return;
    }

    // Compute the pseudo-inverse
    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < ROWS; j++) {
            result[i][j] = 0.0 + 0.0 * I;
            for (int k = 0; k < COLS; k++) {
                result[i][j] += inverse[i][k] * conjugateTrans[k][j];
            }
        }
    }
}

double calculate_psuedo_inverse_time() { // microseconds
    complex double matrix[ROWS][COLS];
    complex double result[COLS][ROWS];

    // Initialize the matrix with some values (for example purposes, this could be any matrix)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = i + j * I; // Example initialization
        }
    }

    // Compute the pseudo-inverse

    double start_time = clock();

    pseudoInverse(matrix, result);

    double end_time = clock();

    return (end_time - start_time) / CLOCKS_PER_SEC * 1000000;
   
}


void loggin(char *message) {
    int log_fd = open("server.log", O_CREAT | O_WRONLY | O_APPEND, 0644);

    if (log_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    //write time to log file
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_buffer[32];
    memset(time_buffer, 0, 32);
    sprintf(time_buffer, "[%d-%d-%d %d:%d:%d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    int n = write(log_fd, time_buffer, strlen(time_buffer));

    if (n == -1) {
        perror("write");
        close(log_fd);
        exit(EXIT_FAILURE);
    }

    
    
    n = write(log_fd, message, strlen(message));

    if (n == -1) {
        perror("write");
        close(log_fd);
        exit(EXIT_FAILURE);
    }

    memset(log_buffer, 0, LOG_SIZE);

    close(log_fd);
}