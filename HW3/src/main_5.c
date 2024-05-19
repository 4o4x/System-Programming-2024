#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>


#define MAX_PICKUP 4
#define MAX_AUTOMOBILE 8



int mFree_perm_pickup = 4;
int mFree_perm_automobile = 8;

int automobileValetResult = 0;
int pickupValetResult = 0;

sem_t newCar;

sem_t inChargeForPickup;
sem_t inChargeForAutomobile;




void * carOwner(void *arg);
void * carAttendant(void *arg);

int main (int argc, char *argv[]) {

    int numberOfCarOwners = 20;

    srand(time(NULL));


    sem_init(&newCar, 0, 1);

    sem_init(&inChargeForPickup, 0, 0);
    sem_init(&inChargeForAutomobile, 0, 0);


    pthread_t carOwnerThread[numberOfCarOwners];
    pthread_t carAttendantThread[2];

    //pthread_attr_t attr;

    //pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int i;

    for (i = 0; i < numberOfCarOwners; i++) {
        int check = pthread_create(&carOwnerThread[i], NULL, carOwner, NULL);
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
    }
    // Create 2 car attendant threads without returning any value
    for (i = 0; i < 2; i++) {
        int check = pthread_create(&carAttendantThread[i], NULL, carAttendant, (void *) i);
        if (check != 0) {
            printf("Error in creating thread\n");
            exit(1);
        }
        
    }

 

    for (i = 0; i < numberOfCarOwners; i++) {
        int check = pthread_join(carOwnerThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carOwner Error: %d\n", check);
            exit(1);
        }
    }

    
    pthread_cancel(carAttendantThread[0]);
    pthread_cancel(carAttendantThread[1]);
    

    for (i = 0; i < 2; i++) {
        
        int check = pthread_join(carAttendantThread[i], NULL);
        if (check != 0) {
            printf("Error in detaching thread in carAttendant Error: %d\n", check);
            exit(1);
        }
    }


    //sleep(1);
    sem_destroy(&newCar);
    sem_destroy(&inChargeForPickup);
    sem_destroy(&inChargeForAutomobile);

    //pthread_attr_destroy(&attr);

    //pthread_detach(pthread_self());


    return 0;

}


void * carOwner(void *arg) {

    int random = rand() % 2;

    if (random == 0) {

        sem_wait(&newCar); // start waiting for a parking spot critical section

        printf("------------------\n");
        printf("A pickup car is arrived.\n");

        printf("Pickup car serving to valet parking.\n");
        printf("------------------\n");

        sem_post(&inChargeForPickup); // Valet parking is in charge of the pickup car start of critical section

    } 
    
    else {

        sem_wait(&newCar); // start waiting for a parking spot critical section

        printf("------------------\n");
        printf("An automobile is arrived.\n");

        printf("Automobile serving to valet parking.\n");
        printf("------------------\n");

        sem_post(&inChargeForAutomobile); // Valet parking is in charge of the automobile start of critical section
    }
    
    return NULL;
}


void * carAttendant(void *arg) {

    int type = (int) arg;


    while (1) {

        if (type == 0) {


            sem_wait(&inChargeForPickup);

            printf("**************************\n");
            printf("Free permanent pickup car parking spots: %d\n", mFree_perm_pickup);

            if (mFree_perm_pickup == 0) {
                printf("No parking available for pickup car in permanent parking. Leaving...\n");
                sem_post(&newCar);
                continue;   
            }
            

            mFree_perm_pickup--;

            printf("Pickup car is parked in permanent parking.\n");
            printf("Available free permanent pickup car parking spots now: %d\n", mFree_perm_pickup);

            printf("**************************\n");
            sem_post(&newCar);
        }

        else {

            sem_wait(&inChargeForAutomobile);
            printf("**************************\n");
            printf("Free permanent automobile parking spots: %d\n", mFree_perm_automobile);

            if (mFree_perm_automobile == 0) {
                printf("No parking available for automobile in permanent parking. Leaving...\n");
                sem_post(&newCar);
                continue;   
            }

            mFree_perm_automobile--;

            printf("Automobile is parked in permanent parking.\n");
            printf("Available free permanent automobile parking spots now: %d\n", mFree_perm_automobile);
            printf("**************************\n");
            sem_post(&newCar);
        }
    }
    printf("Thread %d is exiting...\n", type);
    return NULL;
}