#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MAX_PATH 4096
#define MAX_BUFFER_SIZE 100

typedef struct {
    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
} FilePair;

typedef struct {
    FilePair buffer[MAX_BUFFER_SIZE];
    int buffer_size;
    int in;
    int out;
    volatile int count;
    volatile int done;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    pthread_barrier_t barrier;
} Buffer;

Buffer buffer;
int num_workers;

unsigned int num_files = 0;
unsigned int num_dirs = 0;
unsigned int num_bytes = 0;
unsigned int num_fifos = 0;

void *worker(void *arg);
void *manager(void *arg);
void copy_file(const char *src, const char *dst);
void copy_directory(const char *src, const char *dst, const char *base_src_dir);
void create_directory(const char *path);

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("Interrupt signal received. Cleaning up...\n");
        pthread_mutex_lock(&buffer.mutex);
        buffer.done = 1;
        pthread_cond_broadcast(&buffer.not_empty);
        pthread_cond_broadcast(&buffer.not_full);
        pthread_mutex_unlock(&buffer.mutex);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dst_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int buffer_size = atoi(argv[1]);
    num_workers = atoi(argv[2]);
    char *src_dir = argv[3];
    char *dst_dir = argv[4];

    if (buffer_size <= 0 || num_workers <= 0) {
        fprintf(stderr, "Invalid buffer size or number of workers\n");
        exit(EXIT_FAILURE);
    }

    // Signal handler
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    buffer.buffer_size = buffer_size;
    buffer.in = 0;
    buffer.out = 0;
    buffer.count = 0;
    buffer.done = 0;
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_full, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);
    pthread_barrier_init(&buffer.barrier, NULL, num_workers + 1);

    pthread_t manager_thread;
    pthread_t worker_threads[num_workers];

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    create_directory(dst_dir);

    char *args[2] = {src_dir, dst_dir};
    pthread_create(&manager_thread, NULL, manager, (void *)args);

    for (int i = 0; i < num_workers; i++) {
        pthread_create(&worker_threads[i], NULL, worker, NULL);
    }

    pthread_join(manager_thread, NULL);

    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }
    long milliseconds = nanoseconds / 1000000;
    long minutes = seconds / 60;
    seconds = seconds % 60;

    printf("\n---------------STATISTICS--------------------\n");
    printf("Consumers: %d - Buffer Size: %d\n", num_workers, buffer_size);
    printf("Number of Regular Files: %d\n", num_files);
    printf("Number of FIFO Files: %d\n", num_fifos);
    printf("Number of Directories: %d\n", num_dirs);
    printf("TOTAL BYTES COPIED: %d\n", num_bytes);
    printf("TOTAL TIME: %02ld:%02ld.%03ld (min:sec.milli)\n\n", minutes, seconds, milliseconds);

    pthread_mutex_destroy(&buffer.mutex);
    pthread_cond_destroy(&buffer.not_full);
    pthread_cond_destroy(&buffer.not_empty);
    pthread_barrier_destroy(&buffer.barrier);

    return 0;
}

void *manager(void *arg) {

    pthread_barrier_wait(&buffer.barrier);  // Wait for all worker threads to be created
    

    char **dirs = (char **)arg;
    char *src_dir = dirs[0];
    char *dst_dir = dirs[1];
    copy_directory(src_dir, dst_dir, src_dir);

    pthread_mutex_lock(&buffer.mutex);
    buffer.done = 1;
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_mutex_unlock(&buffer.mutex);

    pthread_barrier_wait(&buffer.barrier);  // Wait for all worker threads to finish copying

    // Perform some additional processing or cleanup
    printf("\n\nManager: All files copied.\n");
    


    return NULL;
}

void copy_directory(const char *src_dir, const char *dst_dir, const char *base_src_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_LNK) {
            num_fifos++;
        }

        char src_path[MAX_PATH], dst_path[MAX_PATH];
        snprintf(src_path, MAX_PATH, "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, MAX_PATH, "%s/%s", dst_dir, src_path + strlen(base_src_dir) + 1);

        struct stat st;
        if (stat(src_path, &st) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            create_directory(dst_path);
            copy_directory(src_path, dst_dir, base_src_dir);
        } else {
            pthread_mutex_lock(&buffer.mutex);

            while (buffer.count == buffer.buffer_size && !buffer.done) {
                pthread_cond_wait(&buffer.not_full, &buffer.mutex);
            }

            if (buffer.done) {
                pthread_mutex_unlock(&buffer.mutex);
                break;
            }

            strcpy(buffer.buffer[buffer.in].src_path, src_path);
            strcpy(buffer.buffer[buffer.in].dst_path, dst_path);
            buffer.in = (buffer.in + 1) % buffer.buffer_size;
            buffer.count++;

            pthread_cond_signal(&buffer.not_empty);
            pthread_mutex_unlock(&buffer.mutex);
        }
    }

    closedir(dir);
}

void *worker(void *arg) {

    pthread_barrier_wait(&buffer.barrier);  // Wait for manager thread to create all worker threads



    while (1) {

        pthread_mutex_lock(&buffer.mutex);

        while (buffer.count == 0 && !buffer.done) {
            pthread_cond_wait(&buffer.not_empty, &buffer.mutex);
        }

        if (buffer.count == 0 && buffer.done) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }

        FilePair file_pair = buffer.buffer[buffer.out];
        buffer.out = (buffer.out + 1) % buffer.buffer_size;
        buffer.count--;

        pthread_cond_signal(&buffer.not_full);
        pthread_mutex_unlock(&buffer.mutex);

        copy_file(file_pair.src_path, file_pair.dst_path);
    }

    pthread_barrier_wait(&buffer.barrier);  // Wait for all threads to finish copying

    return NULL;
}

void copy_file(const char *src, const char *dst) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("open src");
        return;
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("open dst");
        close(src_fd);
        return;
    }

    char write_buffer[4096];
    ssize_t bytes;
    while ((bytes = read(src_fd, write_buffer, sizeof(write_buffer))) > 0) {
        if (write(dst_fd, write_buffer, bytes) != bytes) {
            perror("write");
            break;
        }

        pthread_mutex_lock(&buffer.mutex);
        num_bytes += bytes;
        pthread_mutex_unlock(&buffer.mutex);
    }

    if (bytes < 0) {
        perror("read");
    }

    close(src_fd);
    close(dst_fd);

    //printf("Copied %s to %s\n", src, dst);

    num_files++;
}

void create_directory(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        num_dirs++;
        if (mkdir(path, 0700) == -1) {
            perror("mkdir");
        }
    }
}
