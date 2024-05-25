// Very basic C code to signal

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


int interrupted = 1;

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("Interrupt signal received. Cleaning up...\n");
        interrupted = 0;
    }
}

int main() {
    // Register signal handler
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Infinite loop
    while (interrupted) {
        printf("Hello, World!\n");
        sleep(1);
    }

    printf("Exiting...\n");

    return 0;
}
