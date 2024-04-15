#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#define FIFO_NAME_1 "/tmp/fifo1"
#define FIFO_NAME_2 "/tmp/fifo2"


int main (int argc, char *argv[]) {

    pid_t child1;

    if (mkfifo(FIFO_NAME_1, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create fifo1");
            exit(1);
        }
    }

    if (mkfifo(FIFO_NAME_2, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create fifo2");
            exit(1);
        }
    }

    printf("FIFO1 created\n");

    child1 = fork();
    pid_t child2 = fork();

    if (child1 == -1 || child2 == -1) {
        perror("fork child1 or child2");
        exit(EXIT_FAILURE);
    } else if (child1 == 0) {
        // Child 1 process
        int fifo1_fd = open(FIFO_NAME_1, O_RDONLY);
        if (fifo1_fd == -1) {
            perror("open FIFO1");
            exit(EXIT_FAILURE);
        }
        
      
        int child_number;

        if (read(fifo1_fd, &child_number, sizeof(child_number)) == -1) {
            perror("read FIFO1");
            exit(EXIT_FAILURE);
        }

        printf("Child_1 read number from FIFO1: %d\n", child_number);

        child_number *=10;

        //write to fifo2

        int fifo2_fd = open(FIFO_NAME_2, O_WRONLY);
        if (fifo2_fd == -1) {
            perror("open FIFO2");
            exit(EXIT_FAILURE);
        }


        if (write(fifo2_fd, &child_number, sizeof(child_number)) == -1) {
            perror("write FIFO2");
            exit(EXIT_FAILURE);
        }

        printf("Child 1 wrote number to FIFO2: %d\n", child_number);



        close(fifo1_fd);
        exit(EXIT_SUCCESS);

    }

    else if (child2 == 0) {
        // Child 2 process
        int fifo2_fd = open(FIFO_NAME_2, O_RDONLY);
        if (fifo2_fd == -1) {
            perror("open FIFO2");
            exit(EXIT_FAILURE);
        }


        int first;
        int second;

        if (read(fifo2_fd, &first, sizeof(first)) == -1) {
            perror("read FIFO2");
            exit(EXIT_FAILURE);
        }

        printf("Child 2 read number from FIFO2 first time: %d\n", first);

        if (read(fifo2_fd, &second, sizeof(second)) == -1) {
            perror("read FIFO2");
            exit(EXIT_FAILURE);
        }

        printf("Child 2 read number from FIFO2 second time: %d\n", second);

        close(fifo2_fd);
        exit(EXIT_SUCCESS);
    }

    int fifo1_fd = open(FIFO_NAME_1, O_WRONLY);
    if (fifo1_fd == -1) {
        perror("open FIFO1");
        exit(EXIT_FAILURE);
    }

    int fifo2_fd = open(FIFO_NAME_2, O_WRONLY);
    if (fifo2_fd == -1) {
        perror("open FIFO2");
        exit(EXIT_FAILURE);
    }

    int parent_number_1 = 100;

    if (write(fifo1_fd, &parent_number_1, sizeof(parent_number_1)) == -1) {
        perror("write FIFO1");
        exit(EXIT_FAILURE);
    }

    printf("Parent wrote number_1: %d\n", parent_number_1);

    
    if(write(fifo2_fd, &parent_number_1, sizeof(parent_number_1)) == -1){
        perror("write FIFO2");
        exit(EXIT_FAILURE);
    }

    close(fifo1_fd);
    close(fifo2_fd);

    wait(NULL);
    wait(NULL);

    unlink(FIFO_NAME_1);
    unlink(FIFO_NAME_2);

    return 0;
}