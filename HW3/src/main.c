#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define PICKUP 0
#define AUTOMOBILE 1


#define MAX_PICKUP 4
#define MAX_AUTOMOBILE 8
#define NUM_CAR_OWNERS 30
#define RANDOM_ARRIVAL 1

int free_parking_spots_pickup = MAX_PICKUP;
int free_parking_spots_automobile = MAX_AUTOMOBILE;

int automobileID = 0;
int pickupID = 0;







sem_t newAutomobile;
sem_t newPickup;

sem_t inChargeForPickup;
sem_t inChargeForAutomobile;




void * carOwner(void *arg);
void * carAttendant(void *arg);
void sleep_random(int min, int max);





int main (int argc, char *argv[]) {



    srand(time(NULL));


    // Initialize semaphores
    sem_init(&newAutomobile, 0, 1);
    sem_init(&newPickup, 0, 1);

    sem_init(&inChargeForPickup, 0, 0);
    sem_init(&inChargeForAutomobile, 0, 0);


    pthread_t carOwnerThread[NUM_CAR_OWNERS];
    pthread_t carAttendantThread[2];

    // Create car owner threads
    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        int *arg = malloc(sizeof(*arg));
        if (arg == NULL) {
            fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
            exit(EXIT_FAILURE);
        }
        *arg = i;

        int check = pthread_create(&carOwnerThread[i], NULL, carOwner,arg );
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
    }

    // Create car attendant threads
    for (int i = 0; i < 2; i++) {
        
        int *arg = malloc(sizeof(*arg));
        if (arg == NULL) {
            fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
            exit(EXIT_FAILURE);
        }

        *arg = i;

        int check = pthread_create(&carAttendantThread[i], NULL, carAttendant, arg);
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
    }

    // Join car owner threads
    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        int check = pthread_join(carOwnerThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carOwner Error: %d\n", check);
            exit(1);
        }
    }
    
    // Cancel car attendant threads
    pthread_cancel(carAttendantThread[0]);
    pthread_cancel(carAttendantThread[1]);
    
    // Join car attendant threads
    for (int i = 0; i < 2; i++) {
        
        int check = pthread_join(carAttendantThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carAttendant Error: %d\n", check);
            exit(1);
        }
    }



    
    sem_destroy(&inChargeForPickup);
    sem_destroy(&inChargeForAutomobile);
    sem_destroy(&newAutomobile);
    sem_destroy(&newPickup);

    printf("\n\nProgram is exiting...\n");

    return 0;

}


void * carOwner(void *arg) {

    int type = rand() % 2;


    if(RANDOM_ARRIVAL){
        sleep_random(0, 10);
    }

    if (type == PICKUP) {

        int current_id =  ++pickupID;
        
        printf("\n-> Pickup %d owner arrives.\n", current_id);

        sem_wait(&newPickup);
        
        printf("\n+ Pickup %d is parked in temporary parking.\n", current_id);

        if(free_parking_spots_pickup == 0){

            printf("\n- The parking lot is full. "
            "Pickup %d owner leaves the temporary parking slot.\n", 
            current_id);

            sem_post(&newPickup);
            return NULL;
        }

        sem_post(&inChargeForPickup);

        sleep_random(2, 5);

        printf("\n$ Pickup %d owner leaves the park slot."
        "Free parking spots on Pickup: %d\n", 
        current_id, ++free_parking_spots_pickup);

        return NULL;
    }

    else {

        int current_id =  ++automobileID;
        
        printf("\n-> Automobile %d owner arrives.\n", current_id);

        sem_wait(&newAutomobile);
        
        printf("\n+ Automobile %d is parked in temporary parking.\n", current_id);

        if(free_parking_spots_automobile == 0){
            printf("\n- The parking lot is full. Automobile %d owner leaves the temporary parking slot.\n", current_id);
            sem_post(&newAutomobile);
            return NULL;
        }

        sem_post(&inChargeForAutomobile);

        
        sleep_random(2, 5);

        printf("\n$ Automobile %d owner leaves the park slot. Free parking spots on Automobile: %d\n", current_id, ++free_parking_spots_automobile);

        return NULL;
    }
    

   
}


void * carAttendant(void *arg) {

    int type = *(int*)arg;

    while (1) {

        if (type == PICKUP) {
            
            sem_wait(&inChargeForPickup);

            free_parking_spots_pickup--;

            printf("\n# Vale takes the pickup %d from temporary parking.\n"
                     "# Pickup %d is parked in permanent parking.\n"
                     "# Available free permanent pickup parking spots now: %d\n", 
                     pickupID, pickupID, free_parking_spots_pickup);
            

            sem_post(&newPickup);
        }

        else {
            
            sem_wait(&inChargeForAutomobile);

            free_parking_spots_automobile--;

            printf("\n# Vale takes the automobile %d from temporary parking.\n"
                     "# Automobile %d is parked in permanent parking.\n"
                     "# Available free permanent automobile parking spots now: %d\n", automobileID, automobileID, free_parking_spots_automobile);
            

            sem_post(&newAutomobile);
        }
    }
    printf("Thread %d is exiting...\n", type);
    return NULL;
}


void sleep_random(int min, int max) {
    
    struct timespec req, rem;

    // Generate a random double number between 1.0 and 5.0
    double random_time = (double)rand() / RAND_MAX * (max - min) + min;

    // Separate into seconds and nanoseconds
    req.tv_sec = (time_t)random_time;
    req.tv_nsec = (long)((random_time - req.tv_sec) * 1e9);

    // Call nanosleep
    if (nanosleep(&req, &rem) == -1) {
        perror("nanosleep");
    }
}