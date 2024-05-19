#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define TRUE 1
#define FALSE 0
#define MAX_PICKUP 4
#define MAX_AUTOMOBILE 8
#define NUM_CAR_OWNERS 20
#define RANDOM_ARRIVAL 1



sem_t newCar;

sem_t newAutomobile;
sem_t newPickup;

sem_t inChargeForPickup;
sem_t inChargeForAutomobile;




void * carOwner(void *arg);
void * carAttendant(void *arg);
void sleep_random(int min, int max);





int main (int argc, char *argv[]) {



    srand(time(NULL));


    sem_init(&newCar, 0, 1);

    sem_init(&newAutomobile, 0, 0);
    sem_init(&newPickup, 0, 0);

    sem_init(&inChargeForPickup, 0, MAX_PICKUP);
    sem_init(&inChargeForAutomobile, 0, MAX_AUTOMOBILE);


    pthread_t carOwnerThread[NUM_CAR_OWNERS];
    pthread_t carAttendantThread[2];

    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        int check = pthread_create(&carOwnerThread[i], NULL, carOwner, (void *) i );
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
    }

    for (int i = 0; i < 2; i++) {
        int check = pthread_create(&carAttendantThread[i], NULL, carAttendant, (void *) i);
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
    }


    for (int i = 0; i < NUM_CAR_OWNERS; i++) {
        int check = pthread_join(carOwnerThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carOwner Error: %d\n", check);
            exit(1);
        }
    }
    
    
    pthread_cancel(carAttendantThread[0]);
    pthread_cancel(carAttendantThread[1]);
    

    for (int i = 0; i < 2; i++) {
        
        int check = pthread_join(carAttendantThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carAttendant Error: %d\n", check);
            exit(1);
        }
    }



    sem_destroy(&newCar);
    sem_destroy(&inChargeForPickup);
    sem_destroy(&inChargeForAutomobile);

    return 0;

}


void * carOwner(void *arg) {

    int type = rand() % 2;

    int carOwnerID = (int)arg;
    int free_parking_spots = 0;

    if(RANDOM_ARRIVAL){
        sleep_random(1, 5);
    }

    if (type == 0) {
        
        printf("\n-> Pickup %d owner arrives.\n", carOwnerID);

        sem_wait(&newCar);
        
        printf("\n+ Pickup %d is parked in temporary parking.\n", carOwnerID);

        sem_getvalue(&inChargeForPickup, &free_parking_spots);

        if (free_parking_spots == 0) {
            printf("\n-- No available parking spots for pickup %d. Pickup %d owner leaves the temporary park slot.\n", carOwnerID, carOwnerID);
            sem_post(&newCar);
            return NULL;
        }

        sem_post(&newPickup);
        
        sleep_random(5, 10);

        sem_getvalue(&inChargeForPickup, &free_parking_spots);
        printf("\n$ Pickup %d owner leaves the park slot. Free parking spots on Pickup: %d\n", carOwnerID, free_parking_spots+1);
       
        sem_post(&inChargeForPickup);

    }

    else {

        
        printf("\n-> Automobile %d owner arrives.\n", carOwnerID);

        sem_wait(&newCar);
        
        printf("\n+ Automobile %d is parked in temporary parking.\n", carOwnerID);

        sem_getvalue(&inChargeForAutomobile, &free_parking_spots);
        if (free_parking_spots == 0) {
            printf("\n-- No available parking spots for automobile %d. Automobile %d owner leaves the temporary park slot.\n", carOwnerID, carOwnerID);
            sem_post(&newCar);
            return NULL;
        }

        sem_post(&newAutomobile);
        
        sleep_random(5, 10);

        sem_getvalue(&inChargeForAutomobile, &free_parking_spots);
        printf("\n$ Automobile %d owner leaves the park slot. Free parking spots on Automobile: %d\n", carOwnerID, free_parking_spots+1);
        
        sem_post(&inChargeForAutomobile);
    }
    

   
}


void * carAttendant(void *arg) {

    int type = (int) arg;
    int free_parking_spots = 0;


    while (1) {

        if (type == 0) {
            
            sem_wait(&newPickup);
            sem_wait(&inChargeForPickup);
            sem_getvalue(&inChargeForPickup, &free_parking_spots);
            
            printf("\n# Vale takes the pickup from temporary parking.\n"
                "# Pickup is parked in permanent parking.\n"
                "# Available free permanent pickup parking spots now: %d\n", free_parking_spots);
            
            sem_post(&newCar);
            

        }

        else {
            
            sem_wait(&newAutomobile);
            sem_wait(&inChargeForAutomobile);
            sem_getvalue(&inChargeForAutomobile, &free_parking_spots);
                 printf("\n# Vale takes the automobile from temporary parking.\n"
                     "# Automobile is parked in permanent parking.\n"
                     "# Available free permanent automobile parking spots now: %d\n", free_parking_spots);
            
            sem_post(&newCar);
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