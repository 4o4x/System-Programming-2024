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
#include <signal.h>

#define FIFO_NAME_1 "/tmp/fifo1"
#define FIFO_NAME_2 "/tmp/fifo2"

volatile int children = 2;

// Take a number as an argument and return random array of numbers size of the given number
void random_array(int arr[], int number);
void print_array(int arr[],int size);
int sum_array(int arr[],int size);
int multiply_array(int arr[],int size);

void sigchld_handler(int sig) {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                printf("Child with PID %d terminated with exit status %d.\n", pid, exit_status);
            } else if (WIFSIGNALED(status)) {
                int signal_number = WTERMSIG(status);
                printf("Child with PID %d killed by signal %d.\n", pid, signal_number);
            }
            children--;
        }

        

}



int main (int argc, char *argv[]) {

    //random seed
    srand(time(0));
    

    if (argc != 2) {
        printf("Usage: %s <number>\n", argv[0]);
        exit(1);
    }

    const int number = atoi(argv[1]);

    printf("Input number is given as: %d\n\n",number);

    int arr[number];

    random_array(arr, number);

    printf("Generated array: ");
    print_array(arr,number);
    printf("\n");


    if (mkfifo(FIFO_NAME_1, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create fifo1");
            exit(1);
        }
    }

    //printf("FIFO1 created\n");

    if (mkfifo(FIFO_NAME_2, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create fifo2");
            exit(1);
        }
    }
    //printf("FIFO2 created\n");


    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigfillset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    pid_t pid_1 = fork();
    
    

    if (pid_1 == -1) {
        perror("Failed to fork");
        exit(1);
    }

    
    else if (pid_1 == 0) {

        printf("Child Process 1 -> Child 1 Started\n\n");
        printf("Child Process 1 -> Child 1 PID: %d\n\n",getpid());
        sleep(10);
        printf("Child Process 1 -> Child 1 is proceeding...\n\n");

        int fifo1_fd = open(FIFO_NAME_1, O_RDONLY);
        if (fifo1_fd == -1) {
            perror("Failed to open fifo1 for reading");
            exit(1);
        }

        //printf("Child Process 1 -> Child 1 opened fifo1 for reading\n\n");


        int * arr_1 = (int *)malloc(number * sizeof(int));

        //printf("Child Process 1 -> Child 1 reading array from fifo1\n\n");
        if (read(fifo1_fd, arr_1, number * sizeof(int)) == -1) {
            perror("Failed to read from fifo1");
            free(arr_1);
            exit(1);
        }

        //printf("Child Process 1 -> Child 1 received array: ");
        //print_array(arr_1,number);
        //printf("\n\n");

        close(fifo1_fd);
        //printf("Child Process 1 -> Child 1 closed fifo1\n\n");


        int sum = sum_array(arr_1,number);
        //printf("Child Process 1 -> Child 1 calculated the sum: %d\n\n",sum);

        free(arr_1);
    
        //write to fifo2

        int fifo2_fd = open(FIFO_NAME_2, O_WRONLY);
        if (fifo2_fd == -1) {
            perror("Failed to open fifo2 for writing");
            exit(1);
        }

        //printf("Child Process 1 -> Child 1 opened fifo2 for writing\n\n");

        

        //printf("Child Process 1 -> Child 1 writing sum to fifo2\n\n");
        if (write(fifo2_fd, &sum, sizeof(sum)) == -1) {
            perror("Failed to write to fifo2");
            exit(1);
        }

        //printf("Child Process 1 -> Child 1 wrote sum to fifo2: %d\n\n",sum);

        close(fifo2_fd);
        //printf("Child Process 1 -> Child 1 closed fifo2\n\n");

        exit(0);

    }

    else {        
        pid_t pid_2 = fork();

        if(pid_2 == -1){
            perror("Failed to fork");
            exit(1);
        }

        else if (pid_2 == 0) {
            
            printf("Child Process 2 -> Child 2 Started\n\n");
            printf("Child Process 2 -> Child 2 PID: %d\n\n",getpid());
            sleep(10);
            printf("Child Process 2 -> Child 2 is proceeding...\n\n");
            int * arr_2 = (int *)malloc(number * sizeof(int));
            int fifo2_fd = open(FIFO_NAME_2, O_RDONLY);
            if (fifo2_fd == -1) {
                perror("Failed to open fifo2 for reading");
                exit(1);
            }

            //printf("Child Process 2 -> Child 2 opened fifo2 for reading\n\n");

            //printf("Child Process 2 -> Child 2 reading array from fifo2\n\n");
            if (read(fifo2_fd, arr_2, number * sizeof(int)) == -1) {
                perror("Failed to read from fifo2");
                exit(1);
            }

            printf("Child Process 2 -> Child 2 received array: ");
            print_array(arr_2,number);
            printf("\n");

            //read from fifo2

            char * result = (char *)malloc(9 * sizeof(char));
            //printf("Child Process 2 -> Child 2 reading operation from fifo2\n\n");
            if (read(fifo2_fd, result, 9 * sizeof(char)) == -1) {
                perror("Failed to read from fifo2");
                free(result);
                exit(1);
            }

            //printf("Child Process 2 -> Child 2 received command: %s\n\n", result);

            int multiply;


            if(strcmp(result,"multiply") == 0){
                multiply = multiply_array(arr_2,number);
            }

            else{
                perror("Invalid operation");
                exit(1);
            }

            printf("Child Process 2 -> Child 2 calculated multiply: %d\n\n",multiply);

            free(arr_2);
            free(result);
            //printf("Child Process 2 -> Child 2 freed memory\n\n");
            
            int sum_child1;
            //printf("Child Process 2 -> Child 2 reading sum from fifo2\n\n");
            if (read(fifo2_fd, &sum_child1, sizeof(sum_child1)) == -1) {
                perror("Failed to read from fifo2");
                exit(1);
            }
            printf("Child Process 2 -> Child 2 received sum from child 1: %d\n\n", sum_child1);

            printf("Child Process 2 -> Child 2 calculated the result: %d\n\n", sum_child1 + multiply);
            

            close(fifo2_fd);
            exit(0);
        }

        else{
            
            //printf("Parent Process -> Parent Process Started\n\n");

            int fd1;
            if ((fd1 = open(FIFO_NAME_1, O_WRONLY)) == -1) {
                perror("Failed to open fifo1 for writing");
                exit(1);
            }
            //printf("Parent Process -> Parent opened fifo1 for writing\n\n");

            int fd2;
            if ((fd2 = open(FIFO_NAME_2, O_WRONLY)) == -1) {
                perror("Failed to open fifo2 for writing");
                exit(1);
            }
            //printf("Parent Process -> Parent opened fifo2 for writing\n\n");

            
            

            //printf("Parent Process -> Parent writing array to fifo2\n\n");
            write(fd2, arr, number * sizeof(int));
            //printf("Parent Process -> Parent wrote array to fifo2\n\n");


            //printf("Parent Process -> Parent writing operation to fifo2\n\n");
            write(fd2, "multiply", 9 * sizeof(char));
            //printf("Parent Process -> Parent wrote operation to fifo2\n\n");

            //printf("Parent Process -> Parent writing array to fifo1\n\n");
            write(fd1, arr, number * sizeof(int));
            //printf("Parent Process -> Parent wrote array to fifo1\n\n");

            

            while (children > 0) {
                printf("Parent is proceeding...\n");
                sleep(2);
            }

            if (children != 0) {
                fprintf(stderr, "Error: Child process count mismatch. Expected %d, got %d.\n", 0, children);
            } else {
                printf("All child processes have terminated correctly.\n");
            }

            //printf("Parent Process -> Parent waiting for child processes\n\n");

            close(fd1);
            //printf("Parent Process -> Parent closed fifo1\n\n");

            close(fd2);
            //printf("Parent Process -> Parent closed fifo2\n\n");
            
            unlink(FIFO_NAME_1);
            //printf("Parent Process -> Parent unlinked fifo1\n\n");

            unlink(FIFO_NAME_2);
            //printf("Parent Process -> Parent unlinked fifo2\n\n");


            //printf("Parent Process -> Parent Process Finished\n\n");            
            return 0;

        }
    }
    

   
   
    
}


void print_array(int arr[],int size){
    
    printf("[");
    for (size_t i = 0; i < size; i++)
    {
        printf("%d",arr[i]);

        if (i != size-1)
        {
            printf(",");
        }
        
    }
    printf("]\n");
    
}

void random_array(int arr[], int number) {
    for (size_t i = 0; i < number; i++) {
        arr[i] = rand() % 10 +1;
    }
}

int sum_array(int arr[],int size){
    int sum = 0;
    for (size_t i = 0; i < size; i++)
    {
        sum += arr[i];
    }
    return sum;
}

int multiply_array(int arr[],int size){
    int multiply = 1;
    for (size_t i = 0; i < size; i++)
    {
        multiply *= arr[i];
    }
    return multiply;
}