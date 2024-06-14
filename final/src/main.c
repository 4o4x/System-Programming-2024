#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define OVEN_CAPACITY 6
#define COURIER_THRESHOLD 3

typedef struct {
    int ip;
    int port;
    int pid;
    int order_id;
    int is_served;
    int p;
    int q;
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

OrderQueue order_queue = {.order_mutex = PTHREAD_MUTEX_INITIALIZER};
CookedQueue cooked_queue = {.cooked_mutex = PTHREAD_MUTEX_INITIALIZER, .cooked_cond = PTHREAD_COND_INITIALIZER};

int orders_completed = 0;
int all_orders_handled = 0; // Flag to indicate all orders are handled
pthread_mutex_t orders_completed_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t oven_sem;
sem_t paddle_sem;
sem_t oven_entry_sem;  // New semaphore to limit oven entry to 2

int *chef_counts;
int *courier_counts;
int courier_speed;
int num_chefs;
int num_couriers;
int num_orders;

void* chef(void* arg);
void* courier(void* arg);
void* manager(void* arg);

void reset_data() {
    // Reset all data
    orders_completed = 0;
    all_orders_handled = 0;

    // Free previous dynamic arrays
    free(order_queue.order_queue);
    free(cooked_queue.cooked_orders);
    free(chef_counts);
    free(courier_counts);

    // Allocate memory for new dynamic arrays
    order_queue.order_queue = (Order*)malloc(num_orders * sizeof(Order));
    cooked_queue.cooked_orders = (Order*)malloc(num_orders * sizeof(Order));
    chef_counts = (int*)malloc(num_chefs * sizeof(int));
    courier_counts = (int*)malloc(num_couriers * sizeof(int));

    for (int i = 0; i < num_orders; i++) {
        order_queue.order_queue[i] = (Order){.ip = 127001, .port = 8080, .pid = i, .order_id = i + 1, .is_served = 0, .p = 0, .q = 0};
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

int main() {
    while (1) {
        // Get user input
        printf("Enter the number of chefs: ");
        scanf("%d", &num_chefs);
        printf("Enter the number of couriers: ");
        scanf("%d", &num_couriers);
        printf("Enter the courier speed (seconds): ");
        scanf("%d", &courier_speed);
        printf("Enter the number of orders: ");
        scanf("%d", &num_orders);

        reset_data();

        pthread_t chefs[num_chefs];
        pthread_t couriers[num_couriers];
        pthread_t manager_thread;

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
        printf("Most active courier: Courier %d with %d orders\n", max_courier_id, max_courier_count);

        // Destroy semaphores
        sem_destroy(&oven_sem);
        sem_destroy(&paddle_sem);
        sem_destroy(&oven_entry_sem);
    }

    return 0;
}

void* chef(void* arg) {
    int id = (int)(long)arg;

    while (1) {
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
        printf("Chef %d is preparing order %d\n", id, order.order_id);
        sleep(1);

        sem_wait(&oven_sem);

        sem_wait(&paddle_sem);

        // Wait for an entry to the oven
        sem_wait(&oven_entry_sem);

        // Wait for a paddle and oven space to put the order in the oven
        
        

        // Place the order in the oven
        printf("Chef %d placed order %d in the oven\n", id, order.order_id);

        // Release the paddle
        sem_post(&paddle_sem);
        sem_post(&oven_entry_sem);

        // Wait for the order to be cooked
        usleep(500000); // 0.5 second for cooking

        // Wait for a paddle to take the order out of the oven
        sem_wait(&paddle_sem);
        sem_wait(&oven_entry_sem);

        // Take the order out of the oven
        printf("Chef %d took order %d out of the oven\n", id, order.order_id);

        // Release the oven space
        sem_post(&oven_sem);

        sem_post(&oven_entry_sem);

        // Release the paddle
        sem_post(&paddle_sem);

        // Release the oven entry
       

        // Put the cooked order in the cooked orders queue
        pthread_mutex_lock(&cooked_queue.cooked_mutex);
        cooked_queue.cooked_orders[cooked_queue.cooked_count++] = order;
        printf("Chef %d placed order %d in the cooked orders queue\n", id, order.order_id);
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
        }

        int count = cooked_queue.cooked_count < COURIER_THRESHOLD ? cooked_queue.cooked_count : COURIER_THRESHOLD;
        for (int i = 0; i < count; i++) {
            deliveries[i] = cooked_queue.cooked_orders[--cooked_queue.cooked_count];
        }
        pthread_mutex_unlock(&cooked_queue.cooked_mutex);

        // Deliver the orders
        printf("Courier %d is delivering orders:", id);
        for (int i = 0; i < count; i++) {
            printf(" %d", deliveries[i].order_id);
        }
        printf(". Delivery time: %d seconds\n", courier_speed);
        sleep(courier_speed);

        // Increment courier's delivery count
        courier_counts[id] += count;
    }

    return NULL;
}

void* manager(void* arg) {
    while (1) {
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
        }
        // Signal couriers to pick up cooked orders
        pthread_cond_broadcast(&cooked_queue.cooked_cond);
        pthread_mutex_unlock(&cooked_queue.cooked_mutex);
    }
    return NULL;
}
